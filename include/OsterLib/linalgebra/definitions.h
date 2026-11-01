#pragma once
#include <type_traits>
#include <cstddef>
#include <cstdint>
#include <array>
#include <algorithm>
#include <utility>
#include "OsterLib/traits.h"

namespace linalgebra{
    template<typename VALUE_T,std::size_t RANK>
    class Vector;

    template<std::size_t H_RANK, std::size_t V_RANK,typename VALUE_T>
    class Matrix;
    template<std::size_t H_RANK, std::size_t V_RANK,typename VALUE_T>
    class MatrixWrapper;
}