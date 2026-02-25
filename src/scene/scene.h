#ifndef SCENE_H
#define SCENE_H
#include <vector>
#include "object.h"
#include "../core/math/vec4.h"
struct RenderTri{
    Vec4<float> v0;
    Vec4<float> v1;
    Vec4<float> v2;
    uint matid;
    uint pad0,pad1,pad2;
};
struct BVH{
    Vec4<float> mindat;
    Vec4<float> maxdat;
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
    Vec4<float> rgba;
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
};
namespace scene_util{
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
    void test_scene_init();
};
#endif // SCENE_H