#pragma once
#include <vector>
#include <cstdint>
#include "OsterLib/geometry/Line.h"
#include <future>
#include <algorithm>
#include <numeric>
#include <unordered_set>
#include <iostream>
#include "OsterLib/geometry/algorithms.h"
#include <cstdint>
#include <limits>
#include <span>
#include <ranges>
#include <future>
#include <algorithm>
#include <numeric>
#include <functional>
#include <thread>
#include "OsterLib/geometry/Polyline.h"
#include "OsterLib/geometry/Intersection.h"
#include "OsterLib/geometry/area.h"
#include "OsterLib/linalgebra/LinearObject.h"
#include <cassert>

namespace geometry{

template<uint64_t DIMENSIONS,typename DOMAIN,typename VALUE_T>
class BasicPolygon:public BasicGeometry<false,2,Euclid>{
    BasicPolyline<DIMENSIONS,DOMAIN,VALUE_T> polyline_;
    using bounded_ids_t = std::array<std::pair<std::vector<uint64_t>,std::vector<uint64_t>>,DIMENSIONS>;
    bounded_ids_t bounds_;
    mutable double area_ = 0;
    mutable bool area_defined_ = false;
    mutable bool valid_defined_ = false;
    mutable bool is_valid_ = false;

    template<typename OTHER_VALUE_T>
    std::optional<std::tuple<uint64_t,int8_t>> sync_and_define_direction(const BasicPolygon<DIMENSIONS,DOMAIN,OTHER_VALUE_T>& other) const{
        if(other.number_vertices()<=2)
            return std::nullopt;
        std::size_t sz = vertices().size();
        uint64_t id_found = 0;
        bool found = false;
        auto& point = other.vertices().front();
        for(uint64_t i=0;i<sz;++i){
            if(vertices()[i]==point){
                id_found=i;
                found = true;
                break;
            }
        }
        if(found){
            int8_t inc = 0;
            auto& next_point = *(other.vertices().begin()+1);
            uint64_t next_found;
            for(uint64_t i = (id_found+1)%number_vertices();i!=id_found && 
                    vertices()[i]==vertices()[id_found];i=(i+1)%number_vertices()){}
            if(vertices()[next_found]==next_point)
                return std::make_tuple(id_found,1);
            for(uint64_t i = (id_found-1+number_vertices())%number_vertices();i!=id_found && 
                    vertices()[i]==vertices()[id_found];i=(i+1+number_vertices())%number_vertices()){}
            if(vertices()[next_found]==next_point)
                return std::make_tuple(id_found,-1);
            return std::nullopt;
        }
        else return std::nullopt;
    }

    static uint64_t vertices_per_thread_;

    template<typename VALUE_T_POINT>
    void update_bounds_before_erase(uint64_t new_pos) noexcept{
        [this,&new_pos]<std::size_t... IS>(std::index_sequence<IS...>){
            auto inner = [this,&new_pos]<std::size_t FIRST,std::size_t... REST>(){
                auto& min = bounds_[FIRST].first;
                auto& max = bounds_[FIRST].second;
                if(!min.empty()){
                    VALUE_T p = vertices()[new_pos].template get<FIRST>();
                    VALUE_T COORD_loc = vertices()[min.front()].template get<FIRST>();
                    if(COORD_loc==exec<Get,FIRST>(p)){
                        std::size_t last = min.size()-1;
                        for(uint64_t i = 0,j=i+1;i<last;++i,++j)
                            if(min[i]==new_pos)
                                min[i]=min[j]-1;
                        min.pop_back();
                    }
                }
                if(!max.empty()){
                    VALUE_T p = vertices()[new_pos].template get<FIRST>();
                    VALUE_T COORD_loc = vertices()[max.front()].template get<FIRST>();
                    if(COORD_loc==exec<Get,FIRST>(p)){
                        std::size_t last = max.size()-1;
                        for(uint64_t i = 0,j=i+1;i<last;++i,++j)
                            if(max[i]==new_pos)
                                max[i]=max[j]-1;
                        max.pop_back();
                    }
                }
            };
            (inner.template operator()<IS>(),...);
        }(std::make_index_sequence<DIMENSIONS>());
    }

