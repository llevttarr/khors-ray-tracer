#include "camera.h"

EulerCamera::EulerCamera(int wi, int hi){
    w=wi;
    h=hi;
    upd_dir();
}

void EulerCamera::set_yaw(float y){
    yaw=y;
    if(yaw>360){
        yaw=360-yaw;
    }
    if(yaw<-360){
        yaw=yaw+360;
    }
}
void EulerCamera::set_pitch(float p){
    pitch=p;
    const float lim=1.55f;
    if(pitch>lim){
        pitch=lim;
    } 
    if (pitch<-lim){
        pitch=-lim;
    }
}
void EulerCamera::upd_dir(){
    float sin_y=std::sin(yaw);
    float cos_y=std::cos(yaw);
    float sin_p=std::sin(pitch);
    float cos_p=std::cos(pitch);

    forward = Vec3<float>{cos_p*cos_y,sin_p,cos_p*sin_y};
    forward=Vec3<float>::normalize(forward);
    const Vec3<float> u{0.f,1.f,0.f};

    right=Vec3<float>::cross(forward,u);
    right=Vec3<float>::normalize(right);
    
    up=Vec3<float>::cross(right,forward);

}