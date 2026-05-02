#ifndef OBJECT_H
#define OBJECT_H
#include <cstdint>
#include <string>
#include <vector>
#include <numbers>
#include <stdexcept>

#include "mat4.h"
#include "vec3.h"
#include "vec2.h"

struct MeshLoadException: public std::runtime_error{
    using std::runtime_error::runtime_error;
};
struct Mesh{
    std::vector<Vec3<float>> pos;
    std::vector<Vec3<float>> norm_v;
    std::vector<Vec3<float>> tang_v;
    std::vector<Vec3<float>> bitang_v;
    std::vector<Vec2<float>> uv;
    std::vector<uint32_t> ind;
};
struct Object{
    uint32_t mesh_id;
    Mat4<float> transform{};
    uint matid;
};
namespace obj_util{
    Mesh create_sphere_tri(float radius,uint16_t detail_lvl);
    uint32_t parse_face_ind(const std::string& t, size_t n);
    
    [[deprecated("use renderscene utility instead")]]
    Mesh load_mesh_obj(const std::string& file_path);
    void gen_tbn(Mesh& mesh);
};
#endif //OBJECT_H