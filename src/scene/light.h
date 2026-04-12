#ifndef LIGHT_H
#define LIGHT_H

#include "../core/math/vec4.h"
#include <cstdint>

enum LightType: uint32_t{
    LIGHT_SPOT =1,
    LIGHT_AREA =2,
    LIGHT_DIRECTION =3,
};
struct Light{
    Vec3<float> pos;
    // Vec4<float> ambient;
    Vec3<float> diffuse;
    // Vec4<float> specular;
    uint32_t type;
    float range;
    Vec3<float> dir;
    float cos_out;
    Vec3<float> tangent;
    float half_width; 
    Vec3<float> bitangent;
    float half_height; 
};

#endif