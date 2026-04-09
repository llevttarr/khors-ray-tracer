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
            
        }
        ImGui::End();
    }
private:
    Scene& scene;
    RenderScene& rs;
    Renderer& renderer;
};

#endif
