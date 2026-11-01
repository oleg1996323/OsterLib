#pragma once
#include <cstdint>
#include <cstddef>

namespace osterlib{
    template<uint64_t UINT1,uint64_t UINT2>
    struct Max{
        static constexpr uint64_t value = (UINT1<UINT2)?UINT2:UINT1;
    };

    template<uint64_t UINT1,uint64_t UINT2>
    struct Min{
        static constexpr uint64_t value = (UINT1>UINT2)?UINT2:UINT1;
    };
}