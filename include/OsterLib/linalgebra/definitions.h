#pragma once
#include <type_traits>
#include <cstddef>
#include <cstdint>
#include <array>
#include <algorithm>
#include <utility>
#include "OsterLib/traits.h"

namespace linalgebra{
    template<bool VIEW,typename VALUE_T,std::size_t... RANKS>
    class LinearObject;

    namespace FromIntegerSequence{
        template<bool VIEW, typename VALUE_T, typename RANKS>
        struct ToLOImpl{
            static_assert(VIEW && false, "Third argument must be std::integer_sequence<std::size_t, ...>");
        };

        template<bool VIEW, typename VALUE_T, std::size_t... RANKS>
        struct ToLOImpl<VIEW, VALUE_T, std::integer_sequence<std::size_t, RANKS...>> {
            using type = ::linalgebra::LinearObject<VIEW, VALUE_T, RANKS...>;
        };

        template<bool VIEW, typename VALUE_T, typename RANKS>
        using LinearObject = typename ToLOImpl<VIEW, VALUE_T, RANKS>::type;
    }

    template<typename VALUE_T,std::size_t RANK>
    class Vector;

    template<std::size_t H_RANK, std::size_t V_RANK,typename VALUE_T>
    class Matrix;
    template<std::size_t H_RANK, std::size_t V_RANK,typename VALUE_T>
    class MatrixWrapper;
}