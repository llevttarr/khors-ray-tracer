#ifndef IMG_WIDGET_H
#define IMG_WIDGET_H

#include "widget.h"
#include "vk_renderer.h"

class ImgWidget : public Widget{
public:
    ImgWidget(VKRenderer&r) :renderer(r){}
    void draw() override{

        ImGui::Begin("Image save");
        ImGui::Text("Save image");
        ImGui::InputText("File name", filename_buf,sizeof(filename_buf));
        if (ImGui::Button("Save")) {
            try {
                renderer.save_image(std::string(filename_buf));
                status_msg = "Saved: " + std::string(filename_buf);
                status = true;
            } catch (const std::exception& e) {
                status_msg = std::string("Save failed: ") + e.what();
                status= false;
            }
        }
        if (!status_msg.empty()) {
            ImGui::Spacing();
            ImGui::TextColored(status ? ImVec4(0,1,0,1) : ImVec4(1,0.3f,0.3f,1),"%s", status_msg.c_str());
        }

        ImGui::End();
    }
private:
    VKRenderer& renderer;
    std::string status_msg;
    char filename_buf[256]{};
    bool status = true;
};

#endif
