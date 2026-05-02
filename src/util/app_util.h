#ifndef APP_UTIL_H
#define APP_UTIL_H
#include "camera.h"
#include <vector>

struct ProgramState{
    int w=1280;
    int h=720;
    EulerCamera* camera=nullptr;
    bool first_mouse=true;
    bool cursor_locked= true;
    double last_x = 0.0;
    double last_y= 0.0;
    std::vector<int> active_input{};
};

#endif // APP_UTIL_H