#pragma once
#include <vector>
#include "OsterLib/geometry/intersections.h"

namespace geometry{
    template<typename G_CUTTED,typename G_CUTTING>
    std::vector<G_CUTTED> clip(
        const G_CUTTED& clipped,
        const G_CUTTING& clipping,
        bool left_forward);

    template<uint64_t DIMENSION>
    struct Clip
    {
        template<typename G1, typename G2>
        std::vector<G1> operator()(
            const G1& geom1,
            const G2& geom2,
            bool left_forward) 
            const noexcept(noexcept(clip(geom1,geom2,left_forward)))
        {
            static_assert(G1::dimensions()==G2::dimensions() && 
                    std::is_same_v<typename G1::domain,typename G2::domain>);
            return clip(geom1,geom2,left_forward);
        }
    };
}