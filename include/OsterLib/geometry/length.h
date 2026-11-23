#pragma once


namespace geometry{
    template<typename GEOM>
    auto length(const GEOM& geom);

    template<uint64_t DIMENSION>
    struct Length
    {
        template<typename GEOM>
        auto operator()(const GEOM& geom) 
            const noexcept(noexcept(length(geom)))
        {
            return length(geom);
        }
    };
}