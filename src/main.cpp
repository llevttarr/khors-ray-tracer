//#include <iostream>
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
// #include "backends/imgui_impl_opengl3.h"
#include "backends/imgui_impl_vulkan.h"
#include <glad/gl.h>
#include <volk.h>
#include <GLFW/glfw3.h>
#include <shaderc/shaderc.hpp>

#include "vk_device.h"
#include "vk_cmanager.h"
#include "vk_swapchain.h"
#include "vk_renderer.h"
#include "vk_shaderc.h"

#include "window.h"
#include "ui_manager.h"
#include "widget.h"
#include "mat_widget.h"
#include "obj_widget.h"
#include "light_widget.h"
#include "scene_widget.h"
#include "renderer.h"
#include "camera.h"
#include "light.h"
#include "scene_parser.h"
#include "app_util.h"

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
void glfwErrorCallback(int error, const char* description) {
    std::cerr << "GLFW Error (" << error << "): " << description << std::endl;
}
/*
void run_GL(){

    EulerCamera camera{w,h};
    bool using_vsync=true;
    p.camera=&camera;
    Window window(w,h, "KHORS",p,using_vsync);
    Renderer renderer(window.get_w(),window.get_h(),camera);
    Scene scene{};
    RenderScene r=scene.to_render_scene();
    SceneParser sp{r};
    UIManager uim;
    uim.add_widget(std::make_unique<MatWidget>(scene,renderer,r));
    uim.add_widget(std::make_unique<SceneWidget>(scene,renderer,r,sp));
    uim.add_widget(std::make_unique<ObjWidget>(scene,renderer,r));
    uim.add_widget(std::make_unique<LightWidget>(scene,renderer,r));
    renderer.update_scene(r);
    renderer.cout_data();
    while (!window.should_close()){
        window.poll_events();
        uint16_t fwd=get_status_movement(p,GLFW_KEY_W,GLFW_KEY_S);
        uint16_t rght=get_status_movement(p,GLFW_KEY_D,GLFW_KEY_A);
        uint16_t upw=get_status_movement(p,GLFW_KEY_SPACE,GLFW_KEY_LEFT_SHIFT);
        if (fwd!=0||rght!=0||upw!=0){
            camera.move(fwd,rght,upw);
        }
        renderer.run();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        uim.draw();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        window.swap_buffers();
    }
}
*/
void compile_shaders_VK() {
    VKShaderCompiler compiler;

    compiler.set_optimization(shaderc_optimization_level_performance);
    compiler.set_generate_debug_info(true);

    // compiler.add_include_dir("assets/shaders/include");

    compiler.compile_dir("assets/shaders", true, false);
}
void run_VK(ProgramState& ps){
    
    // TODO: ImGui

    compile_shaders_VK();
    
    Window window("KHORS",ps,true);
    EulerCamera camera{ps.w,ps.h};
    auto vk_device = std::make_shared<VKDevice>(window);
    auto vk_swapchain= std::make_shared<VKSwapchain>(vk_device, ps);
    auto vk_cmanager = std::make_unique<VKCmanager>(vk_device, vk_swapchain);

    VKRenderer renderer(vk_device, vk_swapchain,std::move(vk_cmanager), camera);

    Scene scene{};
    RenderScene r = scene.to_render_scene();
    SceneParser sp{r};
    renderer.update_scene(r);
    while (!window.should_close()){
        window.poll_events();
        uint16_t fwd=get_status_movement(ps,GLFW_KEY_W,GLFW_KEY_S);
        uint16_t rght=get_status_movement(ps,GLFW_KEY_D,GLFW_KEY_A);
        uint16_t upw=get_status_movement(ps,GLFW_KEY_SPACE,GLFW_KEY_LEFT_SHIFT);
        if (fwd!=0||rght!=0||upw!=0){
            camera.move(fwd,rght,upw);
        }
        renderer.run_rs();
    }
    renderer.wait_idle();
    window.destroy();
}
int main(int argc, char* argv[]) {
    ProgramState p{};
    glfwSetErrorCallback(glfwErrorCallback);
    run_VK(p);
    return 0;
}
