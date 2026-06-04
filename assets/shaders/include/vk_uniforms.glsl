#include "camera.glsl"
layout(push_constant) uniform PC {
    uint width;
    uint height;
    uint tric;
    uint spherec;
    uint bvhc;
    uint matc;
    uint lightc;
    uint framec;
    uint init_candidates_restir;
} pc;
#define width pc.width
#define height pc.height
#define tric pc.tric
#define spherec pc.spherec
#define bvhc pc.bvhc
#define matc pc.matc
#define lightc pc.lightc
#define framec pc.framec
#define init_candidates_restir pc.init_candidates_restir

layout(constant_id = 0) const uint brdf_type = 1u;
