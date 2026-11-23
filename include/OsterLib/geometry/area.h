#pragma once
#include <cstdint>

namespace geometry{
    template<typename G>
    auto area(const G& geom1) noexcept;

    template<uint64_t DIMENSION>
    struct Area
    {
        template<typename G>
        auto operator()(const G& geom) 
            const noexcept(noexcept(distance(geom)))
        {
            return area(geom);
        }
    };
}