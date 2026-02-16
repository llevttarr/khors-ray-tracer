#pragma once
#include <cmath>
#include <array>

template<typename T>
class Mat4 {
public:
    constexpr Mat4(){
        for (int i=0;i<16;i+=5){
            dat.at(i)=T{1};
        }
    };
    static constexpr Mat4 zero_mat(){
        std::array<T,16> zer{};
        Mat4 res(&zer);
        return res;
    }
    constexpr explicit Mat4(const std::array<T,16>& arr) : dat(arr) {}
    constexpr const T* data() const noexcept { return dat.data(); }
    constexpr T* data() noexcept { return dat.data(); }

    constexpr T& at(int i, int j) { return dat[i*4 + j]; }
    constexpr T  at(int i, int j) const { return dat[i*4+ j]; }
    static constexpr Mat4 matmul(const Mat4& a, const Mat4& b){
        Mat4 res{};
        for(int i=0;i<4;++i){
            for(int j=0;j<4;++j){
                T tmp= T{};
                for(int k=0;k<4;++k){
                    tmp+=a.at(i,k)*b.at(k,j);
                }
                res.at(i,j)=tmp;
            }
        }
        return res;
    }
    constexpr Mat4 translate(T x,T y,T z) const{
        Mat4 res{};
        res.at(0,3)=x;
        res.at(1,3)=y;
        res.at(2,3)=z;

        return matmul(*this,res);
    }
    constexpr Mat4 scale(T x,T y,T z)const{
        Mat4 res{};
        res.at(0,0)=x;
        res.at(1,1)=y;
        res.at(2,2)=z;

        return matmul(*this,res);
    }
private:
    std::array<T,16> dat{};
};
