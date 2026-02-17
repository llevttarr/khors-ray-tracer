//#include <iostream>
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include "./app/window.h"
#include "./render/renderer.h"
#include "./scene/camera.h"

int main(int argc, char* argv[]) {
    int h=720;
    int w=1280;
    Window window(w,h, "Ray tracer");
    Renderer renderer(window.get_w(),window.get_h());  // resize?
    Scene scene{};
    RenderScene r=scene.to_render_scene();
    renderer.update_scene(r);
    EulerCamera camera{w,h};
    while (!window.should_close()){
        renderer.run();
        window.swap_buffers();
        window.poll_events();
    }
    return 0;
}
