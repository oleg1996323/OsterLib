#pragma once
#include <type_traits>
#include <string_view>
#include "OsterLib/geometry/BasicGeometry.h"
#include "OsterLib/concepts.h"
#include <iostream>

namespace geometry{
    template<uint64_t DIMENSION,typename POINT_T>
    const auto& get(const POINT_T& point);

    template<uint64_t DIMENSION,typename POINT_T>
    auto& get(POINT_T& point);

    template<template<uint64_t, 
    typename...> typename F, uint64_t D, typename... ARGS>
    auto exec(ARGS&&... args)
        noexcept(noexcept(F<D>{}(std::forward<ARGS>(args)...)))
        -> decltype(F<D>{}(std::forward<ARGS>(args)...))
    {
        return F<D>{}(std::forward<ARGS>(args)...);
    }

    template<uint64_t DIMENSION>
    struct Get{
        template<typename POINT_T>
        auto& operator()(POINT_T& point)
        {
            return get<DIMENSION>(point);
        };
        template<typename POINT_T>
        const auto&
            operator()(const POINT_T& point) const
        {
            return get<DIMENSION>(point);
        };
    };

    template<uint64_t DIMENSION>
    struct Set{
        template<typename POINT_T,typename VALUE_T>
        void operator()(POINT_T& point,VALUE_T value){
            get<DIMENSION>(point) = value;
        }
    };

    template<uint64_t D,typename P,typename = std::void_t<>>
    struct defined_get_coordinate:std::false_type{};

    template<uint64_t D,typename P>
    struct defined_get_coordinate<D,P,
    std::void_t<decltype(get<D>(std::declval<const P&>()))>>:std::true_type{};

    template<typename POINT_T, uint64_t D = 0, uint64_t MAX = MAX_ALLOWABLE_DIMENSIONS>
    consteval uint64_t dimensions() noexcept {
        static_assert(D<=MAX,MAX_ALLOWABLE_DIMENSIONS_STR(MAX_ALLOWABLE_DIMENSIONS));
        if constexpr (D >= MAX)
            return MAX;
        else if constexpr (defined_get_coordinate<D, POINT_T>::value)
            return dimensions<POINT_T, D + 1, MAX>();
        else
            return D;
    }

    template<typename POINT_T>
    void print(const POINT_T& point, std::string_view desc = "", bool newline = false)
    {
        if (!desc.empty())
            std::cout << desc.data() << ": ";
        std::cout << "(";

        [&]<std::uint64_t... Is>(std::index_sequence<Is...>) {
            ((
                std::cout
                    << (Is == 0 ? "" : ",")
                    << exec<Get,Is>(point)
            ), ...);
        }(std::make_index_sequence<geometry::dimensions<POINT_T>()>());

        std::cout << ")";
        if (newline)
            std::cout << std::endl;
    }

    template<typename DOMAIN, typename... Ts>
    auto make_point(Ts&&... ts)
        -> BasicPoint<sizeof...(Ts), DOMAIN,
                    std::common_type_t<std::decay_t<Ts>...>>;

    //specified point
    template<uint64_t DIMENSIONS,typename DOMAIN,typename VALUE_T = long double>
    class BasicPoint:public BasicGeometry<false,DIMENSIONS,DOMAIN>{
        std::array<VALUE_T,DIMENSIONS> data_;
        
        template<typename DOM, typename... Ts>
        friend auto make_point(Ts&&... ts);
        public:
        BasicPoint() = default;
        template<typename First, typename... Rest,
            typename = std::enable_if_t<
                sizeof...(Rest) + 1 == DIMENSIONS &&
                (std::is_convertible_v<First, Rest> && ...) &&
                (std::is_convertible_v<Rest, First> && ...)>>
        constexpr BasicPoint(First first, Rest... rest)
            : data_{static_cast<VALUE_T>(first), static_cast<VALUE_T>(rest)...} {}
        template<typename T, std::size_t N,
                typename = std::enable_if_t<N == DIMENSIONS>>
        BasicPoint(const T (&arr)[N]) {
            for (std::size_t i = 0; i < N; ++i)
                data_[i] = static_cast<VALUE_T>(arr[i]);
        }
        BasicPoint(const BasicPoint& other)
        {
            [this,&other]<uint64_t... Is>(std::index_sequence<Is...> is){
                ((data_[Is]=other.data_[Is]),...);
            }(std::make_index_sequence<BasicGeometry<false,DIMENSIONS,DOMAIN>::dimensions()>());
        }
        template<typename P,
                typename = std::void_t<
                    decltype(exec<Get, 0>(std::declval<const P&>()))>,
                typename = std::enable_if_t<
                    !std::is_same_v<P, BasicPoint> &&
                    !std::is_same_v<P, BasicPointRef<DIMENSIONS, DOMAIN,VALUE_T>>>>
        BasicPoint(const P& point)
        {
            [this,&point]<uint64_t... Is>(std::index_sequence<Is...>) {
                ((data_[Is] = exec<Get, Is>(point)), ...);
            }(std::make_index_sequence<DIMENSIONS>());
        }
        BasicPoint(BasicPoint&& other) noexcept
        {
            if(this!=&other)
                data_.swap(other.data_);
        }
        
