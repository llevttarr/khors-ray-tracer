#ifndef VEC4_H
#define VEC4_H
#include <cmath>
#include "./mat4.h"
#include "./vec3.h"

template<typename T>
class Vec4 {
public:
    T x,y,z,w;

    constexpr Vec4(): x(0),y(0),z(0),w(0){}
    constexpr Vec4(T x,T y,T z,T w): x(x),y(y), z(z),w(w){}

    constexpr Vec4 operator+(const Vec4& other) const {
        return Vec4(x +other.x, y+other.y,z+other.z,w+other.w);
    }
    constexpr Vec4 operator-(const Vec4& other) const {
        return Vec4(x -other.x, y-other.y,z-other.z,w-other.w);
    }
    constexpr Vec4 operator*(T s) const {
        return Vec4(s*x,s*y,s*z,s*w);
    }
    constexpr bool operator==(const Vec4& other) const{
        return x==other.x && y==other.y && z==other.z && w==other.w;
    }
    Vec4& operator+=(const Vec4& other) {
        x+= other.x;
        y+= other.y;
        z+= other.z;
        w+= other.w;
        return *this;
    }
    Vec4& operator-=(const Vec4& other) {
        x-= other.x;
        y-= other.y;
        z-= other.z;
        w-=other.w;
        return *this;
    }
    Vec4& operator*=(T s) {
        x*=s;
        y*=s;
        z*=s;
        w*=s;
        return *this;
    }
    constexpr T magnitude_sq() const {
        return x*x+y*y+z*z+w*w;
    }
    static constexpr T dot(const Vec4& a,const Vec4& b) {
        return a.x*b.x+ a.y*b.y+a.z*b.z+a.w*b.w;
    }
    static constexpr Vec4 matvec_mul(const Mat4<T>& a, const Vec4& b){
        Vec4 res{};
        res.x=a.at(0,0)*b.x + a.at(0,1)*b.y+ a.at(0,2)*b.z+a.at(0,3)*b.w;
        res.y=a.at(1,0)*b.x + a.at(1,1)*b.y+ a.at(1,2)*b.z+a.at(1,3)*b.w;
        res.z=a.at(2,0)*b.x + a.at(2,1)*b.y+ a.at(2,2)*b.z+a.at(2,3)*b.w;
        res.w=a.at(3,0)*b.x + a.at(3,1)*b.y+ a.at(3,2)*b.z+a.at(3,3)*b.w;
        return res;
    }
    static Vec4 from_v3(const Vec3<T>& v){
        return Vec4{v.x,v.y,v.z,1.0};
    }
};
#endif //VEC4
