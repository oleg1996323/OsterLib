#pragma once
#include "OsterLib/linalgebra/definitions.h"


namespace linalgebra{
    struct Strategy{};

    struct ElementMultiplies:Strategy{
        template<bool V1,bool V2,typename VAL1,typename VAL2,
            std::size_t... RANKS1,std::size_t... RANKS2,std::size_t... >
        auto operator()(const LinearObject<V1,VAL1,RANKS1...>&,const LinearObject<V2,VAL2,RANKS2...>&){

        }
    };
    struct CrossMultiplies:Strategy{};
    struct OuterProduct:Strategy{
        template<bool V1,bool V2,typename VAL1,typename VAL2,
            std::size_t... RANKS1,std::size_t... RANKS2>
        auto operator()(const LinearObject<V1,VAL1,RANKS1...>& lo1,const LinearObject<V2,VAL2,RANKS2...>& lo2) const{
            using result_t = LinearObject<false,std::common_type_t<VAL1,VAL2>,RANKS1...,RANKS2...>;
            constexpr std::size_t total_1 = (RANKS1 * ... * 1);
            constexpr std::size_t total_2 = (RANKS2 * ... * 1);
            constexpr std::size_t total_R = total_1 * total_2;
            result_t result{};
            for(std::size_t i=0;i<total_R;++i)
                result.data()[i] =
                    lo1.data()[i / total_2]
                    * lo2.data()[i % total_2];
            return result;
        }
    };
    
    template<std::size_t... IDS>
    struct Contraction:Strategy{
        template<bool VIEW,typename VALUE_T,std::size_t... RANKS>
        auto operator()(const LinearObject<VIEW,VALUE_T,RANKS...>& lo) const{
            static_assert(sizeof...(IDS)>0);
            using matched = typename osterlib::ExcludedIdx<std::integer_sequence<std::size_t,RANKS...>,
                    std::integer_sequence<std::size_t,IDS...>>;
            using result_t = FromIntegerSequence::LinearObject<false,VALUE_T,matched>;
            constexpr std::size_t total = osterlib::Product<matched>::value;
            result_t result{};
            std::array<std::size_t,sizeof...(RANKS)> iterators;
            iterators.fill(0);
            constexpr std::array<std::size_t,sizeof...(RANKS)> upper_bounds{RANKS...};
            constexpr std::array<std::size_t,sizeof...(IDS)> ids{IDS...};
            VALUE_T tmp_val = 0;
            for(std::size_t j=0, i=0,m=0;;){
                if(j<ids.front())
                    tmp_val = 0;
                if(iterators[j]==upper_bounds[j]){
                    if(j>0){
                        iterators[j]=0;
                        --j;
                        ++iterators[j];
                        result.data()[m]=tmp_val;
                        ++m; //только когда j==ids.front()
                    }
                    else break;
                }
                else{
                    if(j<sizeof...(RANKS)-1)
                        ++j;
                    else{
                        tmp_val+=lo.data()[i];
                        ++iterators[j];
                        ++i;
                    }
                    continue;
                }
            }
            return result;
        }
    };
    struct ScalarMultiplies:Strategy{};
}