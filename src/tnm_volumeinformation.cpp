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

    // iX is the index running over the 'x' dimension
    // iY is the index running over the 'y' dimension
    // iZ is the index running over the 'z' dimension
    for (size_t iX = 0; iX < dimensions.x; ++iX) {
	for (size_t iY = 0; iY < dimensions.y; ++iY) {
	    for (size_t iZ = 0; iZ < dimensions.z; ++iZ) {
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
		float intensity = VolumeUInt16::voxel(_data->at(i).voxelIndex);

		_data->at(i).dataValues[0] = intensity;

		//
		// Average
		//
		// Compute the average; the voxel method accepts both a single parameter
		// as well as three parameters
		float average = .0f;
		
		size_t jX = max(iX -1, 0);
		size_t jY = max(iY -1, 0);
		size_t jZ = max(iZ -1, 0);
		
		size_t topX = min(iX + 1, dimensions.x);
		size_t topY = min(iY + 1, dimensions.y);
		size_t topZ = min(iZ + 1, dimensions.z);
		
		for (; jX < topX; jX++) {
		    for (; jY < topY; jY++) {
			for (; jZ < topZ; jZ++) {
			    average += VolumeUInt16::voxel(jX, jY, jZ);
			}
		    }
		}
		
		// remove the value in the middle
		average -= intensity;
		// 3*3*3 - 1 = 26
		average /= 26;

		_data->at(i).dataValues[1] = average;

		//
		// Standard deviation
		//
		float stdDeviation = -1.f;
		// Compute the standard deviation


		_data->at(i).dataValues[2] = stdDeviation;

		//
		// Gradient magnitude
		//
		float gradientMagnitude = -1.f;
		// Compute the gradient direction using either forward, central, or backward
		// calculation and then take the magnitude (=length) of the vector.
		// Hint:  tgt::vec3 is a class that can calculate the length for you


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
