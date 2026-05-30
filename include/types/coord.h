#pragma once
#include "serialization.h"
#include "float_conv.h"
typedef float Lat;
typedef float Lon;

struct Coord
{
    Lat lat_ = -999.;
    Lon lon_ = -999.;

    bool is_correct_pos() const{
        if(lon_>=0 && lon_<=180 && lat_<=90 && lat_>=-90)
            return true;
        else return false;
    }

    bool operator==(const Coord& other) const{
        return lat_ == other.lat_ && lon_ == other.lon_;
    }

    bool operator!=(const Coord& other) const{
        return !operator==(other);
    }
};

struct RawCoord
{
    uint32_t lat_ = -999;
    uint32_t lon_ = -999;
};

extern bool is_correct_pos(const Coord* pos);
bool is_correct_pos(const Coord& pos);

namespace serialization{

template<bool NETWORK_ORDER>
struct Serialize<NETWORK_ORDER,Coord>{
    SerializationEC operator()(const Coord& pos, std::vector<char>& buf) const noexcept{
        return serialize<NETWORK_ORDER>(pos,buf,pos.lat_,pos.lon_);
    }
};

template<bool NETWORK_ORDER>
struct Deserialize<NETWORK_ORDER,Coord>{
    SerializationEC operator()(Coord& pos,StreamSerializer& buf) const noexcept{
        return deserialize<NETWORK_ORDER>(pos,buf,pos.lat_,pos.lon_);
    }
};
template<>
struct Serial_size<Coord>{
    size_t operator()(const Coord& val) const noexcept{
        return serial_size(val.lat_,val.lon_);
    }
};
template<>
struct Min_serial_size<Coord>{
    using type = Coord;
    static constexpr size_t value = []()
    {
        return sizeof(type::lat_)+sizeof(type::lon_);
    }();
};
template<>
struct Max_serial_size<Coord>{
    using type = Coord;
    static constexpr size_t value = []()
    {
        return sizeof(type::lat_)+sizeof(type::lon_);
    }();
};
}

template<>
struct std::hash<Coord>{
    size_t operator()(const Coord& val) const{
        return static_cast<size_t>(::to_integer(val.lat_))<<sizeof(Lat)|static_cast<size_t>(::to_integer(val.lon_));
    }
};

#include "boost_functional/json.h"

template<>
boost::json::value to_json<Coord>(const Coord& coord);

template<>
std::expected<Coord,std::exception> from_json<Coord>(const boost::json::value& json);