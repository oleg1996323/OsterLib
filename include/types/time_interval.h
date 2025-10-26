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

class TimeInterval{
    utc_tp from_;
    utc_tp to_;
    template<bool NETWORK_ORDER>
    friend struct serialization::Deserialize;
    friend struct serialization::Max_serial_size<TimeInterval>;
    friend struct serialization::Min_serial_size<TimeInterval>;
    public:
    TimeInterval() = default;
    TimeInterval(utc_tp from,utc_tp to):from_(from<=to?from:to),to_(to>=from?to:from){}
    TimeInterval(const TimeInterval& other):from_(other.from_),to_(other.to_){}
    TimeInterval(TimeInterval&& other):from_(other.from_),to_(other.to_){}
    TimeInterval& operator=(const TimeInterval& other){
        if(this!=&other){
            from_ = other.from_;
            to_ = other.to_;
        }
        return *this;
    }
    TimeInterval& operator=(TimeInterval&& other){
        if(this!=&other){
            from_ = other.from_;
            to_ = other.to_;
        }
        return *this;
    }
    bool operator==(const TimeInterval& other) const;
    bool operator<(const TimeInterval& other) const;
    utc_tp from() const noexcept{
        return from_;
    }
    utc_tp to() const noexcept{
        return to_;
    }
};

#include <ranges>
#include <iostream>

class TimeSequence{
    TimeInterval interval_;
    utc_diff discret_ = utc_diff();
    template<bool NETWORK_ORDER>
    friend struct serialization::Serialize;
    template<bool NETWORK_ORDER>
    friend struct serialization::Deserialize;
    friend struct serialization::Serial_size<TimeSequence>;
    friend struct serialization::Max_serial_size<TimeSequence>;
    friend struct serialization::Min_serial_size<TimeSequence>;
    friend struct std::hash<TimeSequence>;
    friend struct std::less<TimeSequence>;
    friend struct std::equal_to<TimeSequence>;
    public:
    TimeSequence() = default;
    TimeSequence(utc_tp init_time):interval_(init_time,init_time){}
    TimeSequence(utc_tp from, utc_tp to,utc_diff discret):interval_(from,to),discret_(discret){
        if(discret==utc_diff::zero() || discret==from-to)
            return;
        if(std::abs(duration_cast<microseconds>(from + ((to-from)/duration<double>(discret))*discret - to).count()) > std::chrono::nanoseconds(1).count())
            throw std::invalid_argument("Invalid discretness");
    }
    TimeSequence(TimeInterval interval):interval_(interval),discret_(interval.to()-interval.from()){}
    TimeSequence(const TimeSequence& other):interval_(other.interval_),discret_(other.discret_){}
    TimeSequence(TimeSequence&& other):interval_(std::move(other.interval_)),discret_(other.discret_){}
    TimeSequence& operator=(const TimeSequence& other){
        if(this!=&other){
            discret_=other.discret_;
            interval_ = other.interval_;
        }
        return *this;
    }
    TimeSequence& operator=(TimeSequence&& other){
        if(this!=&other){
            discret_=std::move(other.discret_);
            interval_ = std::move(other.interval_);
        }
        return *this;
    }
    bool operator==(const TimeSequence& other) const;
    bool operator<(const TimeSequence& other) const;
    template<std::ranges::range RANGE>
    static std::pair<TimeSequence,typename std::decay_t<RANGE>::iterator> make_from_range(RANGE&& time_series) 
        requires (std::is_same_v<typename std::decay_t<decltype(time_series)>::value_type,utc_tp>){
        if(time_series.size()>0){
            utc_tp first = *time_series.begin();
            utc_tp last = first;
            if(time_series.size()>1){
                utc_diff discret_loc = *std::next(time_series.begin())-*time_series.begin();
                if(discret_loc<utc_diff())
                    throw std::invalid_argument("Unsorted range input");
                else if(discret_loc==utc_diff())
                    return std::make_pair(TimeSequence(first,last,discret_loc),std::next(time_series.begin(),2));
                else{
                    int count = 0;
                    for(auto tp=time_series.begin();tp!=time_series.end();++tp){
                        if(*tp==first+count++*discret_loc)
                            last = *tp;
                        else return std::make_pair(TimeSequence(first,last,discret_loc),tp);
                    }
                    return std::make_pair(TimeSequence(first,last,discret_loc),time_series.end());
                }
            }
            else return std::make_pair(TimeSequence(first,last,utc_diff::zero()),time_series.end());
        }
        else throw std::invalid_argument("Empty range input");
    }
    const TimeInterval get_interval() const noexcept{
        return interval_;
    }
    utc_diff discret() const noexcept{
        return discret_;
    }
    void reset(utc_tp time) noexcept{
        discret_ = utc_diff::zero();
        interval_ = TimeInterval(time,time);
    }
    bool push_time(utc_tp time) noexcept{
        if(push_time_before(time) || push_time_after(time))
            return true;
        else
            return false;
    }
    bool push_time_before(utc_tp time) noexcept{
        if(time<interval_.from() && ((interval_.from()-time)==discret_ || discret_==utc_diff::zero())){
            interval_=TimeInterval(time,interval_.to());
            discret_ = discret_!=discret_.zero()?discret_:interval_.to()-interval_.from();
            return true;
        }
        else return false;
        /* if(((time-interval_.from())%discret_)==utc_diff())
                    return true; */
    }
    bool push_time_after(utc_tp time) noexcept{
        if(time>interval_.to() && ((time - interval_.to())==discret_ || discret_==utc_diff::zero())){
            interval_=TimeInterval(interval_.from(),time);
            discret_ = discret_!=discret_.zero()?discret_:interval_.to()-interval_.from();
            return true;
        }
        else return false;
    }
    void increase_discrete(int multiplicator) noexcept{
        discret_*=multiplicator;
    }
    void decrease_discretness(int divider) noexcept{
        discret_/=divider;
    }
    bool extend_by_interval(TimeInterval interval) noexcept{
        if(extendable(TimeSequence(interval))){
            push_time(interval.from());
            push_time(interval.to());
            return true;
        }
        else return false;
    }
    bool extend_by_sequence(TimeSequence sequence) noexcept{
        return extend_by_interval(sequence.interval_);
    }
    bool extendable(const TimeSequence& other) const noexcept{
        if((other.interval_.to()-other.interval_.from()).count()%discret_.count()!=0)
            return false;
        if(std::abs((interval_.from()-other.interval_.from()).count())%discret_.count()==0)
            return true;
        else return false;
    }
};

