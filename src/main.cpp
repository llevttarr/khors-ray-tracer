//#include <iostream>
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include "./app/window.h"
#include "./render/renderer.h"

int main(int argc, char* argv[]) {
    Window window(1280, 720, "Ray tracer");
    Renderer renderer(window.get_w(),window.get_h());  // resize?
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
