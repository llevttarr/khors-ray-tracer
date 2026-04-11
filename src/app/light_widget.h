#ifndef LIGHT_WIDGET_H
#define LIGHT_WIDGET_H

#include "widget.h"
#include "../render/renderer.h"
#include "../scene/scene.h"
#include "../scene/scene_parser.h"

class LightWidget : public Widget{
public:
    LightWidget(Scene& s,Renderer&r,RenderScene&rs,SceneParser& sp) : scene(s),renderer(r),rs(rs),scene_parser(sp){}
    void draw() override{

        ImGui::Begin("Light");
        ImGui::Text("Light management");
        if (ImGui::Button("Add")) {
            
        }
        if (!status_msg.empty()) {
            ImGui::Spacing();
            ImGui::TextColored(status ? ImVec4(0,1,0,1) : ImVec4(1,0.3f,0.3f,1),"%s", status_msg.c_str());
        }
        ImGui::End();
    }
private:
    Scene& scene;
    RenderScene& rs;
    SceneParser& scene_parser;
    Renderer& renderer;
    char filename_buf[256]{};
    std::string status_msg;
    bool status = true;
};

#endif
