#ifndef CAMERA_H
#define CAMERA_H

#include <cstdint>
#include <cmath>
// #include <numbers>
#include "../core/math/vec3.h"
#include "../core/math/mat4.h"

class EulerCamera {
public:
    EulerCamera(int wi,int hi);
    // Mat4<float> get_proj();
    // Mat4<float> get_view();
    Vec3<float> get_up()const {return up;}
    Vec3<float> get_right() const {return right;}
    Vec3<float> get_forward() const { return forward;}
    Vec3<float> get_pos()const {return pos;}
    void move();
    void upd_dir();
    void set_yaw(float y);
    void set_pitch(float p);
    void set_fov(float f){fov=f;}
    void set_pos(const Vec3<float>& p){pos=p;}
    void upd_aspect(int nw,int nh){w=nw;h=nh;}
    float get_fov() const {return fov;}
    float aspect()const{return float(w)/float(h);}

private:
    int w;
    int h;
    Vec3<float> pos{1.f, 1.f, 1.f};
    Vec3<float> forward{0.f, 0.f, 1.f};
    Vec3<float> right{1.f, 0.f, 0.f};
    Vec3<float> up{0.f, 1.f, 0.f};
    float yaw=0.1f;
    float pitch=0.1f;
    float fov=70.f*3.14159f/180.f;
    float speed;
};
#endif //CAMERA_H
