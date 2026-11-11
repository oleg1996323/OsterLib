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
            // constexpr std::size_t cycle_offset = [&lo](){
                std::array<std::size_t,sizeof...(IDS)> ids{IDS...};
                std::size_t cycle_offset = 1;
                std::size_t min_free_id = 0;
                //define the minimal free axis-order
                for(std::size_t i=0;i<lo.order();++i){
                    bool matched = false;
                    for(std::size_t j=0;j<sizeof...(IDS);++j){
                        if(ids[j]==i){
                            matched = true;
                            break;
                        }
                    }
                    if(!matched)
                        min_free_id = i;
                }
                //define the offset of contracted axis with id less than minimal free axis-order
                //here the minimal must be bigger due to the flat data disposition (bigger id -> first)
                // for(std::size_t i=0;i<lo.order();++i){
                    for(std::size_t j=0;j<sizeof...(IDS);++j){
                        if(ids[j]>min_free_id){
                            cycle_offset*=lo.ranks()[ids[j]];
                        }
                    }
                // }
            //     return cycle_offset;
            // }();
            std::size_t num_sum = lo.total()/result.total();
            for(std::size_t i = 0;i<result.total();++i){
                //add cycle_offset*block_offset
                for(std::size_t j = 0;j<cycle_offset;++j)
                    for(std::size_t off = 0;off<cycle_offset;++off){
                        std::size_t id = off*j;
                        result.data()[i]+=lo.data()[id];
                    }
                        
                // std::size_t id = (i/cycle_offset)%result.total();
                // result.data()[id]+=lo.data()[i];
            }
            return result;
        }
    };
    struct ScalarMultiplies:Strategy{};
}