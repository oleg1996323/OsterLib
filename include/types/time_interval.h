#pragma once
#include <stdbool.h>
#include <chrono>
#include <unordered_map>
#include "byte_order.h"
#include "serialization.h"

using MinTimeRange = std::chrono::duration<int64_t, std::ratio<3600L>>;

using namespace std::chrono;
using utc_tp = system_clock::time_point;
using utc_diff = std::chrono::system_clock::duration;

struct TimeInterval{
    utc_tp from_;
    utc_tp to_;

    bool operator==(const TimeInterval& other) const;
    bool operator<(const TimeInterval& other) const;
};

struct TimeSequence{
    TimeInterval interval_;
    utc_diff discret_;

    bool operator==(const TimeSequence& other) const;
    bool operator<(const TimeSequence& other) const;
};

template<>
struct std::hash<TimeInterval>{
    size_t operator()(const TimeInterval& val){
        return std::hash<size_t>{}(static_cast<size_t>(val.from_.time_since_epoch().count())^(static_cast<size_t>(val.from_.time_since_epoch().count()<<1)));
    }
};

template<>
struct std::equal_to<TimeInterval>{
    bool operator()(const TimeInterval& lhs,const TimeInterval& rhs) const{
        return lhs.from_==rhs.from_ && lhs.to_==rhs.to_;
    }
};

template<>
struct std::less<TimeInterval>{
    bool operator()(const TimeInterval& lhs,const TimeInterval& rhs) const{
        return (lhs.from_<rhs.from_)?true:(lhs.from_==rhs.from_?lhs.to_-lhs.from_<rhs.to_-rhs.from_:false);
    }
};

template<>
struct std::hash<TimeSequence>{
    size_t operator()(const TimeSequence& val){
        return std::hash<TimeInterval>{}(val.interval_)^(static_cast<size_t>(val.discret_.count()<<1));
    }
};

template<>
struct std::equal_to<TimeSequence>{
    bool operator()(const TimeSequence& lhs,const TimeSequence& rhs) const{
        return lhs.interval_==rhs.interval_;
    }
};

template<>
struct std::less<TimeSequence>{
    bool operator()(const TimeSequence& lhs,const TimeSequence& rhs) const{
        return lhs.interval_<rhs.interval_;
    }
};

bool is_correct_interval(const utc_tp& from,const utc_tp& to);
bool intervals_intersect(const TimeInterval& lhs, const TimeInterval& rhs);
bool intervals_intersect(const utc_tp& from_1, const utc_tp& to_1,const utc_tp& from_2, const utc_tp& to_2);
std::optional<TimeInterval> interval_instersection(const TimeInterval&,const TimeInterval&) noexcept;
std::pair<uint16_t,uint16_t> interval_intersection_pos(const TimeInterval& to_seek, const TimeInterval& initial, const utc_diff& discret) noexcept;

namespace serialization{
    template<bool NETWORK_ORDER>
    struct Serialize<NETWORK_ORDER,TimeInterval>{
        auto operator()(const TimeInterval& val,std::vector<char>& buf) const noexcept{
            return serialize<NETWORK_ORDER>(val,buf,val.from_,val.to_);
        }
    };

    template<bool NETWORK_ORDER>
    struct Deserialize<NETWORK_ORDER,TimeInterval>{
        auto operator()(TimeInterval& val,std::span<const char> buf) const noexcept{
            return deserialize<NETWORK_ORDER>(val,buf,val.from_,val.to_);
        }
    };

    template<>
    struct Serial_size<TimeInterval>{
        size_t operator()(const TimeInterval& val) const noexcept{
            return serial_size(val.from_,val.to_);
        }
    };

    template<>
    struct Min_serial_size<TimeInterval>{
        using type = TimeInterval;
        static constexpr size_t value = []()
        {
            return min_serial_size<decltype(type::from_),decltype(type::to_)>();
        }();
    };
     
    template<>
    struct Max_serial_size<TimeInterval>{
        using type = TimeInterval;
        static constexpr size_t value = []()
        {
            return max_serial_size<decltype(type::from_),decltype(type::to_)>();
        }();
    };

    template<bool NETWORK_ORDER>
    struct Serialize<NETWORK_ORDER,TimeSequence>{
        auto operator()(const TimeSequence& val,std::vector<char>& buf) const noexcept{
            return serialize<NETWORK_ORDER>(val,buf,val.interval_,val.discret_);
        }
    };

    template<bool NETWORK_ORDER>
    struct Deserialize<NETWORK_ORDER,TimeSequence>{
        auto operator()(TimeSequence& val,std::span<const char> buf) const noexcept{
            return deserialize<NETWORK_ORDER>(val,buf,val.interval_,val.discret_);
        }
    };

    template<>
    struct Serial_size<TimeSequence>{
        size_t operator()(const TimeSequence& val) const noexcept{
            return serial_size(val.interval_,val.discret_);
        }
    };

    template<>
    struct Min_serial_size<TimeSequence>{
        using type = TimeSequence;
        static constexpr size_t value = []()
        {
            return min_serial_size<decltype(type::interval_),decltype(type::discret_)>();
        }();
    };
     
    template<>
    struct Max_serial_size<TimeSequence>{
        using type = TimeSequence;
        static constexpr size_t value = []()
        {
            return max_serial_size<decltype(type::interval_),decltype(type::discret_)>();
        }();
    };
}

#include "boost_functional/json.h"
#include "boost/lexical_cast.hpp"
#include <expected>

template<>
utc_tp boost::lexical_cast(const std::string& input);
template<>
std::string boost::lexical_cast(const utc_tp& input);
