
layout(rgba32f,binding = 0) uniform image2D outImage;
layout(binding = 1) uniform sampler2DArray baseTexArr;
layout(binding =2) uniform sampler2DArray normalTexArr;
layout(binding =3) uniform sampler2DArray specularTexArr;

uniform uint width;
uniform uint height;
uniform uint tric;
uniform uint spherec;
uniform uint bvhc;
uniform uint matc;
uniform uint lightc;

uniform vec3 camPos;
uniform vec3 camForward;
uniform vec3 camRight;
uniform vec3 camUp;
uniform float camFov;

layout(std430,binding=1) readonly buffer TriBuf {
    Tri tris[];
};

layout(std430,binding=2) readonly buffer SphrBuf {
    Sphr spheres[];
};
layout(std430,binding=3) readonly buffer BVHBuf {
    BVH bvh_v[];
};
layout(std430,binding=4) readonly buffer MatBuf {
    Mat mats[];
};
layout(std430,binding=5) readonly buffer PrimBuf {
    uint prims[];
};
layout(std430,binding=6) readonly buffer LightBuf {
    Light light_v[];
};
