#include "vk_device.h"

VKDevice::VKDevice(Window& w){
    GLFWwindow* window = w.get_glfw_window();


    volkInitialize();

}
VKDevice::~VKDevice(){

}