        BasicPoint& operator=(const BasicPoint& other){
            if(this!=&other){
                [this,&other]<uint64_t... Is>(std::index_sequence<Is...> is){
                    ((data_[Is]=other.data_[Is]),...);
                }(std::make_index_sequence<
                    BasicGeometry<false, DIMENSIONS,DOMAIN>::dimensions()>());
            }
            return *this;
        }
        BasicPoint& operator=(BasicPoint&& other){
            if(this!=&other)
                data_.swap(other.data_);
            return *this;
        }
        BasicPoint(const BasicPointRef<DIMENSIONS,DOMAIN,VALUE_T>& point);
        BasicPoint(BasicPointRef<DIMENSIONS,DOMAIN,VALUE_T>&& point) noexcept;
        BasicPoint& operator=(const BasicPointRef<DIMENSIONS,DOMAIN,VALUE_T>& other);
        BasicPoint& operator=(BasicPointRef<DIMENSIONS,DOMAIN,VALUE_T>&& other) noexcept;
        bool operator==(const BasicPoint<DIMENSIONS,DOMAIN,VALUE_T>& other) const{
            auto cmp = [this,&other]<uint64_t... Is>(std::index_sequence<Is...>)
            {
                return ((get<Is>()==other.get<Is>()) && ...);
            };
            return cmp(std::make_index_sequence<DIMENSIONS>());
        }
        template<uint64_t DIMENSION>
        std::enable_if_t<(DIMENSION<DIMENSIONS),const VALUE_T&> get() const noexcept{
            static_assert(DIMENSION<DIMENSIONS,
            "Not existing dimension");
            return data_[DIMENSION];
        }
        template<uint64_t DIMENSION>
        std::enable_if_t<(DIMENSION<DIMENSIONS),VALUE_T&> get() noexcept{
            static_assert(DIMENSION<DIMENSIONS,
            "Not existing dimension");
            return data_[DIMENSION];
        }
        template<uint64_t DIMENSION,typename ARG>
        void set(ARG value) noexcept{
            static_assert(DIMENSION<DIMENSIONS,
            "Not existing dimension");
            data_[DIMENSION] = value;
        }

        std::array<VALUE_T,DIMENSIONS>& data() noexcept{
            return data_;
        }
        const std::array<VALUE_T,DIMENSIONS>& data() const noexcept{
            return data_;
        }
    };

    template<typename DOMAIN, typename... Ts>
    auto make_point(Ts&&... ts)
        -> BasicPoint<sizeof...(Ts), DOMAIN,
                    std::common_type_t<std::decay_t<Ts>...>>
    {
        return BasicPoint<sizeof...(Ts), DOMAIN,
                        std::common_type_t<std::decay_t<Ts>...>>(
                        std::forward<Ts>(ts)...);
    }

    template<uint64_t DIMENSION,uint64_t DIMENSIONS,typename DOMAIN,typename VALUE_T>
    std::enable_if_t<DIMENSION<DIMENSIONS,
        const VALUE_T&> get(const BasicPoint<DIMENSIONS,DOMAIN,VALUE_T>& point){
        return point.template get<DIMENSION>();
    }

    template<uint64_t DIMENSION,uint64_t DIMENSIONS,typename DOMAIN,typename VALUE_T>
    std::enable_if_t<DIMENSION<DIMENSIONS,
        VALUE_T&> get(BasicPoint<DIMENSIONS,DOMAIN,VALUE_T>& point){
        return point.template get<DIMENSION>();
    }

