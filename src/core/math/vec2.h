#pragma once
#include <cmath>

template<typename T>
class Vec2 {
public:
    T x,y;

    constexpr Vec2(): x(0),y(0){}
    constexpr Vec2(T x,T y): x(x),y(y){}

    constexpr Vec2 operator+(const Vec2& other) const {
        return Vec2(x +other.x, y+other.y);
    }
    constexpr Vec2 operator-(const Vec2& other) const {
        return Vec2(x -other.x, y-other.y);
    }
    constexpr Vec2 operator*(T s) const {
        return Vec2(s*x,s*y);
    }
    constexpr bool operator==(const Vec2& other) const{
        return x==other.x && y==other.y;
    }
    Vec2& operator+=(const Vec2& other) {
        x+= other.x;
        y+= other.y;
        return *this;
    }
    Vec2& operator-=(const Vec2& other) {
        x-= other.x;
        y-= other.y;
        return *this;
    }
    Vec2& operator*=(T s) {
        x*=s;
        y*=s;
        return *this;
    }
    constexpr T magnitude_sq() const {
        return x*x+y*y;
    }
    static constexpr T dot(const Vec2& a,const Vec2& b) {
        return a.x*b.x+ a.y*b.y;
    }
    static constexpr Vec2 normalize(const Vec2& a){
        float mag=std::sqrt(a.magnitude_sq());
        if (mag<=0.0){
            return a;
        }
        return Vec2(
            a.x/mag,
            a.y/mag
        );
    }
};
