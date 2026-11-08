#pragma once
#include <type_traits>
#include "LinearObject.h"
#include "OsterLib/linalgebra/Strategy.h"

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
    
    template<typename T1,typename T2,typename STRATEGY,std::size_t... IDS_ORDER>
    struct OperationResult:std::false_type{};

    template<typename STRATEGY,size_t... IDS,size_t... RANKS1,size_t... RANKS2,
            bool V1,bool V2,typename VAL1,typename VAL2>
    struct OperationResult<LinearObject<V1,VAL1,RANKS1...>,
                    LinearObject<V2,VAL2,RANKS2...>,STRATEGY,IDS...>
    {
        static_assert(std::is_base_of_v<Strategy,STRATEGY>);
        using result_t = std::invoke_result_t<STRATEGY,
                        LinearObject<V1,VAL1,RANKS1...>,
                        LinearObject<V2,VAL2,RANKS2...>>;
    };

    template<bool V1,bool V2,typename VAL1, typename VAL2,std::size_t... R1,std::size_t... R2>
    struct Multiplies<LinearObject<V1,VAL1,R1...>,LinearObject<V2,VAL2,R2...>>{
        const LinearObject<V1,VAL1,R1...>& lo1_;
        const LinearObject<V2,VAL2,R2...>& lo2_;
        Multiplies(const LinearObject<V1,VAL1,R1...>& lo1,
                    const LinearObject<V2,VAL2,R2...>& lo2):
            lo1_(lo1),lo2_(lo2){}
        
        template<std::size_t... IDS,typename STRATEGY>
        OperationResult<std::decay_t<decltype(lo1_)>,
            std::decay_t<decltype(lo2_)>,
            STRATEGY,
            IDS...> operator()(STRATEGY s) const noexcept
        {
                static_assert(std::is_base_of_v<Strategy,STRATEGY>);
                return s.template operator<IDS...>(lo1_,lo2_);
        }
    };
    template<typename T1,typename T2>
    struct Divides;
}