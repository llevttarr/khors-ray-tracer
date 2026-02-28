#ifndef OBJECT_H
#define OBJECT_H
#include <cstdint>
#include <string>
#include <vector>
#include <numbers>
#include <stdexcept>
#include "../core/math/mat4.h"
#include "../core/math/vec3.h"

struct MeshLoadException: public std::runtime_error{
    using std::runtime_error::runtime_error;
};
struct Mesh{
    std::vector<Vec3<float>> pos;
    std::vector<uint32_t> ind;
};
struct Object{
    uint32_t mesh_id;
    Mat4<float> transform{};
    uint matid;
};
namespace obj_util{
    Mesh create_sphere_tri(float radius,uint16_t detail_lvl);
    static uint32_t parse_face_ind(const std::string& t, size_t n);
    Mesh load_mesh_obj(const std::string& file_path);
};
#endif //OBJECT_H