    template<typename VALUE_T_POINT>
    void update_bounds_after_insert(const Point<DOMAIN,DIMENSIONS,VALUE_T_POINT>& p,uint64_t new_pos) noexcept{
        [&]<std::size_t... IS>(std::index_sequence<IS...>){
            auto inner = [&]<std::size_t FIRST,std::size_t... REST>(){
                auto& min = bounds_[FIRST].first;
                auto& max = bounds_[FIRST].second;
                if(!min.empty()){
                    VALUE_T COORD_loc = vertices()[min.front()].template get<FIRST>();
                    if(COORD_loc<exec<Get,FIRST>(p))
                        min.clear();
                    std::for_each(min.begin(),min.end(),[&new_pos](uint64_t& id){
                        if(id>=new_pos)
                            ++id;
                    });
                    if(COORD_loc<=exec<Get,FIRST>(p))
                        min.push_back(new_pos);
                }
                else min.push_back(new_pos);
                if(!max.empty()){
                    VALUE_T COORD_loc = vertices()[max.front()].template get<FIRST>();
                    if(COORD_loc>exec<Get,FIRST>(p))
                        max.clear();
                    std::for_each(max.begin(),max.end(),[&new_pos](uint64_t& id){
                        if(id>=new_pos)
                            ++id;
                    });
                    if(COORD_loc>=exec<Get,FIRST>(p))
                        max.push_back(new_pos);
                }
                else max.push_back(new_pos);
            };
            (inner.template operator()<IS>(),...);
        }(std::make_index_sequence<DIMENSIONS>());
    }

    template<typename OTHER_VALUE_T>
    bool bounds_equal(const BasicPolygon<DIMENSIONS,DOMAIN,OTHER_VALUE_T>& other) const{
        return [&]<std::size_t... IS>(std::index_sequence<IS...>){
            auto inner = [&]<std::size_t FIRST>(){
                auto& min = bounds_[FIRST].first;
                auto& max = bounds_[FIRST].second;
                auto& other_min = other.bounds_[FIRST].first;
                auto& other_max = other.bounds_[FIRST].second;
                if(!min.empty()){
                    if(other_min.empty())
                        return false;
                    VALUE_T this_coord = vertices()[min.front()].template get<FIRST>();
                    OTHER_VALUE_T other_coord = other.vertices()[other_min.front()].template get<FIRST>();
                    if(this_coord!=other_coord)
                        return false;
                }
                else{
                    if(!other_min.empty())
                        return false;
                }
                if(!max.empty()){
                    if(other_max.empty())
                        return false;
                    VALUE_T this_coord = vertices()[max.front()].template get<FIRST>();
                    OTHER_VALUE_T other_coord = other.vertices()[other_max.front()].template get<FIRST>();
                    if(this_coord!=other_coord)
                        return false;
                }
                else{
                    if(!other_max.empty())
                        return false;
                }
                return true;
            };
             return (inner.template operator()<IS>() && ...);
        }(std::make_index_sequence<DIMENSIONS>());
    }
    uint64_t define_turn_id(
        const std::vector<Intersection<DOMAIN,2>>& turns,
        const Intersection<DOMAIN,2>& ti) const noexcept{
        uint64_t result = std::lower_bound(turns.begin(),turns.end(),ti,
            [](const Intersection<DOMAIN,2>& t,const Intersection<DOMAIN,2>& turn)
        {
            return t.first_elem_id()<turn.first_elem_id();
        }) - turns.begin();
        return result;
    }

