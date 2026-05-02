#ifndef CAMERA_H
#define CAMERA_H

#include <cstdint>
#include <cmath>
#include <chrono>
// #include <numbers>
#include "vec3.h"
#include "mat4.h"

constexpr uint16_t MOVE_TICKS_CAP = 100;
constexpr float SPEED_BASE = 0.5;
struct CameraSnapshot {
    Vec3<float> pos;
    Vec3<float> forward;
    Vec3<float> right;
    Vec3<float> up;
    float fov;
    float aspect;
};
class EulerCamera {
public:
    EulerCamera(int wi,int hi);
    // Mat4<float> get_proj();
    // Mat4<float> get_view();
    Vec3<float> get_up()const {return up;}
    Vec3<float> get_right() const {return right;}
    Vec3<float> get_forward() const { return forward;}
    Vec3<float> get_pos()const {return pos;}
    void move(uint16_t fwd,uint16_t rght,uint16_t up);
    void upd_dir();
    void set_yaw(float y);
    void set_pitch(float p);
    void set_fov(float f){fov=f;}
    void set_pos(const Vec3<float>& p){pos=p;}
    void upd_aspect(int nw,int nh){w=nw;h=nh;}
    int get_w(){return w;}
    int get_h(){return h;}
    float get_fov() const {return fov;}
    float get_yaw() const {return yaw;}
    float get_pitch() const {return pitch;}
    float aspect()const{return float(w)/float(h);}

private:
    int w;
    int h;
    Vec3<float> pos{0.f, 1.f,0.f};
    Vec3<float> forward{0.f, 0.f, 1.f};
    Vec3<float> right{1.f, 0.f, 0.f};
    Vec3<float> up{0.f, 1.f, 0.f};
    float yaw=0.1f;
    float pitch=0.1f;
    float fov=60.f*3.14159f/180.f;
    float speed=1.0;
    std::chrono::time_point<std::chrono::steady_clock> time_speedup;
    uint16_t move_ticks=0;
    void calculate_move_ticks();
};
#endif //CAMERA_H
