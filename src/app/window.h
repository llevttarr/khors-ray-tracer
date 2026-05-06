#ifndef WINDOW_H
#define WINDOW_H

#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <string>
#include "app_util.h"

// TODO: give config to constructor instead 
class Window {
public:
    Window(const std::string& title,ProgramState& s,bool using_vsync);
    ~Window();
    void destroy();

    bool should_close() const;
    void poll_events();
    void swap_buffers();
    void load_icon();
    
    int get_w();
    int get_h();

    GLFWwindow* get_glfw_window() {return glfw_window;}
    static void mouse_callback(GLFWwindow* w,double x,double y);
    static void err_callback(int code, const char* desc);
    static void mouse_input_callback(GLFWwindow* w,int button, int action, int mods);
    static void size_callback(GLFWwindow* w,int nw,int nh);
    static void key_callback(GLFWwindow* w,int key,int scancode, int action, int mods);
private:
    GLFWwindow* glfw_window;
    int width;
    int height;
};
#endif //WINDOW_H
