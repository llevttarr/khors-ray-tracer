//#include <iostream>
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include "./app/window.h"
#include "./render/renderer.h"
#include "./scene/camera.h"
#include "./util/app_util.h"

int main(int argc, char* argv[]) {
    int h=720;
    int w=1280;
    ProgramState p{};

    EulerCamera camera{w,h};
    p.camera=&camera;
    Window window(w,h, "Ray tracer",p);
    Renderer renderer(window.get_w(),window.get_h(),camera);  // resize?
    Scene scene{};
    RenderScene r=scene.to_render_scene();
    renderer.update_scene(r);
    while (!window.should_close()){
        renderer.run();
        window.swap_buffers();
        window.poll_events();
    }
    return 0;
}
