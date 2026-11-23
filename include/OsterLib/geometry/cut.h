#pragma once
#include <vector>
#include "OsterLib/geometry/intersections.h"

namespace geometry{
    template<typename G_CUTTED,typename G_CUTTING>
    std::vector<std::pair<G_CUTTED,G_CUTTED>> cut(
        const G_CUTTED& cutted,
        const G_CUTTING& cutting);

    template<uint64_t DIMENSION>
    struct Cut
    {
        template<typename G1, typename G2>
        std::vector<G1> operator()(
            const G1& geom1,
            const G2& geom2) 
            const noexcept(noexcept(cut(geom1,geom2)))
        {
            static_assert(G1::dimensions()==G2::dimensions() && 
                    std::is_same_v<typename G1::domain,typename G2::domain>);
            return cut(geom1,geom2);
        }
    };
}