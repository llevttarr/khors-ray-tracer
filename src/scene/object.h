#ifndef OBJECT_H
#define OBJECT_H
#include <cstdint>
#include <string>
#include <vector>
#include <numbers>
#include "../core/math/mat4.h"
#include "../core/math/vec3.h"
struct Mesh{
    std::vector<Vec3<float>> pos;
    std::vector<uint32_t> ind;
};
struct Object{
    uint32_t mesh_id;
    Mat4<float> transform{};
};
namespace obj_util{
    Mesh create_sphere(uint16_t radius,uint16_t detail_lvl);
};
#endif //OBJECT_H