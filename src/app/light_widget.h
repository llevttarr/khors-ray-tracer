#ifndef LIGHT_WIDGET_H
#define LIGHT_WIDGET_H

#include "widget.h"
#include "gl_renderer.h"
#include "scene.h"
#include "light.h"
// #include "../scene/scene_parser.h"

class LightWidget : public Widget{
public:
    LightWidget(Scene& s,GLRenderer&r,RenderScene&rs) : scene(s),renderer(r),rs(rs){}
    void draw() override{

        ImGui::Begin("Light");
        ImGui::Text("Light management");
        ImGui::BeginChild("LightList", ImVec2(180, 0), true);
        ImGui::SeparatorText("Scene Lights");

        const auto& lights = rs.light_v;

        for (int i = 0; i < lights.size(); i++) {
            const auto& l = lights[i];

            ImVec4 col(l.diffuse.x, l.diffuse.y, l.diffuse.z, 1.0f);
            ImGui::ColorButton(("##lsw" + std::to_string(i)).c_str(), col,
                ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoBorder,
                ImVec2(17,17));
            ImGui::SameLine();

            std::string label = "Light " + std::to_string(i);
            if (ImGui::Selectable(label.c_str(), selected_light == i)) {
                selected_light = i;
            }
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::Button("+ Add", ImVec2(-1, 0))) {
            Light l{};
            l.pos = {0.f, 5.f, 0.f, 0.f};
            l.diffuse = {10.f, 10.f, 10.f, 0.f};
            l.dir_type = {0.f, -1.f, 0.f, (float)LIGHT_POINT};
            l.params1 = {0.f, 0.9f, 1.f, 1.f};
            uint32_t id = scene.add_light(l);
            rs.light_v = scene.get_lights();
            selected_light = (int)id;
        }

        if (!rs.light_v.empty()) {
            if (ImGui::Button("Delete", ImVec2(-1, 0))) {
                status_msg = "TODO delete";
            }
        }

        ImGui::EndChild();

        ImGui::SameLine();

        ImGui::BeginChild("EditLight", ImVec2(0, 0), false);

        if (selected_light >= 0 && selected_light < (int)rs.light_v.size()) {

            Light& l = rs.light_v[selected_light];
            ImGui::SeparatorText("Type");

            const char* types[] = {
                "Point", "Spot", "Area", "Directional", "Triangle"
            };

            int type = light_util::get_type(l);
            if (ImGui::Combo("Light Type", &type, types, IM_ARRAYSIZE(types))) {
                light_util::set_type(l, type);
            }

            ImGui::SeparatorText("Common");
            draw_vec4_rgb("Diffuse", l.diffuse);

            if (type == LIGHT_POINT) {
                ImGui::SeparatorText("Point");
                draw_vec4_xyz("Position", l.pos);
            }

            else if (type == LIGHT_DIRECTION) {
                ImGui::SeparatorText("Directional");
                Vec3<float> dir = light_util::get_dir(l);
                if (draw_vec3_edit("Direction", dir)) {
                    light_util::set_dir(l, dir);
                }

                if (ImGui::Button("Normalize Dir")) {
                    Vec3<float> norm_dir = Vec3<float>::normalize(light_util::get_dir(l));
                    light_util::set_dir(l, norm_dir);
                }
            }

            else if (type == LIGHT_SPOT) {
                ImGui::SeparatorText("Spot");

                draw_vec4_xyz("Position", l.pos);
                
                Vec3<float> dir = light_util::get_dir(l);
                if (draw_vec3_edit("Direction", dir)) {
                    light_util::set_dir(l, dir);
                }
                
                ImGui::DragFloat("cos_outer", &light_util::cos_outer(l), 0.01f, 0.0f, 1.0f);
                ImGui::DragFloat("Range", &light_util::range(l), 0.5f, 0.0f, 100.0f);

                if (ImGui::Button("Normalize Dir##spot")) {
                    Vec3<float> norm_dir = Vec3<float>::normalize(light_util::get_dir(l));
                    light_util::set_dir(l, norm_dir);
                }
            }

            else if (type == LIGHT_AREA) {
                ImGui::SeparatorText("Area");

                draw_vec4_xyz("Position", l.pos);
                
                Vec3<float> dir = light_util::get_dir(l);
                if (draw_vec3_edit("Direction", dir)) {
                    light_util::set_dir(l, dir);
                }

                draw_vec4_xyz("Tangent", l.tangent);
                draw_vec4_xyz("Bitangent", l.bitangent);
                
                ImGui::DragFloat("Half Width", &light_util::half_w(l), 0.1f, 0.01f, 100.f);
                ImGui::DragFloat("Half Height", &light_util::half_h(l), 0.1f, 0.01f, 100.f);

                if (ImGui::Button("Orthonormalize")) {
                    Vec3<float> d = light_util::get_dir(l);
                    d = Vec3<float>::normalize(d);
                    light_util::set_dir(l, d);
                    
                    Vec3<float> tang = {l.tangent.x, l.tangent.y, l.tangent.z};
                    Vec3<float> bitang = {l.bitangent.x, l.bitangent.y, l.bitangent.z};
                    
                    tang = Vec3<float>::normalize(tang);
                    bitang = Vec3<float>::normalize(Vec3<float>::cross(d, tang));
                    tang = Vec3<float>::normalize(Vec3<float>::cross(bitang, d));
                    
                    l.tangent = {tang.x, tang.y, tang.z, 0.f};
                    l.bitangent = {bitang.x, bitang.y, bitang.z, 0.f};
                }
            }
            
            else if (type == LIGHT_TRIANGLE) {
                ImGui::SeparatorText("Triangle Light");
                draw_vec4_xyz("Position", l.pos);
            }
            
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            if (ImGui::Button("Apply", ImVec2(120, 0))) {
                scene.change_light(l, selected_light);
                renderer.update_lights(rs);
                status_msg = "Light applied";
                status = true;
            }

            ImGui::SameLine();

            if (ImGui::Button("Reset", ImVec2(80, 0))) {
                rs.light_v = scene.get_lights();
                status_msg = "Reset";
                status = true;
            }

        } else {
            ImGui::TextDisabled("No light selected.");
        }

        if (!status_msg.empty()) {
            ImGui::Spacing();
            ImGui::TextColored(status ? ImVec4(0,1,0,1) : ImVec4(1,0.3f,0.3f,1),"%s", status_msg.c_str());
        }
        ImGui::EndChild();
        ImGui::End();
    }
private:
    bool draw_vec3_edit(const char* label, Vec3<float>& v) {
        float arr[3] = {v.x, v.y, v.z};
        if (ImGui::DragFloat3(label, arr, 0.1f)) {
            v = {arr[0], arr[1], arr[2]};
            return true;
        }
        return false;
    }
    void draw_vec3(const char* label, Vec3<float>& v) {
        float arr[3] = {v.x, v.y, v.z};
        if (ImGui::DragFloat3(label, arr, 0.1f)) {
            v = {arr[0], arr[1], arr[2]};
        }
    }
    void draw_vec4_xyz(const char* label, Vec4<float>& v) {
        float arr[3] = {v.x, v.y, v.z};
        if (ImGui::DragFloat3(label, arr, 0.1f)) {
            v.x = arr[0];
            v.y = arr[1];
            v.z = arr[2];
        }
    }
    
    void draw_vec4_rgb(const char* label, Vec4<float>& v) {
        float arr[3] = {v.x, v.y, v.z};
        if (ImGui::DragFloat3(label, arr, 0.1f)) {
            v.x = arr[0];
            v.y = arr[1];
            v.z = arr[2];
        }
    }
    
    Scene& scene;
    RenderScene& rs;
    GLRenderer& renderer;
    int selected_light = -1;
    std::string status_msg;
    bool status = true;
};

#endif
