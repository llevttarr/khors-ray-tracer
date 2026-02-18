#ifndef APP_UTIL_H
#define APP_UTIL_H
#include "../scene/camera.h"

struct ProgramState{
    int w=1280;
    int h=720;
    EulerCamera* camera=nullptr;
    bool first_mouse=true;
    double last_x = 0.0;
    double last_y= 0.0;
};

#endif // APP_UTIL_H