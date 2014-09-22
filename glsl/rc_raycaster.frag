// variables for storing compositing results
vec4 result = vec4(0.0);

#define SAMPLING_BASE_INTERVAL_RCP 200.0

struct VOLUME_STRUCT {
    sampler3D volume_;              // the actual dataset normalized
    vec3 datasetDimensions_;        // the dataset's resolution, e.g. [ 256.0, 128.0, 128.0]
    vec3 datasetDimensionsRCP_;     // Reciprocal of the dataset dimension (= 1/datasetDimensions_)
};

struct TEXTURE_PARAMETERS {
    vec2 dimensions_;        // the texture's resolution, e.g. [256.0, 128.0]
    vec2 dimensionsRCP_;
};

struct LIGHT_SOURCE {
    vec3 position_;        // light position in world space
    vec3 ambientColor_;    // ambient color (r,g,b)
    vec3 diffuseColor_;    // diffuse color (r,g,b)
    vec3 specularColor_;   // specular color (r,g,b)
    vec3 attenuation_;     // attenuation (constant, linear, quadratic)
};

// uniforms needed for shading
uniform vec3 cameraPosition_;   // in world coordinates
uniform float shininess_;       // material shininess parameter
uniform LIGHT_SOURCE lightSource_;

// Settings for the raycaster
uniform float samplingStepSize_;
uniform float samplingRate_;

// declare entry and exit parameters
uniform sampler2D entryPoints_;            // ray entry points
uniform TEXTURE_PARAMETERS entryParameters_;
uniform sampler2D exitPoints_;             // ray exit points
uniform TEXTURE_PARAMETERS exitParameters_;

// declare volume
uniform VOLUME_STRUCT volumeStruct_;    // volume data with parameters

// declare transfer function
uniform sampler1D transferFunc_;

/////////////////////////////////////////////////////

vec3 calculateGradient(in vec3 samplePosition) {
    const vec3 h = volumeStruct_.datasetDimensionsRCP_;
    
    // calculate central differences
    
    vec3 xp = samplePosition + vec3(h.x, 0, 0);
    vec3 xm = samplePosition - vec3(h.x, 0, 0);
    float x = texture(volumeStruct_.volume_, xp).a - texture(volumeStruct_.volume_, xm).a;
    
    vec3 yp = samplePosition + vec3(0, h.y, 0);
    vec3 ym = samplePosition - vec3(0, h.y, 0);
    float y = texture(volumeStruct_.volume_, yp).a - texture(volumeStruct_.volume_, ym).a;
    
    vec3 zp = samplePosition + vec3(0, 0, h.z);
    vec3 zm = samplePosition - vec3(0, 0, h.z);
    float z = texture(volumeStruct_.volume_, zp).a - texture(volumeStruct_.volume_, zm).a;

    vec3 gradient = vec3(x, y, z);
    
    gradient *= 1/(2*h);
    
    return normalize(gradient);
}

vec3 applyPhongShading(in vec3 pos, in vec3 gradient, in vec3 ka, in vec3 kd, in vec3 ks) {
    vec3 lightVector = normalize(lightSource_.position_ - pos);
    vec3 cameraVector = normalize(cameraPosition_ - pos);
    vec3 specularDirection = normalize(cameraVector + lightVector);
    vec3 reflectance = normalize(2 * min(max(dot(lightVector, gradient), 0), 1)*gradient - lightVector);

    vec3 shadedColor = vec3(0.0);

    float diffuseFactor = min(max(dot(gradient, lightVector), 0), 1);
    float specularFactor = pow(min(max(dot(gradient, specularDirection), 0), 1), shininess_);
    //float specularFactor = pow(min(max(dot(cameraVector, reflectance), 0), 1), shininess_);

    shadedColor += min(max(ka * lightSource_.ambientColor_, 0), 1);
    shadedColor += kd * lightSource_.diffuseColor_ * diffuseFactor;
    shadedColor += ks * lightSource_.specularColor_ * specularFactor;

    return (shadedColor);
}
void rayTraversal(in vec3 first, in vec3 last) {
    // calculate the required ray parameters
    float t     = 0.0;
    float tIncr = 0.0;
    float tEnd  = 1.0;
    vec3 rayDirection = last - first;
    tEnd = length(rayDirection);
    rayDirection = normalize(rayDirection);
    tIncr = 1.0/(samplingRate_ * length(rayDirection*volumeStruct_.datasetDimensions_));

    bool finished = false;
    while (!finished) {
        vec3 samplePos = first + t * rayDirection;
        float intensity = texture(volumeStruct_.volume_, samplePos).a;

        vec3 gradient = calculateGradient(samplePos);

        vec4 color = texture(transferFunc_, intensity);
        
        color.rgb = applyPhongShading(samplePos, gradient, color.rgb, color.rgb, vec3(1.0,1.0,1.0));
        
        // if opacity greater zero, apply compositing
        if (color.a > 0.0) {
            color.a = 1.0 - pow(1.0 - color.a, samplingStepSize_ * SAMPLING_BASE_INTERVAL_RCP);
            // Insert your front-to-back alpha compositing code here
            result.rgb = (1.0 - result.a) * color.rgb + result.rgb * result.a;
            result.a = (1.0 - result.a) * color.a + result.a;
        }

        // early ray termination
        if (result.a > 1.0)
            finished = true;

        t += tIncr;
        finished = finished || (t > tEnd);
    }
}

void main() {
    vec3 frontPos = texture(entryPoints_, gl_FragCoord.xy * entryParameters_.dimensionsRCP_).rgb;
    vec3 backPos = texture(exitPoints_, gl_FragCoord.xy * exitParameters_.dimensionsRCP_).rgb;

    // determine whether the ray has to be casted
    if (frontPos == backPos)
        // background needs no raycasting
        discard;
    else
        // fragCoords are lying inside the bounding box
        rayTraversal(frontPos, backPos);

    FragData0 = result;
}