    public:
    BasicPolygon()=default;
    BasicPolygon(const BasicPolygon& other):
        polyline_(other.polyline_),
        area_defined_(other.area_defined_),
        valid_defined_(other.valid_defined_),
        area_(other.area_),
        is_valid_(other.is_valid_),
        bounds_(other.bounds_){}
    BasicPolygon(BasicPolygon&& other) noexcept:
        polyline_(std::move(other.polyline_)),
        area_defined_(other.area_defined_),
        valid_defined_(other.valid_defined_),
        area_(other.area_),
        is_valid_(other.is_valid_),
        bounds_(other.bounds_){}
    BasicPolygon& operator=(const BasicPolygon& other){
        if(this!=&other){
            polyline_=other.polyline_;
            area_defined_ = other.area_defined_;
            valid_defined_ = other.valid_defined_;
            area_ = other.area_;
            is_valid_ = other.is_valid_;
            bounds_ = other.bounds_;
        }
        return *this;
    }
    BasicPolygon& operator=(BasicPolygon&& other){
        if(this!=&other){
            polyline_=std::move(other.polyline_);
            area_defined_ = other.area_defined_;
            valid_defined_ = other.valid_defined_;
            area_ = other.area_;
            is_valid_ = other.is_valid_;
            bounds_ = std::move(other.bounds_);
        }
        return *this;
    }
    bool operator==(const BasicPolygon& other) const noexcept{
        return this == &other ||
            (polyline_.number_vertices()==other.polyline_.number_vertices() &&
            is_valid()==other.is_valid() &&
            area()==other.area() &&
            perimeter()==other.perimeter() &&
            bounds_ == other.bounds_);
    }
    std::optional<double> area() const noexcept{
        if(area_defined_)
            return area_;
        else{
            if(is_valid()){
                area_defined_ = false;
                return std::nullopt;
            }
            area_defined_ = true;
            area_ = exec<Area,DIMENSIONS>(*this);
            return area_;
        }
    }
    std::optional<double> perimeter() const noexcept{
        auto result = polyline_.length();
        if(result==0)
            return 0;
        return result + 
            exec<Distance,3>(vertices().back(),vertices().front());
    }
    bool is_valid() const noexcept{
        if(valid_defined_)
            return is_valid_;
        else{
            valid_defined_ = true;
            if(polyline_.number_vertices()<3){
                is_valid_ = false;
                return is_valid_;
            }
            else{
                //if self-intersections are not allowed
                uint64_t last_segment = polyline_.number_vertices()-1;
                while(last_segment>1 &&
                    (vertices()[last_segment]==
                    vertices()[(last_segment-1+
                    polyline_.number_vertices())%polyline_.number_vertices()] || 
                    vertices()[last_segment] == vertices()[0]))
                {
                    --last_segment;
                }
                if(last_segment<2){
                    is_valid_ = false;
                    return is_valid_;
                }
                uint64_t first_segment = 0;
                while(first_segment<last_segment && 
                    vertices()[first_segment]==
                        vertices()[first_segment+1])
                    ++first_segment;
                for(uint64_t i=first_segment;i<=last_segment;++i){
                    while(i<=last_segment && vertices()[i]==vertices()[i+1])
                        ++i;
                    if(i>last_segment)
                        break;
                    uint64_t j_last = last_segment;
                    if(i==first_segment)
                        --j_last;
                    uint64_t j = i+2;
                    while(j<=j_last && vertices()[i+1]==vertices()[j])
                        ++j;
                    if(j>j_last)
                        break;
                    Line init_line(vertices()[i],vertices()[i+1]);
                    for(;j<=j_last;++j){
                        while(j<=last_segment && 
                                (vertices()[j]==vertices()[(j+1)%polyline_.number_vertices()]))/* ||
                                vertices()[j]==vertices()[j-1]))*/
                            ++j;
                        if(j>last_segment)
                            break;
                        else{
                            if(init_line.intersects(Line(vertices()[j],vertices()[(j+1)%polyline_.number_vertices()]))){
                                is_valid_ = false;
                                return is_valid_;
                            }
                            else continue;
                        }
                    }
                }
                is_valid_ = true;
                return is_valid_;
            }
        }
    }
    void append(Point<Euclid,2> coord) noexcept{
        if(!vertices().empty()){
            if(vertices().back()!=coord &&
                vertices().front()!=coord)
            {
                area_defined_ = false;
                valid_defined_ = false;
            }
        }
        else{
            area_defined_ = false;
            valid_defined_ = false;
        }
        polyline_.append(std::move(coord));
        update_bounds_after_insert(coord,polyline_.number_vertices());
    }
    template<typename OTHER_VALUE_T>
    void insert(uint64_t coord_id,Point<DOMAIN,DIMENSIONS,OTHER_VALUE_T> coord) noexcept{
        if(coord_id>=polyline_.number_vertices())
            append(coord);
        else{
            if(vertices()[(coord_id-1+polyline_.number_vertices())%
                polyline_.number_vertices()]!=coord &&
            vertices()[(coord_id+1)%polyline_.number_vertices()]!=coord)
            {
                area_defined_ = false;
                valid_defined_ = false;
            }
            polyline_.insert(coord_id,coord);
            update_bounds_after_insert(coord,coord_id);
        }
    }
    void erase(uint32_t coord_id) noexcept{
        if(vertices().empty())
            return;
        uint64_t erased_id = coord_id>=polyline_.number_vertices()?polyline_.number_vertices()-1:coord_id;
        update_bounds_before_erase(erased_id);
        valid_defined_ = false;
        area_defined_ = false;
        polyline_.erase(erased_id);
    }
    size_t number_vertices() const noexcept{
        return polyline_.number_vertices();
    }
    const std::vector<Point<DOMAIN,DIMENSIONS,VALUE_T>>& vertices() const noexcept{
        return polyline_.vertices();
    }

