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
    Vec4<float> min;
    Vec4<float> max;
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