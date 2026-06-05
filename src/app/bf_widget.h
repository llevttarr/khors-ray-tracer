#ifndef BF_WIDGET_H
#define BF_WIDGET_H

#include "widget.h"
#include "vk_renderer.h"
#include "scene.h"
#include "light.h"

class BFWidget : public Widget{
public:
    BFWidget(VKRenderer&r) :renderer(r){}
    void draw() override{

        ImGui::Begin("BloomFog");
        ImGui::Text("Bloom/fog management");
       float* threshold_bl = renderer.get_threshold_bl();
       float* strength_bl = renderer.get_strength_bl();
       float* fog_density = renderer.get_fog_density();
       float* fog_asymmetry = renderer.get_fog_asymmetry();
       float* fog_base = renderer.get_fog_base();
       float* sigma_a = renderer.get_fog_sigma_a();
       float* sigma_s = renderer.get_fog_sigma_s();
       float* height_falloff = renderer.get_fog_height_falloff();
       float* fog_time = renderer.get_fog_time();
        if (ImGui::DragFloat("Threshold##bloom", threshold_bl, 0.01f, 0.f, 1.f, "%.3f")){ /*renderer.upd_bloom_thr(*threshold_bl);*/ }
        if (ImGui::DragFloat("Strength##bloom", strength_bl, 0.01f, 0.f, 1.f, "%.3f")){ /*renderer.upd_bloom_str(*strength_bl);*/ }
        
        ImGui::Separator();
        if (ImGui::DragFloat("Density##fog", fog_density, 0.01f, 0.f, 1.f, "%.3f")) { /*renderer.upd_fog_density(*fog_density);*/ }
        // if (ImGui::DragFloat("Asymmetry##fog", fog_asymmetry, 0.01f, -1.f, 1.f, "%.3f")){ /*renderer.upd_fog_asymmetry(*fog_asymmetry);*/ }
        if (ImGui::DragFloat("Base##fog", fog_base, 0.01f, 0.f, 100.f, "%.3f")){ /*renderer.upd_fog_base(*fog_base);*/ }
        if (ImGui::DragFloat("Sigma_a##fog", sigma_a, 0.01f, 0.f, 1.f, "%.3f")){ /*renderer.upd_fog_base(*fog_base);*/ }
        if (ImGui::DragFloat("Sigma_s##fog", sigma_s, 0.01f, 0.f, 1.f, "%.3f")){ /*renderer.upd_fog_base(*fog_base);*/ }
        if (ImGui::DragFloat("Height falloff##fog", height_falloff, 0.01f, 0.f, 1.f, "%.3f")){ /*renderer.upd_fog_base(*fog_base);*/ }
        if (ImGui::DragFloat("Fog_time##fog", fog_time, 0.1f, 0.f, 100.f, "%.3f")){ /*renderer.upd_fog_base(*fog_base);*/ }


        ImGui::End();
    }
private:
    VKRenderer& renderer;
    std::string status_msg;
    bool status = true;
};

#endif
