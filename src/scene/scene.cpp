#include "scene.h"

RenderScene Scene::to_render_scene() const{
    RenderScene res;
    res.tri_v.reserve(10000);
    
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
            Vec4<float> v1=Vec4<float>::matvec_mul(t,Vec4<float>(p0.x,p0.y,p0.z,1.0));
            Vec4<float> v2=Vec4<float>::matvec_mul(t,Vec4<float>(p0.x,p0.y,p0.z,1.0));
            res.tri_v.push_back(RenderTri{v0,v1,v2});
        }
    }
    
    return res;
}

Scene::Scene(){

}

Scene::~Scene(){

}
void Scene::add_mesh(Mesh m){
    mesh_v.push_back(std::move(m));
}
void Scene::add_object(Object o){
    obj_v.push_back(std::move(o));
}
