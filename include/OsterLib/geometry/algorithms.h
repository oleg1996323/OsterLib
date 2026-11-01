#pragma once
#include "OsterLib/geometry/BasicGeometry.h"
#include "OsterLib/linalgebra/matrix.h"

namespace geometry{
    template<uint64_t DIMENSIONS1,typename VALUE1_T,
        uint64_t DIMENSIONS2,typename VALUE2_T,
        uint64_t DIMENSIONS3,typename VALUE3_T>
    long double cross(const BasicPoint<DIMENSIONS1,VALUE1_T>& a,
            const BasicPoint<DIMENSIONS2,VALUE2_T>& b, 
            const BasicPoint<DIMENSIONS3,VALUE3_T>& c);
    template<uint64_t DIMENSIONS1,typename VALUE1_T,
        uint64_t DIMENSIONS2,typename VALUE2_T,
        uint64_t DIMENSIONS3,typename VALUE3_T>
    bool on_segment(const BasicPoint<DIMENSIONS1,VALUE1_T>& a,
            const BasicPoint<DIMENSIONS2,VALUE2_T>& b, 
            const BasicPoint<DIMENSIONS3,VALUE3_T>& c);
    template<uint64_t DIMENSIONS1,typename VALUE1_T,
        uint64_t DIMENSIONS2,typename VALUE2_T>
    bool on_segment(const BasicPoint<DIMENSIONS1,VALUE1_T>& seg,
            const BasicPoint<DIMENSIONS2,VALUE2_T>& point);
    template<uint64_t DIMENSIONS1,typename VALUE1_T,
        uint64_t DIMENSIONS2,typename VALUE2_T>
    long double pythagoras(const BasicLine<DIMENSIONS1,VALUE1_T>& first,
                        const BasicLine<DIMENSIONS2,VALUE2_T>& second) noexcept;
    template<uint64_t DIMENSIONS1,typename VALUE1_T,
        uint64_t DIMENSIONS2,typename VALUE2_T>
    long double pythagoras2(const BasicLine<DIMENSIONS1,VALUE1_T>& first,
                        const BasicLine<DIMENSIONS2,VALUE2_T>& second) noexcept;

    template<uint64_t DIMENSIONS1,typename VALUE1_T,
        uint64_t DIMENSIONS2,typename VALUE2_T,
        uint64_t DIMENSIONS3,typename VALUE3_T>
    long double cross(const BasicPoint<DIMENSIONS1,VALUE1_T>& a,
            const BasicPoint<DIMENSIONS2,VALUE2_T>& b, 
            const BasicPoint<DIMENSIONS3,VALUE3_T>& c) {
        using value_result_t = std::common_type_t<
            std::common_type_t<VALUE1_T,VALUE2_T>,VALUE3_T>;
        using max_prec_t = std::common_type_t<value_result_t,long double>;
        return (static_cast<max_prec_t>(exec<Get,0>(b)) - 
                static_cast<max_prec_t>(exec<Get,0>(a))) * 
                (static_cast<max_prec_t>(exec<Get,1>(c)) - 
                static_cast<max_prec_t>(exec<Get,1>(a)))
            - (static_cast<max_prec_t>(exec<Get,1>(b)) - 
                static_cast<max_prec_t>(exec<Get,1>(a))) * 
                (static_cast<max_prec_t>(exec<Get,0>(c)) - 
                static_cast<max_prec_t>(exec<Get,0>(a)));
    }

    template<uint64_t DIMENSIONS1,typename VALUE1_T,
        uint64_t DIMENSIONS2,typename VALUE2_T,
        uint64_t DIMENSIONS3,typename VALUE3_T>
    bool on_segment(const BasicPoint<DIMENSIONS1,VALUE1_T>& first,
                    const BasicPoint<DIMENSIONS2,VALUE2_T>& second,
                    const BasicPoint<DIMENSIONS3,VALUE3_T>& to_check) 
    {
        using value_result_t = std::common_type_t<VALUE1_T,VALUE2_T>;
        using max_prec_t = std::common_type_t<value_result_t,long double>;
        static_assert(DIMENSIONS1==DIMENSIONS2 && DIMENSIONS1==DIMENSIONS3,
                "Please bring the points to the same dimension >=2");
        return [&]<std::size_t... Is>(std::index_sequence<Is...>){
            auto check = [&](auto ic){
                constexpr std::size_t I = decltype(ic)::value;
                return std::min(exec<Get,I>(first),
                        exec<Get,I>(second)) <= exec<Get,I>(to_check) && 
                        exec<Get,I>(to_check) <= 
                        std::max(exec<Get,I>(first), exec<Get,I>(second));
            };
            return ((check(std::integral_constant<std::size_t,Is>{})) && ... && true);
        }(std::make_index_sequence<DIMENSIONS1>());
    }

    template<uint64_t DIMENSIONS1,typename VALUE1_T,
        uint64_t DIMENSIONS2,typename VALUE2_T>
    long double pythagoras2(
            const BasicPoint<DIMENSIONS1,VALUE1_T>& first,
            const BasicPoint<DIMENSIONS2,VALUE2_T>& second) noexcept{
        static_assert(DIMENSIONS1==DIMENSIONS2,"Please bring the points to the same dimension >=2");
        return [&]<std::size_t... Is>(std::index_sequence<Is...>){
            auto delta2 = [&](auto ic){
                constexpr std::size_t I = decltype(ic)::value;
                long double delta = static_cast<long double>(exec<Get,I>(first))-
                                    static_cast<long double>(exec<Get,I>(second));
                return delta*delta;
            };
            return ((delta2(std::integral_constant<std::size_t,Is>{}))+...+0.0L);
        }(std::make_index_sequence<DIMENSIONS1>());
    }
    template<uint64_t DIMENSIONS1,typename VALUE1_T,
        uint64_t DIMENSIONS2,typename VALUE2_T>
    long double pythagoras(
            const BasicPoint<DIMENSIONS1,VALUE1_T>& first,
            const BasicPoint<DIMENSIONS2,VALUE2_T>& second) noexcept{
        return std::sqrtl(pythagoras2(first,second));
    }
}