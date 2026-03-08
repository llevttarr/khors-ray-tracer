#ifndef SCENE_H
#define SCENE_H
#include <vector>
#include <random>
#include <chrono> 

#include "object.h"
#include "textures.h"
#include "../core/math/vec4.h"
#include "../core/math/vec3.h"
#include "../core/math/vec2.h"
struct RenderTri{
    Vec4<float> v0;
    Vec4<float> v1;
    Vec4<float> v2;
    Vec2<float> uv0;
    Vec2<float> uv1;
    Vec2<float> uv2;
    uint matid;
    uint pad0;
};
struct BVH{
    Vec4<float> mindat;
    Vec4<float> maxdat;
};
struct Light{
    Vec4<float> pos;
    Vec4<float> ambient;
    Vec4<float> diffuse;
    Vec4<float> specular;
};
struct AABB{
    Vec3<float> minv;
    Vec3<float> maxv;
};
struct Prim{
    AABB aabb;
    Vec3<float> c;
    uint32_t dat;
};
struct Mat{
    Vec4<float> ambient;
    Vec4<float> diffuse;
    Vec4<float> specular;
    Vec4<float> emission;
    Vec3<int32_t> texture;
    Vec2<float> uv_scale;
    Vec2<float> uv_offset;
    int32_t flags;
};
struct Sphr{
    float cx;
    float cy;
    float cz;
    float r;
    uint matid;
    uint pad0,pad1,pad2;
};
struct RenderScene{
    std::vector<RenderTri> tri_v;
    std::vector<BVH> bvh_v;
    std::vector<Sphr> sphr_v;
    std::vector<Mat> mat_v;
    std::vector<uint32_t> prim_v;
    std::vector<Light> light_v;
    TextureManager tex_manager; 
};
namespace scene_util{
    Vec4<float> rand_vec(std::mt19937& engine, std::uniform_real_distribution<float>& dist);
    uint32_t get_type_id(uint32_t type, uint32_t id);
    AABB tri_to_aabb(const RenderTri& t);
    AABB sphr_to_aabb(const Sphr& s);
    AABB merge_aabb(const AABB& a, const AABB& b);
    AABB prims_vec_aabb(std::vector<Prim>& prims,size_t l,size_t r);
    AABB c_prims_aabb(std::vector<Prim>& prims,size_t l,size_t r);
    uint16_t max_axis(const AABB& a);
    Vec3<float> get_c_aabb(const AABB& a);
    uint32_t build_bvh(std::vector<Prim>& prims,size_t l,size_t r, std::vector<uint32_t>& prim_v,std::vector<BVH>& bvh_v);
};
class Scene{
public:
    Scene();
    ~Scene();
    RenderScene to_render_scene() const;
    uint32_t add_mesh(Mesh m);
    uint32_t add_object(Object o);
    uint32_t add_sphere(Sphr s);
private:
    std::vector<Sphr> sphere_v;
    std::vector<Mesh> mesh_v;
    std::vector<Object> obj_v;
    std::vector<Mat> mat_v;
    std::vector<Light> light_v;
    void test_scene_init();
    TextureManager tex_manager;
};
#endif // SCENE_H