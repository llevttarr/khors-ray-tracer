#ifndef SCENE_H
#define SCENE_H
#include <vector>
#include "object.h"
#include "../core/math/vec4.h"
struct RenderTri{
    Vec4<float> v0;
    Vec4<float> v1;
    Vec4<float> v2;
};
struct BVH{

};
struct RenderScene{
    std::vector<RenderTri> tri_v;
    std::vector<BVH> bvh_v;
};
class Scene{
public:
    Scene();
    ~Scene();
    RenderScene to_render_scene() const;
    uint32_t add_mesh(Mesh m);
    uint32_t add_object(Object o);
private:
    std::vector<Mesh> mesh_v;
    std::vector<Object> obj_v;
    void test_scene_init();
};
#endif // SCENE_H