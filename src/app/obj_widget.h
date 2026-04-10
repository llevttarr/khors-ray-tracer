#ifndef OBJ_WIDGET_H
#define OBJ_WIDGET_H

#include "widget.h"
#include "../render/renderer.h"
#include "../scene/scene.h"

class ObjWidget : public Widget{
public:
    ObjWidget(Scene& s,Renderer&r,RenderScene&rs):scene(s),renderer(r),rs(rs){}
    void draw() override{

        ImGui::Begin("Objects");
        ImGui::Text("Object management");
        if (ImGui::Button("Import")) {
            try {
                scene.load_obj(std::string(filename_buf));
                RenderScene newrs=scene.to_render_scene();
                rs =std::move(newrs);
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
        if (!status_msg.empty()) {
            ImGui::Spacing();
            ImGui::TextColored(status ? ImVec4(0,1,0,1) : ImVec4(1,0.3f,0.3f,1),"%s", status_msg.c_str());
        }
        ImGui::End();
    }
private:
    Scene& scene;
    RenderScene& rs;
    Renderer& renderer;
    char filename_buf[256]{};
    std::string status_msg;
    bool status = true;
};

#endif