    template<uint64_t DIMENSIONS,typename DOMAIN,typename VALUE_T = long double>
    class BasicPointRef:public BasicGeometry<true,DIMENSIONS,DOMAIN>{
        std::array<VALUE_T,DIMENSIONS>& data_;
        public:
        BasicPointRef(const BasicPointRef& other):
            data_(other.data_)
        {}
        BasicPointRef(BasicPointRef&& other) = delete;
        BasicPointRef& operator=(const BasicPointRef& other){
            if(this!=&other)
                data_ = other.data_;
            return *this;
        }
        BasicPointRef& operator=(BasicPointRef&& other) = delete;
        template<uint64_t DIMENSION>
        const VALUE_T& get() const noexcept{
            static_assert(DIMENSION<DIMENSIONS,
            "Not existing dimension");
            return data_[DIMENSION];
        }
    };

    template<uint64_t DIMENSION,uint64_t DIMENSIONS,typename DOMAIN,typename VALUE_T>
    const VALUE_T& get(const BasicPointRef<DIMENSIONS,VALUE_T>& point){
        return point.template get<DIMENSION>();
    }

    template<uint64_t DIMENSIONS,typename DOMAIN,typename VALUE_T>
    BasicPoint<DIMENSIONS,DOMAIN,VALUE_T>::BasicPoint(const BasicPointRef<DIMENSIONS,DOMAIN,VALUE_T>& other)
    {
        
        [this,&other]<uint64_t... Is>(std::index_sequence<Is...> is){
            ((set<Is>(other.template get<Is>())),...);
        }(std::make_index_sequence<BasicGeometry<false,DIMENSIONS,DOMAIN>::dimensions()>());
    }

    template<uint64_t DIMENSIONS,typename DOMAIN,typename VALUE_T>
    BasicPoint<DIMENSIONS,DOMAIN,VALUE_T>::BasicPoint(BasicPointRef<DIMENSIONS,DOMAIN,VALUE_T>&& other) noexcept
    {
        data_.swap(other.data_);
    }

    template<uint64_t DIMENSIONS,typename DOMAIN,typename VALUE_T>
    BasicPoint<DIMENSIONS,DOMAIN,VALUE_T>& 
        BasicPoint<DIMENSIONS,DOMAIN,VALUE_T>::operator=(
            const BasicPointRef<DIMENSIONS,DOMAIN,VALUE_T>& other)
    {
        [this,&other]<uint64_t... Is>(std::index_sequence<Is...> is){
            ((set<Is>(other.template get<Is>())),...);
        }(std::make_index_sequence<BasicGeometry<false,DIMENSIONS,DOMAIN>::dimensions()>());
        return *this;
    }
    static_assert(std::is_default_constructible_v<BasicPoint<2,Euclid>>);
    static_assert(std::is_copy_constructible_v<BasicPoint<2,Euclid>>);
    static_assert(std::is_move_constructible_v<BasicPoint<2,Euclid>>);
    static_assert(std::is_copy_assignable_v<BasicPoint<2,Euclid>>);
    static_assert(std::is_move_assignable_v<BasicPoint<2,Euclid>>);
    static_assert(std::is_assignable_v<BasicPoint<2,Euclid>,const BasicPointRef<2,Euclid>&>);
    static_assert(std::is_assignable_v<BasicPoint<2,Euclid>,BasicPointRef<2,Euclid>&&>);
    static_assert(!std::is_default_constructible_v<BasicPointRef<2,Euclid>>);
    static_assert(std::is_copy_constructible_v<BasicPointRef<2,Euclid>>);
    static_assert(!std::is_move_constructible_v<BasicPointRef<2,Euclid>>);
    static_assert(std::is_copy_assignable_v<BasicPointRef<2,Euclid>>);
    static_assert(!std::is_move_assignable_v<BasicPointRef<2,Euclid>>);
    static_assert(dimensions<BasicPoint<2,Euclid>>() == 2);
    template<typename DOMAIN,uint64_t DIMENSIONS,typename VALUE_T = long double>
    using Point = BasicPoint<DIMENSIONS,DOMAIN,VALUE_T>;
}

#include "distance.h"

namespace geometry{
    template<std::size_t DIMENSIONS,typename V1,typename V2>
    inline auto distance(
        const Point<Euclid,DIMENSIONS,V1>& first,
        const Point<Euclid,DIMENSIONS,V2>& second) noexcept
    {
        static_assert(DIMENSIONS>0,"Dimmensions cannot be 0");
        return std::sqrt([&]<std::size_t... IS>(std::index_sequence<IS...>) -> long double{
            auto summarize_inner = [&]<std::size_t FIRST_IS>()->long double
            {
                long double delta = static_cast<long double>(exec<Get,FIRST_IS>(second)) - exec<Get,FIRST_IS>(first);
                return delta*delta;
            };
            return (0.0L+...+summarize_inner.template operator()<IS>());
        }(std::make_index_sequence<DIMENSIONS>()));
    }
}