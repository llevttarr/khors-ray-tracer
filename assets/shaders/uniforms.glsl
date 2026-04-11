
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
uniform uint framec;
uniform uint init_candidates_restir;

uniform uint brdf_type; //0-phong,1-blinn-phong

uniform vec3 camPos;
uniform vec3 camForward;
uniform vec3 camRight;
uniform vec3 camUp;
uniform float camFov;
