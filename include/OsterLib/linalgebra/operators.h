#pragma once
#include <type_traits>

namespace linalgebra{
    template<typename T1,typename T2>
    struct Plus;
    template<typename T1,typename T2>
    struct Minus;
    template<typename T1,typename T2>
    struct Multiplies;
    template<typename T1,typename T2>
    struct Divides;

    template<typename T1,typename T2>
    auto operator+(const T1&,const T2&);
    template<typename T1,typename T2>
    auto operator-(const T1&,const T2&);
    template<typename T1,typename T2>
    auto operator*(const T1&,const T2&);
    template<typename T1,typename T2>
    auto operator/(const T1&,const T2&);
}