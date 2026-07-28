#pragma once
#include <chrono>
#include "OsterLib/serialization.h"

using MinTimeRange = std::chrono::duration<int64_t, std::ratio<3600L>>;

using namespace std::chrono;
using utc_tp = system_clock::time_point;
using utc_diff = std::chrono::system_clock::duration;

enum class TIME_UNITS{
    SECONDS,
    MINUTES,
    HOURS,
    DAYS,
    MONTHS,
    YEARS
};

struct TimePeriod{
    years years_ = years(0);
    months months_ = months(0);
    days days_ = days(0);
    hours hours_ = hours(0);
    minutes minutes_ = minutes(0);
    
    std::chrono::seconds seconds_ = std::chrono::seconds(0);
    template<bool NETWORK_ORDER>
    friend struct serialization::Serialize;
    template<bool NETWORK_ORDER>
    friend struct serialization::Deserialize;
    friend struct serialization::Serial_size<TimePeriod>;
    friend struct serialization::Max_serial_size<TimePeriod>;
    friend struct serialization::Min_serial_size<TimePeriod>;
    public:
    TimePeriod() noexcept = default;
    explicit TimePeriod(years y,months mo,days d,hours h,minutes m,std::chrono::seconds s) noexcept;
    TimePeriod(const TimePeriod& other) noexcept;
    TimePeriod(TimePeriod&& other) noexcept;
    TimePeriod& operator=(const TimePeriod& other) noexcept;
    TimePeriod& operator=(TimePeriod&& other) noexcept;

    /**
     * @return Next time-point from current time-point by set time-offset
     */
    utc_tp get_next_tp(utc_tp current_time) const noexcept;
    /**
     * @return The time-point aligned to the lesser defined time-unit
     */
    utc_tp get_null_aligned_tp(utc_tp current_time, utc_tp from_initial_interval) const noexcept;

    inline bool is_set() const noexcept{
        return years_!=years(0) || months_!=months(0) || days_!=days(0) ||
                hours_!=hours(0) || minutes_!=minutes(0) || seconds_!=std::chrono::seconds(0);
    }

    TIME_UNITS min_valuable() const noexcept{
        if(seconds_>std::chrono::seconds())
            return TIME_UNITS::SECONDS;
        else if (minutes_>minutes())
            return TIME_UNITS::MINUTES;
        else if (hours_>hours())
            return TIME_UNITS::HOURS;
        else if (days_>days())
            return TIME_UNITS::DAYS;
        else if(months_>months())
            return TIME_UNITS::MONTHS;
        else if(years_>years())
            return TIME_UNITS::YEARS;
        else return TIME_UNITS::SECONDS;
    }
};

inline utc_tp operator+(const utc_tp& tp, const TimePeriod& period) noexcept{
    return tp+period.years_+period.months_+period.days_+period.hours_+period.minutes_+period.seconds_;
}

inline utc_tp operator+(const TimePeriod& period,const utc_tp& tp) noexcept{
    return tp+period.years_+period.months_+period.days_+period.hours_+period.minutes_+period.seconds_;
}

inline utc_tp operator-(const utc_tp& tp, const TimePeriod& period) noexcept{
    return tp-period.years_-period.months_-period.days_-period.hours_-period.minutes_-period.seconds_;
}

inline utc_tp operator-(const TimePeriod& period,const utc_tp& tp) noexcept{
    return tp-period.years_-period.months_-period.days_-period.hours_-period.minutes_-period.seconds_;
}

namespace serialization{

    template<bool NETWORK_ORDER>
    struct Serialize<NETWORK_ORDER,TimePeriod>{
        auto operator()(const TimePeriod& val,std::vector<char>& buf) const noexcept{
            return serialize<NETWORK_ORDER>(val,buf,val.years_,val.months_,val.days_,val.hours_,val.minutes_,val.seconds_);
        }
    };

    template<bool NETWORK_ORDER>
    struct Deserialize<NETWORK_ORDER,TimePeriod>{
        auto operator()(TimePeriod& val,StreamSerializer& buf) const noexcept{
            return deserialize<NETWORK_ORDER>(val,buf,val.years_,val.months_,val.days_,val.hours_,val.minutes_,val.seconds_);
        }
    };

    template<>
    struct Serial_size<TimePeriod>{
        size_t operator()(const TimePeriod& val) const noexcept{
            return serial_size(val.years_,val.months_,val.days_,val.hours_,val.minutes_,val.seconds_);
        }
    };

    template<>
    struct Min_serial_size<TimePeriod>{
        using type = TimePeriod;
        static constexpr size_t value = []()
        {
            return min_serial_size<decltype(type::years_),decltype(type::months_),decltype(type::days_),decltype(type::hours_),decltype(type::minutes_),decltype(type::seconds_)>();
        }();
    };
     
    template<>
    struct Max_serial_size<TimePeriod>{
        using type = TimePeriod;
        static constexpr size_t value = []()
        {
            return max_serial_size<decltype(type::years_),decltype(type::months_),decltype(type::days_),decltype(type::hours_),decltype(type::minutes_),decltype(type::seconds_)>();
        }();
    };
}

#include "OsterLib/boost_functional/json.h"
#include <boost/lexical_cast.hpp>
#include <expected>

template<>
boost::json::value to_json(const TimePeriod& val);

template<>
std::expected<TimePeriod,std::exception> from_json(const boost::json::value& val);

template<>
TimePeriod boost::lexical_cast(const std::string& input);
template<>
std::string boost::lexical_cast(const TimePeriod& input);