#include "scene.h"
#include <random>
#include <chrono> 
#include <iostream>
RenderScene Scene::to_render_scene() const{
    RenderScene res;
    res.tri_v.reserve(1000);
    
    for (const Object& obj : obj_v){
        const Mesh& mesh=mesh_v[obj.mesh_id];
        Mat4<float> t= obj.transform;
        for(size_t i=0;i<mesh.ind.size()-2;i+=3){
            uint32_t i0=mesh.ind[i];
            uint32_t i1=mesh.ind[i+1];
            uint32_t i2=mesh.ind[i+2];
            Vec3<float> p0=mesh.pos[i0];
            Vec3<float> p1=mesh.pos[i1];
            Vec3<float> p2=mesh.pos[i2];
            Vec4<float> v0=Vec4<float>::matvec_mul(t,Vec4<float>(p0.x,p0.y,p0.z,1.0));
            Vec4<float> v1=Vec4<float>::matvec_mul(t,Vec4<float>(p1.x,p1.y,p1.z,1.0));
            Vec4<float> v2=Vec4<float>::matvec_mul(t,Vec4<float>(p2.x,p2.y,p2.z,1.0));
            res.tri_v.push_back(RenderTri{v0,v1,v2,1});
        }
    }
    res.sphr_v=sphere_v;

    // spheres are already included in sphr_v
    return res;
}

Scene::Scene(){
    test_scene_init();
}

Scene::~Scene(){

}
uint32_t Scene::add_mesh(Mesh m){
    mesh_v.push_back(std::move(m));
    return (uint32_t)(mesh_v.size()-1);
}
uint32_t Scene::add_object(Object o){
    obj_v.push_back(std::move(o));
    return (uint32_t)(obj_v.size()-1);
}
uint32_t Scene::add_sphere(Sphr s){
    sphere_v.push_back(std::move(s));
    return (uint32_t)(sphere_v.size()-1);
}
void Scene::test_scene_init(){
    Mesh tri_sphere_mesh=obj_util::create_sphere_tri(3,4);

    uint32_t tri_sphere_id = add_mesh(std::move(tri_sphere_mesh));
    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    std::mt19937 engine(seed);
    std::uniform_real_distribution<float> dist(-10.0, 10.0);

    std::vector<uint32_t> objs{};
    objs.reserve(11);
    Mat4<float> identity{};
    Mat4<float> temp{};
    float r0;
    float r1;
    float r2;
    uint32_t obj;
    for (size_t i=0;i<2;++i){
        r0=dist(engine);
        r1=1.0;
        r2=dist(engine);
        temp=identity.translate(r0,r1,r2);
        obj=add_object(Object{tri_sphere_id,temp});
    }

    sphere_v.reserve(100);
    Sphr i=Sphr{};
    i.cx=-25.0;
    i.cy=1.0;
    i.cz=-25.0;
    i.r=2.0;
    i.matid=1;
    add_sphere(i);

    // Mat4<float> t0{};
    // t0 = t0.translate(10.0,2.0,10.0);
    // uint32_t obj0 = add_object(Object{sphere_id,t0});
    // Mat4<float> t1{};
    // t1 = t1.translate(-10.0,-2.0,-2.0);
    // uint32_t obj1 = add_object(Object{sphere_id,t1});
    // Mat4<float> t2{};
    // t2 = t2.translate(2.0,2.0,-2.0);
    // uint32_t obj2 = add_object(Object{sphere_id,t2});
    
}