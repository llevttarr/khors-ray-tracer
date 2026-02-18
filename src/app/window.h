#ifndef WINDOW_H
#define WINDOW_H

#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <string>
#include "../util/app_util.h"

// TODO: give config to constructor instead 
class Window {
public:
    Window(int w,int h,const std::string& title,ProgramState& s);
    ~Window();

    bool should_close() const;
    void poll_events();
    void swap_buffers();

    int get_w();
    int get_h();

    GLFWwindow* get_glfw_window() {return glfw_window;}
    static void mouse_callback(GLFWwindow* w,double x,double y);
    static void size_callback(GLFWwindow* w,double nw,double nh);
private:
    GLFWwindow* glfw_window;
    int width;
    int height;
};
#endif //WINDOW_H
