#pragma once


namespace geometry{
    template<typename G1,typename G2>
    bool intersects(const G1& geom1,const G2& geom2) noexcept;

    template<uint64_t DIMENSION>
    struct Intersects
    {
        template<typename G1, typename G2>
        bool operator()(const G1& geom1,const G2& geom2) 
            const noexcept(noexcept(intersects(geom1,geom2)))
        {
            static_assert(G1::dimensions()==G2::dimensions() && 
                    std::is_same_v<typename G1::domain,typename G2::domain>);
            return intersects(geom1,geom2);
        }
    };
}