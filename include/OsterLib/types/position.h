#pragma once
#include "OsterLib/serialization.h"
#include "OsterLib/float_conv.h"
typedef double Y;
typedef double X;

struct Position
{
    Y y_ = -999.;
    X x_ = -999.;

    bool is_correct_pos() const{
        if(x_>=0 && x_<=180 && y_<=90 && y_>=-90)
            return true;
        else return false;
    }

    bool operator==(const Position& other) const{
        return y_ == other.y_ && x_ == other.x_;
    }

    bool operator!=(const Position& other) const{
        return !operator==(other);
    }
};

struct RawPosition
{
    oster::detail::to_integer_type<sizeof(decltype(std::declval<Position>().y_))> y_ = -999;
    oster::detail::to_integer_type<sizeof(decltype(std::declval<Position>().x_))> x_ = -999;
};

extern bool is_correct_pos(const Position* pos);
bool is_correct_pos(const Position& pos);

namespace serialization{

template<bool NETWORK_ORDER>
struct Serialize<NETWORK_ORDER,Position>{
    SerializationEC operator()(const Position& pos, std::vector<char>& buf) const noexcept{
        return serialize<NETWORK_ORDER>(pos,buf,pos.y_,pos.x_);
    }
};

template<bool NETWORK_ORDER>
struct Deserialize<NETWORK_ORDER,Position>{
    SerializationEC operator()(Position& pos,StreamSerializer& buf) const noexcept{
        return deserialize<NETWORK_ORDER>(pos,buf,pos.y_,pos.x_);
    }
};
template<>
struct Serial_size<Position>{
    size_t operator()(const Position& val) const noexcept{
        return serial_size(val.y_,val.x_);
    }
};
template<>
struct Min_serial_size<Position>{
    using type = Position;
    static constexpr size_t value = []()
    {
        return sizeof(type::y_)+sizeof(type::x_);
    }();
};
template<>
struct Max_serial_size<Position>{
    using type = Position;
    static constexpr size_t value = []()
    {
        return sizeof(type::y_)+sizeof(type::x_);
    }();
};
}

template<>
struct std::hash<Position>{
    size_t operator()(const Position& val) const{
        return static_cast<size_t>(::to_integer(val.y_))<<sizeof(Y)|static_cast<size_t>(::to_integer(val.x_));
    }
};

#include "OsterLib/boost_functional/json.h"

template<>
boost::json::value to_json<Position>(const Position& coord);

template<>
std::expected<Position,std::exception> from_json<Position>(const boost::json::value& json);