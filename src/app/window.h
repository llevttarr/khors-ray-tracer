#ifndef WINDOW_H
#define WINDOW_H

#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <string>

// TODO: give config to constructor instead 
class Window {
public:
    Window(int w,int h,const std::string& title);
    ~Window();

    bool should_close() const;
    void poll_events();
    void swap_buffers();

    int get_w();
    int get_h();

private:
    GLFWwindow* glfw_window;
    int width;
    int height;
    void size_callback(); // TODO
};
#endif //WINDOW_H
