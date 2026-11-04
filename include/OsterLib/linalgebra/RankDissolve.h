#pragma once
#include <cstddef>
#include <cstdint>
#include "OsterLib/traits.h"

namespace linalgebra{
    namespace __detail__{
        template<typename VALUE_T,std::size_t First,std::size_t... RANKS>
        struct RankDropper;

        template<typename VALUE_T,std::size_t... RANKS>
        struct RankDropper<VALUE_T,0,RANKS...>{
            using type = LinearObject<true,VALUE_T,RANKS...>;
        };

        template<typename VALUE_T,std::size_t SKIP,std::size_t FIRST,std::size_t... RANKS>
        struct RankDropper<VALUE_T,SKIP,FIRST,RANKS...>:RankDropper<VALUE_T,SKIP-1,RANKS...>{};
    }
}