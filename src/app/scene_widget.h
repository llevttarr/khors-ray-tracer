#ifndef SCENE_WIDGET_H
#define SCENE_WIDGET_H

#include "widget.h"
#include "../render/renderer.h"
#include "../scene/scene.h"
#include "../scene/scene_parser.h"

class SceneWidget : public Widget{
public:
    SceneWidget(Scene& s,Renderer&r,RenderScene&rs,SceneParser& sp) : scene(s),renderer(r),rs(rs),scene_parser(sp){}
    void draw() override{

        ImGui::Begin("Scene");
        ImGui::Text("Scene management");
        ImGui::InputText("File name", filename_buf,sizeof(filename_buf));
        if (ImGui::Button("Save")) {
            
            try {
                std::cout<<"tri_v size"<<rs.tri_v.size()<<std::endl;
                std::cout<<"sphr_v size"<<rs.sphr_v.size()<<std::endl;
                scene_parser.save_scene(std::string(filename_buf));
                status_msg = "Saved: " + std::string(filename_buf) + ".khorssc";
                status = true;
            } catch (const std::exception& e) {
                status_msg = std::string("Save failed: ") + e.what();
                status= false;
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Load")) {
            try {
                scene_parser.load_scene(std::string(filename_buf));
                renderer.update_scene(rs);
                std::cout<<"tri_v size"<<rs.tri_v.size()<<std::endl;
                std::cout<<"sphr_v size"<<rs.sphr_v.size()<<std::endl;
                status_msg = "Loaded: " + std::string(filename_buf) + ".khorssc";
                status  = true;
            } catch (const std::exception& e) {
                status_msg = std::string("Load failed: ") + e.what();
                status  = false;
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Switch tracing type")) {
            renderer.switch_tt();
        }
        ImGui::SameLine();
        if (ImGui::Button("Switch brdf type (only for tracing type 0)")) {
            renderer.switch_brdf();
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
