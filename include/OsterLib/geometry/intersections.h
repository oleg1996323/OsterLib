#pragma once
#include <vector>
#include "OsterLib/geometry/Intersection.h"

namespace geometry{
    template<typename G1,typename G2>
    std::vector<Intersection<typename G1::domain,G1::dimensions()>> intersections(const G1& geom1,const G2& geom2) noexcept;

    template<uint64_t DIMENSION>
    struct Intersections
    {
        template<typename G1,typename G2>
        std::vector<Intersection<typename G1::domain,G1::dimensions()>> operator()(const G1& geom1,const G2& geom2) 
            const noexcept(noexcept(intersections(geom1,geom2)))
        {
            static_assert(std::is_same_v<typename G1::domain,typename G2::domain>,"Geomtries domains must be same");
            return intersections(geom1,geom2);
        }
    };
}