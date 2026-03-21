#ifndef WIDGET_H
#define WIDGET_H
#include <vector>
#include <string>
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
class Widget{
public:
    virtual ~Widget()=default;
    virtual void draw()=0;
};
#endif
