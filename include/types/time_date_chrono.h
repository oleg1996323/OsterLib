#pragma once
#include <chrono>
#include "concepts.h"

using MinTimeRange = std::chrono::duration<int64_t, std::ratio<3600L>>;

using namespace std::chrono;
template<typename DURATION = std::chrono::nanoseconds>
using utc_tp_t = time_point<std::chrono::system_clock,DURATION>;
using utc_tp = utc_tp_t<>;
template<typename DURATION = std::chrono::nanoseconds>
using utc_diff_t = std::chrono::duration<typename DURATION::rep,typename DURATION::period>;
using utc_diff = utc_diff_t<>;

struct TimeDateDuration{
    utc_diff_t<std::chrono::seconds> duration_;
    using rep = uint64_t;
    using period = utc_diff_t<std::chrono::seconds>::period;
    
    template<IsDuration OTHER_DURATION>
    static std::chrono::years years(OTHER_DURATION other){
        return std::chrono::floor<years>(other);
    }

    template<IsDuration OTHER_DURATION>
    static std::chrono::months months(OTHER_DURATION other){
        if(auto mo = floor<std::chrono::months>(other)-std::chrono::months(years(other).count()*12);mo.count()>0)
            return mo;
        else return std::chrono::months();
    }

    template<IsDuration OTHER_DURATION>
    static std::chrono::days days(OTHER_DURATION other){
        using ratio_t = std::ratio_divide<std::chrono::months::period,std::chrono::days::period>;
        if(auto d = floor<std::chrono::days>(other)-std::chrono::days(floor<std::chrono::months>(other).count()*ratio_t::num/ratio_t::den);d.count()>0)
            return d;
        else return std::chrono::days();
    }

    template<IsDuration OTHER_DURATION>
    static std::chrono::hours hours(OTHER_DURATION other){
        if(auto h = floor<std::chrono::hours>(other)-std::chrono::hours(floor<std::chrono::days>(other).count()*24);h.count()>0)
            return h;
        else return std::chrono::hours();
    }

    template<IsDuration OTHER_DURATION>
    static std::chrono::minutes minutes(OTHER_DURATION other){
        if(auto m = floor<std::chrono::minutes>(other)-std::chrono::minutes(floor<std::chrono::hours>(other).count()*60);m.count()>0)
            return m;
        else return std::chrono::minutes();
    }

    template<IsDuration OTHER_DURATION>
    static std::chrono::seconds seconds(OTHER_DURATION other){
        if(auto s = floor<std::chrono::seconds>(other)-std::chrono::seconds(floor<std::chrono::minutes>(other).count()*60);s.count()>0)
            return s;
        else return std::chrono::seconds();
    }

    TimeDateDuration& operator+=(const TimeDateDuration& other){
        duration_ += other.duration_;
        return *this;
    }
    TimeDateDuration& operator-=(const TimeDateDuration& other){
        duration_ -= other.duration_;
        return *this;
    }
    TimeDateDuration& operator%(const TimeDateDuration& other){
        duration_%other.duration_;
        return *this;
    }
    bool operator==(const TimeDateDuration& other) const{
        duration_ == other.duration_;
    }
    bool operator!=(const TimeDateDuration& other) const{
        duration_ != other.duration_;
    }
    bool operator>(const TimeDateDuration& other) const{
        return duration_ > other.duration_;
    }
    bool operator>=(const TimeDateDuration& other) const{
        return duration_ >= other.duration_;
    }
    bool operator<=(const TimeDateDuration& other) const{
        return duration_ <= other.duration_;
    }
    bool operator<(const TimeDateDuration& other) const{
        return duration_ < other.duration_;
    }
    TimeDateDuration& operator=(const TimeDateDuration& other){
        duration_ = other.duration_;
        return *this;
    }
    TimeDateDuration& operator=(TimeDateDuration&& other){
        duration_ = std::move(other.duration_);
        return *this;
    }
};  

struct TimeDateClock {
    using duration = TimeDateDuration;
    using rep = duration::rep;
    using period = duration::period;
    using TimeDate = std::chrono::time_point<TimeDateClock>;
    
    static constexpr bool is_steady = std::chrono::system_clock::is_steady;
    
    static TimeDate now() noexcept;
};

using TimeDate = std::chrono::time_point<TimeDateClock>;

TimeDate& operator+(const TimeDate& tp,const TimeDateDuration& dur){
    std::chrono::year_month_day ymd(std::chrono::sys_days(tp.time_since_epoch().duration_.count()));

}

TimeDate& operator+(const TimeDateDuration& tp,const TimeDate& dur){
    
}

void foo(){
    TimeDateDuration dur;
    TimeDate td;
    td+dur;
}

TimeDate TimeDateClock::now() noexcept {
    return TimeDate(TimeDateDuration(std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch())));
}