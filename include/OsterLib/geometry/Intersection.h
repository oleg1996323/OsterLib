#pragma once
#include <cstdint>
#include <array>
#include "Point.h"

namespace geometry{
    template<typename DOMAIN,uint64_t DIMENSIONS=2>
    class Intersection{
        Point<DOMAIN,DIMENSIONS> point_;
        uint64_t id_1_;
        uint64_t id_2_;
        public:
        Intersection(Point<DOMAIN,DIMENSIONS> point,uint64_t id_1,uint64_t id_2):
            point_(point),id_1_(id_1),id_2_(id_2){}
        Intersection(const Intersection& other) noexcept:
            point_(other.point_),
            id_1_(other.id_1_),
            id_2_(other.id_2_){}
        Intersection(Intersection&& other) noexcept:
            point_(other.point_),
            id_1_(other.id_1_),
            id_2_(other.id_2_){}
        Intersection& operator=(const Intersection& other) noexcept{
            if(this!=&other){
                point_ = other.point_;
                id_1_ = other.id_1_;
                id_2_ = other.id_2_;
            }
            return *this;
        }
        Intersection& operator=(Intersection&& other) noexcept{
            if(this!=&other){
                point_ = other.point_;
                id_1_ = other.id_1_;
                id_2_ = other.id_2_;
            }
            return *this;
        }
        const Point<DOMAIN,DIMENSIONS>& point() const noexcept{
            return point_;
        }
        uint64_t first_elem_id() const noexcept{
            return id_1_;
        }
        uint64_t second_elem_id() const noexcept{
            return id_2_;
        }
        void first_elem_id(uint64_t val) noexcept{
            id_1_ = val;
        }
        void second_elem_id(uint64_t val) noexcept{
            id_2_ = val;
        }
    };

    template<uint64_t DIMENSIONS,typename DOMAIN>
    void print(const Intersection<DOMAIN,DIMENSIONS>& is){
        std::cout<<"intersection: point";print(is.point());
        std::cout<<"first ID: "<<is.first_elem_id()<<"; "<<"second ID: "<<is.second_elem_id()<<std::endl;
    }

    template<uint64_t DIMENSIONS,typename DOMAIN>
    void print(const std::vector<Intersection<DOMAIN,DIMENSIONS>>& iss){
        for(const Intersection<DOMAIN,DIMENSIONS>& is:iss)
            print(is);
    }
}