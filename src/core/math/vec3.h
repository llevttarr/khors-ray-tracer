#pragma once
#include <cmath>

template<typename T>
class Vec3 {
public:
    T x,y,z;

    constexpr Vec3(): x(0),y(0),z(0){}
    constexpr Vec3(T x,T y,T z): x(x),y(y), z(z){}

    constexpr Vec3 operator+(const Vec3& other) const {
        return Vec3(x +other.x, y+other.y,z+other.z);
    }
    constexpr Vec3 operator-(const Vec3& other) const {
        return Vec3(x -other.x, y-other.y,z-other.z);
    }
    constexpr Vec3 operator*(T s) const {
        return Vec3(s*x,s*y,s*z);
    }
    constexpr bool operator==(const Vec3& other) const{
        return x==other.x && y==other.y && z==other.z;
    }
    Vec3& operator+=(const Vec3& other) {
        x+= other.x;
        y+= other.y;
        z+= other.z;
        return *this;
    }
    Vec3& operator-=(const Vec3& other) {
        x-= other.x;
        y-= other.y;
        z-= other.z;
        return *this;
    }
    Vec3& operator*=(T s) {
        x*=s;
        y*=s;
        z*=s;
        return *this;
    }
    constexpr T magnitude_sq() const {
        return x*x+y*y+z*z;
    }
    static constexpr T dot(const Vec3& a,const Vec3& b) {
        return a.x*b.x+ a.y*b.y+a.z*b.z;
    }
    static constexpr Vec3 cross(const Vec3& a,const Vec3& b) {
        return Vec3(
            a.y*b.z-a.z*b.y,
            a.z*b.x-a.x*b.z,
            a.x*b.y-a.y*b.x
        );
    }
    static constexpr Vec3 normalize(const Vec3& a){
        float mag=std::sqrt(a.magnitude_sq());
        if (mag<=0.0){
            return a;
        }
        return Vec3(
            a.x/mag,
            a.y/mag,
            a.z/mag
        );
    }
};
