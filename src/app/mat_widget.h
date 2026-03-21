#ifndef MAT_WIDGET_H
#define MAT_WIDGET_H
#include "widget.h"
#include "../render/renderer.h"
#include "../scene/scene.h"

class MatWidget : public Widget{
public:
    MatWidget(Scene& s,Renderer&r,RenderScene&rs):scene(s),renderer(r),r(rs){}
    void draw() override{

        ImGui::Begin("Materials");
        ImGui::Text("Materials");
        if (ImGui::Button("random materials")) {
            scene.gen_random_mats(5,0,0,0);
            r.mat_v = std::move(scene.get_mats());
            renderer.update_mats(r);
        }
        ImGui::End();
    }
private:
    Scene& scene;
    RenderScene& r;
    Renderer& renderer;
};

#endif //mat_widget_H
