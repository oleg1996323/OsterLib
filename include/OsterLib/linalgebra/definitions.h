#pragma once
#include <type_traits>
#include <cstddef>
#include <cstdint>
#include <array>
#include <algorithm>
#include <utility>
#include "OsterLib/traits.h"

namespace linalgebra{
    template<bool VIEW,typename VALUE_T,bool CONST,std::size_t... RANKS>
    class LinearObject;

    template<template<bool,typename,bool,std::size_t...> typename LO,std::size_t... IDS>
    class LinearSubObject:std::false_type{};

    namespace FromIntegerSequence{
        template<bool VIEW, typename VALUE_T, typename RANKS>
        struct ToLOImpl{
            static_assert(VIEW && false, "Third argument must be std::integer_sequence<std::size_t, ...>");
        };

        template<bool VIEW, typename VALUE_T, std::size_t... RANKS>
        struct ToLOImpl<VIEW, VALUE_T, std::integer_sequence<std::size_t, RANKS...>> {
            using type = ::linalgebra::LinearObject<VIEW, VALUE_T, RANKS...>;
        };

        template<bool VIEW, typename VALUE_T, bool CONST, typename RANKS>
        using LinearObject = typename ToLOImpl<VIEW, VALUE_T, RANKS>::type;
    }

    template<std::size_t DIM, bool VIEW,typename VALUE_T, bool CONST>
    using Vector = LinearObject<VIEW,VALUE_T,CONST,DIM>;

    template<std::size_t H_DIM, std::size_t V_DIM,bool VIEW,typename VALUE_T, bool CONST>
    using Matrix = LinearObject<VIEW,VALUE_T,CONST,H_DIM,V_DIM>;
}