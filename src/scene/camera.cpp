#include "camera.h"

EulerCamera::EulerCamera(int wi, int hi){
    w=wi;
    h=hi;
    upd_dir();
    time_speedup=std::chrono::steady_clock::now();
}
void EulerCamera::calculate_move_ticks(){
    auto curr_time=std::chrono::steady_clock::now();
    std::chrono::duration<double> diff=curr_time-time_speedup;
    if (diff.count()>1.0){
        move_ticks=1;
        time_speedup=curr_time;
        return;
    }
    ++move_ticks;
    if (move_ticks>MOVE_TICKS_CAP){
        move_ticks=MOVE_TICKS_CAP;
    }
    time_speedup=curr_time;
    return;
}

void EulerCamera::set_yaw(float y){
    yaw=y;
    const float lim=6.28318f;
    if(yaw>lim){
        yaw-=lim;
    }
    if(yaw<-lim){
        yaw+=lim;
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
void EulerCamera::move(uint16_t fwd,uint16_t rght,uint16_t upw){
    calculate_move_ticks();
    float acc =0.000001*(move_ticks*move_ticks*move_ticks);
    float speed = SPEED_BASE + acc;
    Vec3<float> dir{0.0f,0.0f,0.0f};
    if (fwd==1){
        dir=dir+forward;
        
    }if (fwd==2){
        // backwards
        dir=dir-forward;
    }

    if (rght==1){
        dir=dir+right;
        
    }if (rght==2){
        // left
        dir=dir-right;
    }

    if (upw==1){
        dir = dir+Vec3<float>{0.0,1.0,0.0};
    } if (upw==2){
        dir = dir-Vec3<float>{0.0,1.0,0.0};
    }
    pos={pos.x+dir.x*speed,pos.y+dir.y*speed,pos.z+dir.z*speed};
    
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