template<>
struct std::hash<TimeInterval>{
    size_t operator()(const TimeInterval& val){
        return std::hash<size_t>{}(static_cast<size_t>(val.from().time_since_epoch().count())^(static_cast<size_t>(val.from().time_since_epoch().count()<<1)));
    }
};

template<>
struct std::equal_to<TimeInterval>{
    bool operator()(const TimeInterval& lhs,const TimeInterval& rhs) const{
        return lhs==rhs;
    }
};

template<>
struct std::less<TimeInterval>{
    bool operator()(const TimeInterval& lhs,const TimeInterval& rhs) const{
        return (lhs.from()<rhs.from())?true:(lhs.from()==rhs.from()?lhs.to()-lhs.from()<rhs.to()-rhs.from():false);
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
bool is_correct_interval(const TimeInterval& interval) noexcept;
bool intervals_intersect(const TimeInterval& lhs, const TimeInterval& rhs);
bool intervals_intersect(const utc_tp& from_1, const utc_tp& to_1,const utc_tp& from_2, const utc_tp& to_2);
std::optional<TimeInterval> interval_intersection(const TimeInterval&,const TimeInterval&) noexcept;
std::optional<std::pair<uint64_t,uint64_t>> interval_intersection_pos(const TimeInterval& to_seek, const TimeSequence& initial) noexcept;

namespace serialization{
    template<bool NETWORK_ORDER>
    struct Serialize<NETWORK_ORDER,TimeInterval>{
        auto operator()(const TimeInterval& val,std::vector<char>& buf) const noexcept{
            return serialize<NETWORK_ORDER>(val,buf,val.from(),val.to());
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
            return serial_size(val.from(),val.to());
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
