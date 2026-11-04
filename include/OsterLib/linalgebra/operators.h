#pragma once
#include <type_traits>
#include "LinearObject.h"

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
    
    template<size_t CUR,size_t Last,typename OBJ1,typename OBJ2>
    struct CommonRank;

    template<size_t CUR,size_t Last,size_t... RANKS1,size_t... RANKS2,
            bool V1,bool V2,typename VAL1,typename VAL2,
            template<bool,typename,size_t...> typename LINOBJ1,
            template<bool,typename,size_t...> typename LINOBJ2>
    struct CommonRank<CUR,
                    osterlib::Min<
                    LinearObject<V1,VAL1,RANKS1...>::order_,
                    LinearObject<V2,VAL2,RANKS2...>::order_>::value,
                    LinearObject<V1,VAL1,RANKS1...>,
                    LinearObject<V2,VAL2,RANKS2...>>
    {
        
    };

    template<size_t First1,size_t First2,size_t... RANKS1,size_t... RANKS2,
            bool V1,bool V2,typename VAL1,typename VAL2,
            template<bool,typename,size_t...> typename LINOBJ1,
            template<bool,typename,size_t...> typename LINOBJ2>
    struct CommonRank<First1,First2,LinearObject<V1,VAL1,RANKS1...>,LinearObject<V2,VAL2,RANKS2...>>
    {

    };

    template<bool V1,bool V2,typename VAL1, typename VAL2,std::size_t... R1,std::size_t... R2>
    struct Multiplies<LinearObject<V1,VAL1,R1...>,LinearObject<V2,VAL2,R2...>>{
        const LinearObject<V1,VAL1,R1...>& lo1_;
        const LinearObject<V2,VAL2,R2...>& lo2_;
        Multiplies(const LinearObject<V1,VAL1,R1...>& lo1,
                    const LinearObject<V2,VAL2,R2...>& lo2):
            lo1_(lo1),lo2_(lo2){}
        
        CommonRank<std::decay_t<decltype(lo1_)>,
                std::decay_t<decltype(lo2_)>,> operator()() const noexcept{

        }
    };
    template<typename T1,typename T2>
    struct Divides;
}