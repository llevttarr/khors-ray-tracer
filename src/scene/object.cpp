#include <cmath>
#include "object.h"
namespace obj_util{
    Mesh create_sphere(float radius,uint16_t detail_lvl){
        Mesh res{};
        double pi=std::numbers::pi;

        uint16_t st=detail_lvl*2;
        uint16_t sl=detail_lvl*4;

        res.pos.reserve((sl+1)*(st+1));
        for(size_t i=0;i<=st;++i){
            float phi=pi* (float(i)/float(st));
            float y=radius*std::cos(phi);
            float sinp=std::sin(phi);
            for(size_t j=0;j<=sl;++j){
                float theta=pi*2.0f*(float(j)/float(sl));
                float x=radius*std::cos(theta)*sinp;
                float z=radius*std::sin(theta)*sinp;
                res.pos.push_back(Vec3<float>(x,y,z));
            }
        }
        res.ind.reserve(sl*st*6);

        for (size_t i=0;i<st;++i){
            for(size_t j=0;j<sl;++j){
                uint16_t i0=i*(sl+1)+j;
                uint16_t i1=(i+1)*(sl+1)+j;
                uint16_t i2=(i+1)*(sl+1)+(j+1);
                uint16_t i3=i*(sl+1)+(j+1);

                res.ind.push_back(i0);
                res.ind.push_back(i1);
                res.ind.push_back(i2);
                res.ind.push_back(i0);
                res.ind.push_back(i2);
                res.ind.push_back(i3);
            }
        }

        return res;
    }
}