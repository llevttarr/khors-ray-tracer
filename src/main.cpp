//#include <iostream>
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include "./app/window.h"
#include "./render/renderer.h"
#include "./scene/camera.h"
#include "./util/app_util.h"

uint16_t get_status_movement(ProgramState& p, int f, int b){
    std::vector<int> inp = p.active_input;
    auto inp_f=std::find(inp.begin(),inp.end(),f);
    auto inp_b=std::find(inp.begin(),inp.end(),b);
    if (inp_f==inp.end()&&inp_b==inp.end()){
        return 0;
    }if (inp_b==inp.end()){
        return 1;
    }
    return 2;
}

int main(int argc, char* argv[]) {
    int h=720;
    int w=1280;
    ProgramState p{};

    EulerCamera camera{w,h};
    bool using_vsync=true;
    p.camera=&camera;
    Window window(w,h, "KHORS",p,using_vsync);
    Renderer renderer(window.get_w(),window.get_h(),camera);  // resize?
    Scene scene{};
    RenderScene r=scene.to_render_scene();
    renderer.update_scene(r);
    while (!window.should_close()){
        window.poll_events();
        uint16_t fwd=get_status_movement(p,GLFW_KEY_W,GLFW_KEY_S);
        uint16_t rght=get_status_movement(p,GLFW_KEY_D,GLFW_KEY_A);
        uint16_t upw=get_status_movement(p,GLFW_KEY_SPACE,GLFW_KEY_LEFT_SHIFT);
        if (fwd!=0||rght!=0||upw!=0){
            camera.move(fwd,rght,upw);
        }
        renderer.run();
        window.swap_buffers();
    }
    return 0;
}
