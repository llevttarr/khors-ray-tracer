#pragma once
#include <cmath>
#include <array>

template<typename T>
class Mat3 {
public:
    constexpr Mat3(){
        for (int i=0;i<9;i+=4){
            dat.at(i)=T{1};
        }
    };
    static constexpr Mat3 zero_mat(){
        Mat3 res;
        res.dat.fill(T{0});
        return res;
    }
    constexpr explicit Mat3(const std::array<T,9>& arr) : dat(arr) {}
    constexpr const T* data() const noexcept { return dat.data(); }
    constexpr T* data() noexcept { return dat.data(); }

    constexpr T& at(int i, int j) { return dat[i*3 + j]; }
    constexpr T  at(int i, int j) const { return dat[i*3+ j]; }
    static constexpr Mat3 matmul(const Mat3& a, const Mat3& b){
        Mat3 res=zero_mat();
        for(int i=0;i<3;++i){
            for(int j=0;j<3;++j){
                T tmp= T{};
                for(int k=0;k<3;++k){
                    tmp+=a.at(i,k)*b.at(k,j);
                }
                res.at(i,j)=tmp;
            }
        }
        return res;
    }
private:
    std::array<T,9> dat{};
};
