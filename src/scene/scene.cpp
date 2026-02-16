#include "scene.h"

RenderScene Scene::to_render_scene() const{
    RenderScene res;
    res.tri_v.reserve(10000);
    
    
    
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
