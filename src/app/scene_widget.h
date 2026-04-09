#ifndef SCENE_WIDGET_H
#define SCENE_WIDGET_H

#include "widget.h"
#include "../render/renderer.h"
#include "../scene/scene.h"
#include "../scene/scene_parser.h"

class ObjWidget : public Widget{
    ObjWidget(Scene& s,Renderer&r,RenderScene&rs,SceneParser& sp) : scene(s),renderer(r),r(rs),scene_parser(sp){}
    void draw() override{

        ImGui::Begin("Scene");
        ImGui::Text("Scene management");
        if (ImGui::Button("Save")) {
            
        }
        if (ImGui::Button("Load")) {
            
        }
        
        ImGui::End();
    }
private:
    Scene& scene;
    RenderScene& r;
    SceneParser& scene_parser;
    Renderer& renderer;
};

#endif
