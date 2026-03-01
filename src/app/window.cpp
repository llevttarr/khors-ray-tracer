#include "window.h"
#include <stdexcept>

#include <glad/gl.h> 

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
void Window::mouse_callback(GLFWwindow* w,double x,double y){
    auto* state = static_cast<ProgramState*>(glfwGetWindowUserPointer(w));
    if(!state){
        throw std::runtime_error("mouse fail");
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
Window::Window(int w, int h, const std::string& title,ProgramState& p)
    : width(w), height(h),glfw_window(nullptr)
{
    if (!glfwInit()){
        throw std::runtime_error("glfwInit fail");
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    glfw_window = glfwCreateWindow(w, h,title.c_str(), NULL, NULL);
    if (!glfw_window) {
        glfwTerminate();
        throw std::runtime_error("glfw_window creation fail");
    }
    glfwMakeContextCurrent(glfw_window);
    if (!gladLoaderLoadGL()) {
        glfwDestroyWindow(glfw_window);
        glfwTerminate();
        throw std::runtime_error("glad init fail");
    }
    glfwSwapInterval(1);
    glfwSetWindowUserPointer(glfw_window, &p);
    glfwSetCursorPosCallback(glfw_window,mouse_callback);
    glfwSetWindowSizeCallback(glfw_window,size_callback);
    glfwSetInputMode(glfw_window,GLFW_CURSOR,GLFW_CURSOR_DISABLED);
    
}

Window::~Window() {
    glfwDestroyWindow(glfw_window);
    glfwTerminate();
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