    bool point_inside(const Point<Euclid,2>& point,
            uint32_t max_threads = std::thread::hardware_concurrency()) const noexcept;
    std::vector<BasicPolygon> intersection(const BasicPolygon& other) const noexcept;

    bool is_plane() const;

    template<typename OTHER_VALUE_T>
    std::vector<BasicPolygon> cut(
        const Line<DIMENSIONS,DOMAIN,OTHER_VALUE_T>& line
        ) const noexcept;
    
    template<typename OTHER_VALUE_T>
    std::vector<BasicPolygon> clip(
        const Line<DIMENSIONS,DOMAIN,OTHER_VALUE_T>& line,
        bool left_forward
        ) const noexcept
    {
        std::vector<BasicPolygon<DIMENSIONS,DOMAIN,VALUE_T>> result;
        //firstly define the coplanarity of Line and Polygon
        //if planar - define the increment for Polygon
        if(DIMENSIONS==2){
            const geometry::Point<DOMAIN,DIMENSIONS,VALUE_T>& first = line.first();
            const geometry::Point<DOMAIN,DIMENSIONS,VALUE_T>& second = line.second();
            bool dy_positive = first.template get<1>()<second.template get<1>();
            bool dy_zero = first.template get<1>()==second.template get<1>();
            bool dx_positive = first.template get<0>()<second.template get<0>();
            bool dx_zero = first.template get<0>()==second.template get<0>();
            Line<2,DOMAIN,VALUE_T> seg;
            if(dy_positive)
                seg = Line(first,second);
            else{
                if(dy_zero && dx_positive)
                    seg = Line(make_point<DOMAIN>(first.template get<0>(),first.template get<1>()),make_point<DOMAIN>(second.template get<0>(),second.template get<1>()));
                else{
                    seg = Line(make_point<DOMAIN>(second.template get<0>(),second.template get<1>()),
                        make_point<DOMAIN>(first.template get<0>(),first.template get<1>()));
                    left_forward = !left_forward;
                }   
            }
            return {};//@todopg.cut_impl(seg,left_forward,pg.is_clockwise());
        }
        else{
            if(!is_plane())
                return {*this};
            else{
                return {};
            }
        }
    }
    template<typename OTHER_VALUE_T>
    bool equivalent(const BasicPolygon<DIMENSIONS,DOMAIN,OTHER_VALUE_T>& other,bool multithread) const noexcept{
        uint64_t number_threads = polyline_.number_vertices()/vertices_per_thread_;
        if(!bounds_equal(other))
            return false;
        else{
            std::optional<std::tuple<uint64_t,int8_t>> found_id_and_inc = other.sync_and_define_direction(*this);
            if(found_id_and_inc.has_value())
                return false;
            else{
                int8_t other_inc = std::get<1>(found_id_and_inc.value());
                uint64_t j_start = std::get<0>(found_id_and_inc.value());
                uint64_t last = number_vertices()-1;
                for(uint64_t i = 0, j = j_start;
                    i<=last;++i,j=(j+other_inc+other.polyline_.number_vertices())%other.polyline_.number_vertices())
                {
                    uint64_t j_loc = (j+other_inc+other.polyline_.number_vertices())%other.polyline_.number_vertices();
                    uint64_t i_loc = (i+1+polyline_.number_vertices())%polyline_.number_vertices();
                    while(other.vertices()[j]==other.vertices()[j_loc]){
                        j=(j+other_inc+other.polyline_.number_vertices())%other.polyline_.number_vertices();
                        j_loc = (j+other_inc+other.polyline_.number_vertices())%other.polyline_.number_vertices();
                    }
                    while(vertices()[i]==vertices()[i_loc] && i<last){
                        ++i;
                        ++i_loc;
                    }
                    if(vertices()[i]!=other.vertices()[j])
                        return false;
                    continue;
                }
                return true;
            }
        }
    }
};

template<uint64_t DIMENSIONS,typename DOMAIN,typename VALUE_T1,typename VALUE_T2>
std::vector<Intersection<Euclid,2>> intersections(
        const BasicPolygon<DIMENSIONS,DOMAIN,VALUE_T1> pg, 
        const Line<DIMENSIONS,DOMAIN,VALUE_T2>& cut_line)
{
    std::vector<Intersection<Euclid,2>> result;
    for(uint64_t i = 0;i<pg.number_vertices();++i){
        auto& first = pg.vertices()[i];
        auto& second = pg.vertices()[(i+1)%pg.number_vertices()];
        if(geometry::intersects(first,second,
            cut_line.first(),cut_line.second())){
            if(auto found = cut_line.intersection(Line(first.lon_,first.lat_,second.lon_,second.lat_));
                found.has_value()){
                Point<Euclid,2> coord{found.value().first,found.value().second};
                if(!result.empty()){
                    if(result.back().point()!=coord && result.front().point()!=coord)
                        result.push_back(Intersection(coord,i,0));
                    else continue;
                }
                else result.push_back(Intersection(coord,i,0));
            }
            else continue;
        }
        else continue;
    }
    std::sort(result.begin(),result.end(),
        [](const Intersection<DOMAIN,2>& lhs,const Intersection<DOMAIN,2>& rhs)
    {
        return lhs.first_elem_id()<rhs.first_elem_id();
    });
    return result;
}
template<typename VALUE_T1,typename VALUE_T2>
std::vector<Intersection<Euclid,2>> intersections(
    const BasicPolygon<2,Euclid,VALUE_T1> pg,
    const BasicPolyline<2,Euclid,VALUE_T2>& cut_line)
{
    std::vector<Intersection<Euclid,2>> result;
    for(uint64_t i = 0;i<pg.number_vertices();++i){
        auto& first = pg.vertices()[i];
        auto& second = pg.vertices()[(i+1)%pg.number_vertices()];
        if(geometry::intersects(first,second,
            cut_line.first(),cut_line.second())){
            if(auto found = exec<Intersections,2>(cut_line,Line(first,second));
                found.has_value()){
                Point<Euclid,2> coord{  .lat_ = found.value().second,
                        .lon_ = found.value().first};
                if(!result.empty()){
                    if(result.back().point()!=coord && result.front().point()!=coord)
                        result.push_back(Intersection(coord,i,0));
                    else continue;
                }
                else result.push_back(Intersection(coord,i,0));
            }
            else continue;
        }
        else continue;
    }
    std::sort(result.begin(),result.end(),
        [](const Intersection<Euclid,2>& lhs,const Intersection<Euclid,2>& rhs)
    {
        return lhs.first_elem_id()<rhs.first_elem_id();
    });
    return result;
}
template<typename VALUE_T>
long double area(const BasicPolygon<2,Euclid,VALUE_T>& pg) noexcept{
    long double result = 0;
    if(pg.number_vertices()<3)
        return result;
    auto sz = pg.number_segments();
    for(uint64_t i = 0,j=i+1;i<sz;++i,++j)
        result+=pg.vertices()[j].template get<1>()*pg.vertices()[i].template get<0>()-
                pg.vertices()[j].template get<0>()*pg.vertices()[i].template get<1>();
    result+=pg.vertices().front().template get<1>()*pg.vertices().back().template get<0>()-
                pg.vertices().front().template get<0>()*pg.vertices().back().template get<1>();
    return result/2.;
}

template<typename VALUE_T>
void print(const BasicPolygon<2,Euclid,VALUE_T>& pg){
    std::cout<<"outer: ";
    if(!pg.vertices().empty()){
        for(auto& vertice:pg.vertices())
            print(vertice);
    }
    else std::cout<<"NaN";
    std::cout<<std::endl;
}

template<uint64_t DIMENSIONS,typename DOMAIN,typename VALUE_T= long double>
using Polygon = BasicPolygon<DIMENSIONS,DOMAIN,VALUE_T>;

template<uint64_t DIMENSIONS,typename DOMAIN,typename VALUE_T1,typename VALUE_T2>
bool equivalent(const BasicPolygon<DIMENSIONS,DOMAIN,VALUE_T1>& first,
            const BasicPolygon<DIMENSIONS,DOMAIN,VALUE_T1>& second,bool multithread)
{
    return first.equivalent(second,multithread);
}

template<uint64_t DIM,typename DOMAIN,typename VALUE_T>
bool BasicPolygon<DIM,DOMAIN,VALUE_T>::is_plane() const{
    if constexpr(DIM==2)
        return true;
    else {
        if(number_vertices()<4)
            return true;
        //@todo
        linalgebra::Vector<DIM,true,VALUE_T,true> v1(
            linalgebra::as_vector(vertices()[0].data()));
        linalgebra::Vector<DIM,true,VALUE_T,true> v2(
            linalgebra::as_vector(vertices()[1].data()));
        linalgebra::Vector<DIM,true,VALUE_T,true> v3(
            linalgebra::as_vector(vertices()[2].data()));
        auto f = [](){
            
        };     
        assert(false);
        return false;
    }
}
}