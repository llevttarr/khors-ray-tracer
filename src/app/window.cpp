#include "window.h"
#include "image_util.h"
#include <stdexcept>
#include <iostream>
#include <algorithm>
#include <glad/gl.h> 
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include "backends/imgui_impl_vulkan.h"

void Window::size_callback(GLFWwindow* w,int nw,int nh){
    auto* state = static_cast<ProgramState*>(glfwGetWindowUserPointer(w));
    if(!state){
        throw std::runtime_error("size fail");
    }
    state->w=nw;
    state->h=nh;
    auto camera=state->camera;
    camera->upd_aspect(nw,nh);
}
void Window::key_callback(GLFWwindow* w,int key,int scancode, int action, int mods){
    auto* state = static_cast<ProgramState*>(glfwGetWindowUserPointer(w));
    if(!state){
        throw std::runtime_error("key fail");
    }
    auto& inp = state->active_input;
    auto inp_i=std::find(inp.begin(),inp.end(),key);
    if (action == GLFW_PRESS){
        if (inp_i==inp.end()){
            inp.push_back(key);
        }
    }else if (action==GLFW_RELEASE){
        if (inp_i!=inp.end()){
            inp.erase(inp_i);
        }
    }
    if (key == GLFW_KEY_ESCAPE&& action == GLFW_PRESS){
        bool cursor_locked=state->cursor_locked;
        state->cursor_locked=!cursor_locked;
        if (cursor_locked==true){ // now false
            glfwSetInputMode(w,GLFW_CURSOR,GLFW_CURSOR_NORMAL);
        }else{
            glfwSetInputMode(w,GLFW_CURSOR,GLFW_CURSOR_DISABLED);
        }
    }
}
void Window::mouse_input_callback(GLFWwindow* w,int button, int action, int mods){
    auto* state = static_cast<ProgramState*>(glfwGetWindowUserPointer(w));
    if(!state){
        throw std::runtime_error("mouse input fail");
    }


}
void Window::mouse_callback(GLFWwindow* w,double x,double y){
    auto* state = static_cast<ProgramState*>(glfwGetWindowUserPointer(w));
    if(!state){
        throw std::runtime_error("mouse fail");
    }
    if (!state->cursor_locked){
        return;
    }
    if(state->first_mouse){
        state->last_x=x;
        state->last_y=y;
        state->first_mouse=false;
        return;
    }
    double dx=x-state->last_x;
    double dy=state->last_y-y;
    double sensitivity=0.01f;
    dx*=sensitivity;
    dy*=sensitivity;
    state->last_x=x;
    state->last_y=y;
    auto camera=state->camera;
    camera->set_yaw(camera->get_yaw()+dx);
    camera->set_pitch(camera->get_pitch()+dy);
    camera->upd_dir();
}
Window::Window(const std::string& title,ProgramState& p,bool using_vsync)
    : width(p.w), height(p.h),glfw_window(nullptr)
{
    if (!glfwInit()){
        throw std::runtime_error("glfwInit fail");
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    if (p.using_VK){
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    }else{
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    }

    glfw_window = glfwCreateWindow(p.w, p.h,title.c_str(), NULL, NULL);
    if (!glfw_window) {
        glfwTerminate();
        throw std::runtime_error("glfw_window creation fail");
    }
    if (p.using_VK ){

    }else{
        glfwMakeContextCurrent(glfw_window);
        if (!gladLoaderLoadGL()) {
            glfwDestroyWindow(glfw_window);
            glfwTerminate();
            throw std::runtime_error("glad init fail");
        }
        if (using_vsync){
            glfwSwapInterval(1);
        }else{
            glfwSwapInterval(0);    
        }
    }
    glfwSetWindowUserPointer(glfw_window, &p);
    // glfwSetCursorPosCallback(glfw_window,mouse_callback);
    glfwSetWindowSizeCallback(glfw_window,size_callback);
    glfwSetKeyCallback(glfw_window,key_callback);
    // glfwSetInputMode(glfw_window,GLFW_CURSOR,GLFW_CURSOR_DISABLED);
    glfwSetErrorCallback(err_callback);
    
    load_icon();
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark();
    if (p.using_VK){
        ImGui_ImplGlfw_InitForVulkan(glfw_window, true);
    }else{
        ImGui_ImplGlfw_InitForOpenGL(glfw_window, true);
        ImGui_ImplOpenGL3_Init("#version 460");
    }
}
void Window::err_callback(int code,const char* desc){
    std::cout<<"glfw error: "<<code<<": "<<desc<<std::endl;
}
void Window::load_icon(){
    std::string iconpath="assets/img/logo_khors.png";
    Image icon=image_util::load_image(iconpath,0);
    GLFWimage img;
    img.pixels=icon.data.data();
    img.height=icon.h;
    img.width=icon.w;
    glfwSetWindowIcon(glfw_window,1,&img);
}
Window::~Window() {
    destroy();
}
bool Window::should_close() const{
    return glfwWindowShouldClose(glfw_window);
}
void Window::poll_events(){
    glfwPollEvents();
}
void Window::swap_buffers(){
    glfwSwapBuffers(glfw_window);
}
int Window::get_h(){
    return height;
}
int Window::get_w(){
    return width;
}
void Window::destroy(){
    glfwDestroyWindow(glfw_window);
    glfwTerminate();
}