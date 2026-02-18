#include "window.h"
#include <stdexcept>

#include <glad/gl.h> 

Window::Window(int w, int h, const std::string& title)
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