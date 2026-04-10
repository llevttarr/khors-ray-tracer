#ifndef MAT_WIDGET_H
#define MAT_WIDGET_H
#include "widget.h"
#include "../render/renderer.h"
#include "../scene/scene.h"
struct TexBufs {
    char base[256]{};
    char normal[256]{};
    char specular[256]{};
};
class MatWidget : public Widget{
public:
    MatWidget(Scene& s,Renderer&r,RenderScene&rs):scene(s),renderer(r),rs(rs){}
    void draw() override{

        ImGui::Begin("Materials");
        ImGui::BeginChild("MatList", ImVec2(180, 0), true);
        ImGui::SeparatorText("Scene Materials");

        const auto& mats = rs.mat_v;
        for (int i = 0; i < (int)mats.size(); i++) {
            const auto& d = mats[i].diffuse;
            ImVec4 col(d.x, d.y, d.z, 1.0f);
            ImGui::ColorButton(("##sw" + std::to_string(i)).c_str(), col,ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoBorder,ImVec2(17,17));
            ImGui::SameLine();
            std::string label = mat_names.count(i)? mat_names[i]: ("matid: " + std::to_string(i));
            bool selected = (selected_mat == i);
            if (ImGui::Selectable(label.c_str(), selected)){
                selected_mat = i;
            }
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::Button("+ Add##mat", ImVec2(-1, 0))) {
            Mat m{};
            m.diffuse = {0.8f, 0.8f, 0.8f, 1.0f};
            m.specular = {1.0f, 1.0f, 1.0f, 0.5f};
            m.ambient  = {0.1f, 0.1f, 0.1f, 1.0f};
            uint32_t id = scene.add_mat(m);
            rs.mat_v = scene.get_mats();
            selected_mat = (int)id;
            mat_names[id] = "matid: " + std::to_string(id);
        }
        if (!rs.mat_v.empty()) {
            if (ImGui::Button("Delete##mat", ImVec2(-1, 0))) {
                status_msg = "TODO";
            }
        }
        if (ImGui::Button("random materials")) {
            scene.gen_random_mats(5,0,0,0);
            rs.mat_v = std::move(scene.get_mats());
            renderer.update_mats(rs);
        }

        ImGui::EndChild();
        ImGui::SameLine();
        ImGui::BeginChild("Edit", ImVec2(0, 0), false);

        ImGui::InputText("Material ID", matid_buf,sizeof(matid_buf));
        ImGui::EndChild();
        ImGui::End();
    }
private:
    Scene& scene;
    RenderScene& rs;
    Renderer& renderer;
    char matid_buf[32]{};
    std::string status_msg;
    bool status = true;
    std::unordered_map<int, std::string> mat_names;
    int selected_mat = -1;
};

#endif //mat_widget_H
