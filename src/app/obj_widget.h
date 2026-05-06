#ifndef OBJ_WIDGET_H
#define OBJ_WIDGET_H

#include <iostream>
#include <string>
#include "widget.h"
#include "gl_renderer.h"
#include "scene.h"
#include "rs_obj_parser.h"

class ObjWidget : public Widget{
public:
    ObjWidget(Scene& s,GLRenderer&r,RenderScene&rs):scene(s),renderer(r),rs(rs){}
    void draw() override{

        ImGui::Begin("Objects");
        ImGui::Text("Object management");
        ImGui::InputText("File name", filename_buf,sizeof(filename_buf));
        if (ImGui::Button("Import")) {
            try {
                rs_obj_parser::load_obj_into_rs(static_cast<std::string>(filename_buf)+".obj",static_cast<std::string>(filename_buf)+".mtl",rs);
                renderer.update_scene(rs);
                std::cout<<"tri_v size"<<rs.tri_v.size()<<std::endl;
                std::cout<<"sphr_v size"<<rs.sphr_v.size()<<std::endl;
                status_msg = "Loaded: " + std::string(filename_buf) + ".obj";
                status  = true;
            } catch (const std::exception& e) {
                status_msg = std::string("Load failed: ") + e.what();
                status  = false;
            }
        }
        // ImGui::SameLine();
        // if (ImGui::Button("Translate")) {
        //     try {

        //     } catch (const std::exception& e) {
        //         status_msg = std::string("Translate failed: ") + e.what();
        //         status  = false;
        //     }
        // }
        // ImGui::SameLine();
        // if (ImGui::Button("Set matid")) {
        //     try {

        //     } catch (const std::exception& e) {
        //         status_msg = std::string("Translate failed: ") + e.what();
        //         status  = false;
        //     }
        // }
        if (!status_msg.empty()) {
            ImGui::Spacing();
            ImGui::TextColored(status ? ImVec4(0,1,0,1) : ImVec4(1,0.3f,0.3f,1),"%s", status_msg.c_str());
        }
        ImGui::End();
    }
private:
    Scene& scene;
    RenderScene& rs;
    GLRenderer& renderer;
    char filename_buf[256]{};
    std::string status_msg;
    bool status = true;
};

#endif
