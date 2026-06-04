#ifndef MAT_WIDGET_H
#define MAT_WIDGET_H
#include "widget.h"
#include "renderer.h"
#include "scene.h"
struct TexBuffs {
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

        if (selected_mat >= 0 && selected_mat < (int)rs.mat_v.size()) {
            Mat& m = rs.mat_v[selected_mat];
            ImGui::SeparatorText("Colors");
            constexpr auto flags = ImGuiColorEditFlags_AlphaBar| ImGuiColorEditFlags_Float| ImGuiColorEditFlags_AlphaPreview;
            draw_color4("Ambient", m.ambient, flags);
            draw_color4("Diffuse", m.diffuse, flags);
            draw_color4("Specular", m.specular,flags);
            draw_color4("Emission", m.emission,flags);
            ImGui::SeparatorText("UV Mapping");
            float uv_scale[2] = {m.uv.x, m.uv.y};
            float uv_offset[2] = {m.uv.z, m.uv.w};
            ImGui::SetNextItemWidth(140);
            if (ImGui::DragFloat2("Scale##uv",  uv_scale,  0.01f, 0.001f, 100.f, "%.3f"))
                { m.uv.x = uv_scale[0]; m.uv.y = uv_scale[1]; }
            ImGui::SetNextItemWidth(140);
            if (ImGui::DragFloat2("Offset##uv", uv_offset, 0.01f, -100.f, 100.f, "%.3f"))
                { m.uv.z = uv_offset[0]; m.uv.w = uv_offset[1]; }
            ImGui::SeparatorText("Textures");
            draw_tex_slot("Base", tex_buffs[selected_mat].base,tex_base_loaded, 0);
            draw_tex_slot("Normal", tex_buffs[selected_mat].normal,tex_normal_loaded, 1);
            draw_tex_slot("Specular",tex_buffs[selected_mat].specular, tex_spec_loaded, 2);

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            if (ImGui::Button("Apply to Renderer", ImVec2(180, 0))) {
                scene.change_mat(m, selected_mat);
                renderer.update_mats(rs);
                status_msg = "Material applied.";
            }
            ImGui::SameLine();
            if (ImGui::Button("Reset", ImVec2(60, 0))) {
                rs.mat_v = scene.get_mats();
                status_msg = "Reset to scene state.";
            }
        } else {
            ImGui::TextDisabled("No material selected.");
        }
        if (!status_msg.empty()) {
            ImGui::Spacing();
            ImGui::TextDisabled("%s", status_msg.c_str());
        }

        ImGui::InputText("Material ID", matid_buf,sizeof(matid_buf));
        ImGui::EndChild();
        ImGui::End();
    }
private:
    void draw_color4(const char* label, Vec4<float>& v, ImGuiColorEditFlags flags) {
        float col[4] = {v.x, v.y, v.z, v.w};
        if (ImGui::ColorEdit4(label, col, flags)){
            v = {col[0], col[1], col[2], col[3]};
        }
    }
    void draw_tex_slot(const char* label, char (&buf)[256],std::unordered_map<int,bool>& loaded, int type)
    {
        Mat& m = rs.mat_v[selected_mat];
        int32_t tex_id = (type == 0) ? m.tex.x: (type == 1) ? m.tex.y: m.tex.z;
        bool has_tex = (tex_id >= 0);

        ImVec4 dot_col = has_tex? ImVec4(0.35f, 0.85f, 0.35f, 1.f): ImVec4(0.45f, 0.45f, 0.45f, 0.6f);
        ImGui::TextColored(dot_col, has_tex ? "[T]" : "[ ]");
        if (ImGui::IsItemHovered() && has_tex)
            ImGui::SetTooltip("tex id: %d", tex_id);
        ImGui::SameLine();

        float avail = ImGui::GetContentRegionAvail().x;
        ImGui::SetNextItemWidth(avail - 90);
        std::string uid = std::string("##tex") + label + std::to_string(selected_mat);
        ImGui::InputText(uid.c_str(), buf, sizeof(buf));
        ImGui::SameLine();

        if (ImGui::Button(("Load##" + std::string(label)).c_str(), ImVec2(50, 0))) {
            std::string path(buf);
            int result = -1;
            if (type == 0) result = rs.tex_manager.load_base(path);
            else if (type == 1) result = rs.tex_manager.load_normal(path);
            else result = rs.tex_manager.load_specular(path);
            if (result >= 0) {
                loaded[selected_mat] = true;
                if (type == 0) m.tex.x = result;
                else if (type == 1) m.tex.y = result;
                else m.tex.z = result;
                status_msg = std::string(label) + " loaded (id " + std::to_string(result) + ")";
            } else {
                status_msg = "err: " + path;
            }
        }
        ImGui::SameLine();

        if (has_tex) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.3f, 0.3f, 1.f));
            if (ImGui::Button(("X##clr" + std::string(label)).c_str(), ImVec2(22, 0))) {
                buf[0] = '\0';
                loaded.erase(selected_mat);
                if (type == 0) m.tex.x = -1;
                else if (type == 1) m.tex.y = -1;
                else m.tex.z = -1;
                status_msg = std::string(label) + " texture removed.";
            }
            ImGui::PopStyleColor();
        } else {
            ImGui::Dummy(ImVec2(22, 0));
        }

        ImGui::SameLine();
        ImGui::TextDisabled("%s", label);
    }
    Scene& scene;
    RenderScene& rs;
    Renderer& renderer;
    char matid_buf[32]{};
    std::string status_msg;
    bool status = true;
    std::unordered_map<int, std::string> mat_names;
    int selected_mat = -1;
    std::unordered_map<int, TexBuffs> tex_buffs;
    std::unordered_map<int, bool> tex_base_loaded;
    std::unordered_map<int, bool> tex_normal_loaded;
    std::unordered_map<int, bool> tex_spec_loaded;
};

#endif //mat_widget_H
