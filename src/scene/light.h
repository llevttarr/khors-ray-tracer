#ifndef LIGHT_H
#define LIGHT_H

#include "vec4.h"
#include <cstdint>

enum LightType: int32_t{
    LIGHT_POINT =0,
    LIGHT_SPOT =1,
    LIGHT_AREA =2,
    LIGHT_DIRECTION =3,
    LIGHT_TRIANGLE =4,
};
struct Light{
    Vec4<float> pos;
    Vec4<float> diffuse;
    Vec4<float> dir_type;
    Vec4<float> params1;
    Vec4<float> tangent;
    Vec4<float> bitangent;
};
namespace light_util{
    inline int get_type(const Light& l) {
        return (int)(l.dir_type.w + 0.5f);
    };

    inline Vec3<float> get_dir(const Light& l) {
        return {l.dir_type.x, l.dir_type.y, l.dir_type.z};
    };

    inline float& range(Light& l) { return l.params1.x; };
    inline float& cos_outer(Light& l) { return l.params1.y; };
    inline float& half_w(Light& l) { return l.params1.z; };
    inline float& half_h(Light& l) { return l.params1.w; };
    inline void set_type(Light& l, int t) {
        l.dir_type.w = (float)t;
    };

    inline void set_dir(Light& l, const Vec3<float>& d) {
        l.dir_type.x = d.x;
        l.dir_type.y = d.y;
        l.dir_type.z = d.z;
    };
}

#endif