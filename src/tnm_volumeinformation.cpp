#include "modules/tnm093/include/tnm_volumeinformation.h"
#include "voreen/core/datastructures/volume/volumeatomic.h"

namespace voreen {

	const std::string loggerCat_ = "TNMVolumeInformation";
	
namespace {
	// This ordering function allows us to sort the Data vector by the voxelIndex
	// The extraction *should* produce a sorted list, but you never know
	bool sortByIndex(const VoxelDataItem& lhs, const VoxelDataItem& rhs) {
		return lhs.voxelIndex < rhs.voxelIndex;
	}

}

TNMVolumeInformation::TNMVolumeInformation()
    : Processor()
    , _inport(Port::INPORT, "in.volume")
    , _outport(Port::OUTPORT, "out.data")
    , _data(0)
{
    addPort(_inport);
    addPort(_outport);
}

TNMVolumeInformation::~TNMVolumeInformation() {
    delete _data;
}

void TNMVolumeInformation::process() {
    const VolumeHandleBase* volumeHandle = _inport.getData();
    const Volume* baseVolume = volumeHandle->getRepresentation<Volume>();
    const VolumeUInt16* volume = dynamic_cast<const VolumeUInt16*>(baseVolume);
    if (volume == 0) {
        return;
    }
    
    // If we get this far, there actually is a volume to work with

    // If this is the first call, we will create the Data object
    if (_data == 0) {
	_data = new Data;
    }
    
    // Retrieve the size of the three dimensions of the volume
    const tgt::svec3 dimensions = volume->getDimensions();
    // Create as many data entries as there are voxels in the volume
    _data->resize(dimensions.x * dimensions.y * dimensions.z);
    
    int dim_x = dimensions.x;
    int dim_y = dimensions.y;
    int dim_z = dimensions.z;

    // iX is the index running over the 'x' dimension
    // iY is the index running over the 'y' dimension
    // iZ is the index running over the 'z' dimension
    for (int iX = 0; iX < dim_x; ++iX) {
	for (int iY = 0; iY < dim_y; ++iY) {
	    for (int iZ = 0; iZ < dim_z; ++iZ) {
		// i is a unique identifier for the voxel calculated by the following
		// (probably one of the most important) formulas:
		// iZ*dimensions.x*dimensions.y + iY*dimensions.x + iX;
		const size_t i = VolumeUInt16::calcPos(volume->getDimensions(), tgt::svec3(iX, iY, iZ));

		// Setting the unique identifier as the voxelIndex
		_data->at(i).voxelIndex = i;
		// use iX, iY, iZ, i, and the VolumeUInt16::voxel method to derive the measures here

		//
		// Intensity
		//
		// Retrieve the intensity using the 'VolumeUInt16's voxel method
		float intensity = volume->voxel(_data->at(i).voxelIndex);

		_data->at(i).dataValues[0] = intensity;

		//
		// Average
		//
		// Compute the average; the voxel method accepts both a single parameter
		// as well as three parameters
		float average = .0f;
		
		int count = 0;
				
		int topX = std::min(iX + 1, (dim_x-1));
		int topY = std::min(iY + 1, (dim_y-1));
		int topZ = std::min(iZ + 1, (dim_z-1));
		
		for (int jX = std::max(iX - 1, 0); jX < topX; jX++) {
		    for (int jY = std::max(iY - 1, 0); jY < topY; jY++) {
			for (int jZ = std::max(iZ - 1, 0); jZ < topZ; jZ++) {
			    average += volume->voxel(jX, jY, jZ);
			    count++;
			}
		    }
		}
		
		average /= count;

		_data->at(i).dataValues[1] = average;

		//
		// Standard deviation
		//
		float stdDeviation = .0f;
		// Compute the standard deviation

		
		for (int jX = std::max(iX -1, 0); jX < topX; jX++) {
		    for (int jY = std::max(iY -1, 0); jY < topY; jY++) {
			for (int jZ = std::max(iZ -1, 0); jZ < topZ; jZ++) {
			    stdDeviation += std::pow((volume->voxel(jX, jY, jZ) - average), 2);
			}
		    }
		}
		
// 		stdDeviation -= std::pow(intensity - average, 2);
		stdDeviation /= count;
		
		stdDeviation = std::sqrt(stdDeviation);

		_data->at(i).dataValues[2] = stdDeviation;

		//
		// Gradient magnitude
		//
		// Compute the gradient direction using either forward, central, or backward
		// calculation and then take the magnitude (=length) of the vector.
		// Hint:  tgt::vec3 is a class that can calculate the length for you

		int prevX = std::max(iX -1, 0);
		int prevY = std::max(iY -1, 0);
		int prevZ = std::max(iZ -1, 0);
		
		tgt::vec3 gradient = tgt::vec3(.0f);
		
		gradient.x = volume->voxel(topX, iY, iZ) - volume->voxel(prevX, iY, iZ);
		gradient.y = volume->voxel(iX, topY, iZ) - volume->voxel(iX, prevY, iZ);
		gradient.z = volume->voxel(iX, iY, topZ) - volume->voxel(iX, iY, prevZ);
		
		gradient.x /= 2;
		gradient.y /= 2;
		gradient.z /= 2;
		
		// is .size on tgt::vec3 correct? NO 
		float gradientMagnitude = tgt::length(gradient);
		
		_data->at(i).dataValues[3] = gradientMagnitude;
	    }
	}
    }

    // sort the data by the voxel index for faster processing later
    std::sort(_data->begin(), _data->end(), sortByIndex);

    // And provide access to the data using the outport
    _outport.setData(_data, false);
}

} // namespace
