#pragma once
#include <stdbool.h>
#include <chrono>
#include <unordered_map>
#include "byte_order.h"
#include "serialization.h"
#include <ctime>
#include <cmath>
#include <iostream>

using MinTimeRange = std::chrono::duration<int64_t, std::ratio<3600L>>;

using namespace std::chrono;
template<typename DURATION = std::chrono::nanoseconds>
using utc_tp_t = time_point<std::chrono::system_clock,DURATION>;
using utc_tp = utc_tp_t<>;
template<typename DURATION = std::chrono::nanoseconds>
using utc_diff_t = std::chrono::duration<typename DURATION::rep,typename DURATION::period>;
using utc_diff = utc_diff_t<>;

namespace chrono_func{
template<IsDuration OTHER_DURATION>
static std::chrono::years years(OTHER_DURATION other){
    return std::chrono::floor<std::chrono::years>(other);
}

template<IsDuration OTHER_DURATION>
std::chrono::months months(OTHER_DURATION other){
    if(auto mo = floor<std::chrono::months>(other)-std::chrono::months(years(other).count()*12);mo.count()>0)
        return mo;
    else return std::chrono::months();
}

template<IsDuration OTHER_DURATION>
std::chrono::days days(OTHER_DURATION other){
    using ratio_t = std::ratio_divide<std::chrono::months::period,std::chrono::days::period>;
    if(auto d = floor<std::chrono::days>(other)-std::chrono::days(floor<std::chrono::months>(other).count()*ratio_t::num/ratio_t::den);d.count()>0)
        return d;
    else return std::chrono::days();
}

template<IsDuration OTHER_DURATION>
std::chrono::hours hours(OTHER_DURATION other){
    if(auto h = floor<std::chrono::hours>(other)-std::chrono::hours(floor<std::chrono::days>(other).count()*24);h.count()>0)
        return h;
    else return std::chrono::hours();
}

template<IsDuration OTHER_DURATION>
std::chrono::minutes minutes(OTHER_DURATION other){
    if(auto m = floor<std::chrono::minutes>(other)-std::chrono::minutes(floor<std::chrono::hours>(other).count()*60);m.count()>0)
        return m;
    else return std::chrono::minutes();
}

template<IsDuration OTHER_DURATION>
std::chrono::seconds seconds(OTHER_DURATION other){
    if(auto s = floor<std::chrono::seconds>(other)-std::chrono::seconds(floor<std::chrono::minutes>(other).count()*60);s.count()>0)
        return s;
    else return std::chrono::seconds();
}
}

template<IsDuration DUR_PRECISION = utc_diff>
class __time_interval__{
    using duration_t = DUR_PRECISION;
    using time_point_t = utc_tp_t<DUR_PRECISION>;
    utc_tp_t<DUR_PRECISION> from_;
    utc_tp_t<DUR_PRECISION> to_;
    template<bool NETWORK_ORDER>
    friend struct serialization::Deserialize;
    friend struct serialization::Max_serial_size<__time_interval__>;
    friend struct serialization::Min_serial_size<__time_interval__>;
    public:
    __time_interval__() = default;

    template<IsTimePoint ARG1_TP,IsTimePoint ARG2_TP>
    __time_interval__(ARG1_TP&& from,ARG2_TP&& to):from_(time_point_cast<DUR_PRECISION>(from<=to?from:to)),to_(time_point_cast<DUR_PRECISION>(to>=from?to:from)){}

    template<IsDuration OTHER_DUR>
    __time_interval__(const __time_interval__<OTHER_DUR>& other):from_(time_point_cast<DUR_PRECISION>(other.from())),to_(time_point_cast<DUR_PRECISION>(other.to())){}

    template<IsDuration OTHER_DUR>
    __time_interval__(__time_interval__<OTHER_DUR>&& other):from_(time_point_cast<DUR_PRECISION>(other.from())),to_(time_point_cast<DUR_PRECISION>(other.to())){}

    template<IsDuration OTHER_DUR>
    __time_interval__<DUR_PRECISION>& operator=(const __time_interval__<OTHER_DUR>& other){
        if constexpr(std::is_same_v<DUR_PRECISION,OTHER_DUR>){
            if(this!=&other){
                from_ = other.from();
                to_ = other.to();
            }
        }
        else{
            from_ = time_point_cast<DUR_PRECISION>(other.from());
            to_ = time_point_cast<DUR_PRECISION>(other.to());
        }
        return *this;
    }
    template<IsDuration OTHER_DUR>
    __time_interval__<DUR_PRECISION>& operator=(__time_interval__<OTHER_DUR>&& other){
        if constexpr(std::is_same_v<DUR_PRECISION,OTHER_DUR>){
            if(this!=&other){
                from_ = other.from();
                to_ = other.to();
            }
        }
        else{
            from_ = time_point_cast<DUR_PRECISION>(other.from());
            to_ = time_point_cast<DUR_PRECISION>(other.to());
        }
        return *this;
    }
    template<IsDuration OTHER_DUR>
    bool operator==(const __time_interval__<OTHER_DUR>& other) const{
        return from_==other.from() && to_==other.to();
    }
    template<IsDuration OTHER_DUR>
    bool operator<(const __time_interval__<OTHER_DUR>& other) const{
        return (from_<other.from())?true:(from_==other.from()?to_-from_<other.to()-other.from():false);
    }
    utc_tp_t<DUR_PRECISION> from() const noexcept{
        return from_;
    }
    utc_tp_t<DUR_PRECISION> to() const noexcept{
        return to_;
    }
    template<IsDuration INTERVAL_DUR>
    bool contains(const __time_interval__<INTERVAL_DUR>& other) const{
        return from_<=other.from() && to_>=other.to();
    }
};

#include <ranges>

#include <stdfloat>

struct DateTimeDiff{
    int16_t years_ = 0;
    int8_t months_ = 0;
    int8_t days_ = 0;
    int8_t hours_ = 0;
    int8_t minutes_ = 0;
    int8_t seconds_ = 0;

    public:
    static bool isLeapYear(uint16_t year) {
        return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
    }

    static uint8_t getDaysInMonth(std::chrono::year year, std::chrono::month month) {
        static const std::array<uint8_t, 12> days_in_month = {
            31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
        };
        
        if (static_cast<unsigned>(month) == 2 && year.is_leap()) {
            return 29;
        }
        return days_in_month[static_cast<unsigned>(month) - 1];
    }

    static uint8_t getDaysInMonth(uint8_t year, uint8_t month){
        return getDaysInMonth(std::chrono::year(year),std::chrono::month(month));
    }

    static uint64_t days_between_dates(std::chrono::year_month_day ymd_from, std::chrono::year_month_day ymd_to){
        if (ymd_to < ymd_from) {
            return static_cast<uint64_t>((std::chrono::sys_days(ymd_from)-std::chrono::sys_days(ymd_to)).count());
        }
        else return static_cast<uint64_t>((std::chrono::sys_days(ymd_to)-std::chrono::sys_days(ymd_from)).count());
    }

    DateTimeDiff() noexcept = default;

    explicit DateTimeDiff(   std::error_code& err,std::chrono::years years,
                    std::chrono::months months = std::chrono::months(),
                    std::chrono::days days = std::chrono::days(),
                    std::chrono::hours hours = std::chrono::hours(),
                    std::chrono::minutes minutes = std::chrono::minutes(),
                    std::chrono::seconds seconds = std::chrono::seconds()) noexcept
            {
                if(years.count()>std::numeric_limits<decltype(years_)>::max() || years.count()<std::numeric_limits<decltype(years_)>::min())
                    err = std::make_error_code(std::errc::invalid_argument);
                else
                    years_=static_cast<int16_t>(years.count());
                if(months.count()>std::numeric_limits<decltype(months_)>::max() || months.count()<std::numeric_limits<decltype(months_)>::min())
                    err = std::make_error_code(std::errc::invalid_argument);
                else
                    months_=static_cast<decltype(months_)>(months.count());
                if(days.count()>std::numeric_limits<decltype(days_)>::max() || days.count()<std::numeric_limits<decltype(days_)>::min())
                    err = std::make_error_code(std::errc::invalid_argument);
                else
                    days_=static_cast<decltype(days_)>(days.count());
                if(hours.count()>std::numeric_limits<decltype(hours_)>::max() || hours.count()<std::numeric_limits<decltype(hours_)>::min())
                    err = std::make_error_code(std::errc::invalid_argument);
                else
                    hours_=static_cast<decltype(hours_)>(hours.count());
                if(minutes.count()>std::numeric_limits<decltype(minutes_)>::max() || minutes.count()<std::numeric_limits<decltype(minutes_)>::min())
                    err = std::make_error_code(std::errc::invalid_argument);
                else
                    minutes_=static_cast<decltype(minutes_)>(minutes.count());
                if(seconds.count()>std::numeric_limits<decltype(seconds_)>::max() || seconds.count()<std::numeric_limits<decltype(seconds_)>::min())
                    err = std::make_error_code(std::errc::invalid_argument);
                else
                    seconds_=static_cast<decltype(seconds_)>(seconds.count());
            }
    explicit DateTimeDiff(std::error_code& err,std::chrono::months months,
                std::chrono::days days = std::chrono::days(),
                std::chrono::hours hours = std::chrono::hours(),
                std::chrono::minutes minutes = std::chrono::minutes(),
                std::chrono::seconds seconds = std::chrono::seconds()) noexcept:
                DateTimeDiff(err,std::chrono::years(0),months,days,hours,minutes,seconds){}
    explicit DateTimeDiff(std::error_code& err,std::chrono::days days,
            std::chrono::hours hours = std::chrono::hours(),
            std::chrono::minutes minutes = std::chrono::minutes(),
            std::chrono::seconds seconds = std::chrono::seconds()) noexcept:
            DateTimeDiff(err,std::chrono::years(0),std::chrono::months(0),days,hours,minutes,seconds){}

    explicit DateTimeDiff(std::error_code& err,std::chrono::hours hours,
        std::chrono::minutes minutes = std::chrono::minutes(),
        std::chrono::seconds seconds = std::chrono::seconds()) noexcept:
        DateTimeDiff(err,std::chrono::years(0),std::chrono::months(0),std::chrono::days(0),hours,minutes,seconds){}

    explicit DateTimeDiff(std::error_code& err,std::chrono::minutes minutes,
        std::chrono::seconds seconds = std::chrono::seconds()) noexcept:
        DateTimeDiff(err,std::chrono::years(0),std::chrono::months(0),std::chrono::days(0),std::chrono::hours(0),minutes,seconds){}

    explicit DateTimeDiff(std::error_code& err,std::chrono::seconds seconds) noexcept:
        DateTimeDiff(err,std::chrono::years(0),std::chrono::months(0),std::chrono::days(0),std::chrono::hours(0),std::chrono::minutes(0),seconds){}

    template<IsTimePoint ARG1_TP,IsTimePoint ARG2_TP>
    DateTimeDiff(ARG1_TP&& from,ARG2_TP&& to){
        using namespace std::string_literals;
        std::chrono::year_month_day ymd_from(std::chrono::floor<std::chrono::days>(from)),
                                    ymd_to(std::chrono::floor<std::chrono::days>(to));
        int diff = 0;
        if(diff = static_cast<unsigned>(ymd_to.day())-static_cast<unsigned>(ymd_from.day());diff>std::numeric_limits<decltype(days_)>::max() || diff<std::numeric_limits<decltype(days_)>::min()){
            throw std::invalid_argument("Months-diff value overflow. Must be "s+
                            std::to_string(std::numeric_limits<decltype(days_)>::min())+
                            "<=[VAL]]<="+std::to_string(std::numeric_limits<decltype(days_)>::max()));
        }
        else{
            if(diff<0){
                days_ =diff+getDaysInMonth(ymd_from.year(),ymd_from.month());
                months_-=1;
            }
            else days_ =static_cast<decltype(days_)>(diff);
        }
        if(diff = static_cast<unsigned>(ymd_to.month())-static_cast<unsigned>(ymd_from.month());diff>std::numeric_limits<decltype(months_)>::max() || diff<std::numeric_limits<decltype(months_)>::min()){
            throw std::invalid_argument("Months-diff value overflow. Must be "s+
                            std::to_string(std::numeric_limits<decltype(months_)>::min())+
                            "<=[VAL]]<="+std::to_string(std::numeric_limits<decltype(months_)>::max()));
        }
        else {
            if(diff<0){
                months_ += diff+12;
                years_-=1;
            }
            else
                months_ =static_cast<decltype(months_)>(diff);
        }
        if(diff = static_cast<int>(ymd_to.year())-static_cast<int>(ymd_from.year());diff>std::numeric_limits<decltype(years_)>::max() || diff<std::numeric_limits<decltype(years_)>::min())
            throw std::invalid_argument("Years-diff value overflow. Must be "s+
                            std::to_string(std::numeric_limits<decltype(years_)>::min())+
                            "<=[VAL]]<="+std::to_string(std::numeric_limits<decltype(years_)>::max()));
        else years_ += static_cast<decltype(years_)>(diff);
        hours_ = std::chrono::duration_cast<std::chrono::hours>((to-std::chrono::floor<std::chrono::days>(to))-
                    (from-std::chrono::floor<std::chrono::days>(from))).count();
        minutes_ = std::chrono::duration_cast<std::chrono::minutes>((to-std::chrono::floor<std::chrono::hours>(to))-
                    (from-std::chrono::floor<std::chrono::hours>(from))).count();
        seconds_ = std::chrono::duration_cast<std::chrono::seconds>((to-std::chrono::floor<std::chrono::minutes>(to))-
                    (from-std::chrono::floor<std::chrono::minutes>(from))).count();
        // std::cout<<"Resulted duration from constructor:\nyears:"<<static_cast<int>(years_)<<
        //             "\nmonths:"<<static_cast<int>(months_)<<"\ndays:"<<static_cast<int>(days_)<<
        //             "\nhours:"<<static_cast<int>(hours_)<<"\nminutes"<<static_cast<int>(minutes_)<<
        //             "\nseconds"<<static_cast<int>(seconds_)<<std::endl;
    }

    template<IsTimePoint ARG1_TP,IsTimePoint ARG2_TP>
    DateTimeDiff(ARG1_TP&& from,ARG2_TP&& to, uint32_t number_of_intervals, std::error_code& err){
        using namespace std::string_literals;
        if(from==to){
            if(number_of_intervals>0)
                err = std::make_error_code(std::errc::invalid_argument);
            return;
        }
        if(from!=to && number_of_intervals==0){
            err = std::make_error_code(std::errc::invalid_argument);
            return;
        }

        auto is_integer = [](const auto& number){
            bool is = std::fmod(std::fabs(number),1)<std::numeric_limits<std::decay_t<decltype(number)>>::epsilon();
            return is;
        };

        std::chrono::year_month_day ymd_from(std::chrono::floor<std::chrono::days>(from)),
                                    ymd_to(std::chrono::floor<std::chrono::days>(to));
        double diff = 0;
        {
            auto from_time = from-std::chrono::floor<std::chrono::days>(from);
            auto to_time = to-std::chrono::floor<std::chrono::days>(to);
            if(from_time>to_time){
                diff = static_cast<double>((to_time-from_time+std::chrono::hours(24)).count())/number_of_intervals;
                --days_;
            }
            else 
                diff = static_cast<double>((to_time-from_time).count())/number_of_intervals;
        }
        if(!is_integer(diff)){
            err = std::make_error_code(std::errc::invalid_argument);
            return;
        }
        else{
            hours_ = static_cast<decltype(hours_)>(diff/3600);
            minutes_ = static_cast<decltype(minutes_)>((diff-hours_*3600)/60);
            seconds_ = static_cast<decltype(minutes_)>(diff-hours_*3600-minutes_*60);
        }       
        if(ymd_to.day()<ymd_from.day()){
            diff = static_cast<double>((ymd_to.day()-ymd_from.day()+std::chrono::days(getDaysInMonth(ymd_from.year(),ymd_from.month())+days_)).count())/number_of_intervals;
            --months_;
        }
        else diff = static_cast<double>((ymd_to.day()-ymd_from.day()).count()+days_)/number_of_intervals;

        if(diff>std::numeric_limits<decltype(days_)>::max() || diff<std::numeric_limits<decltype(days_)>::min()){
            err = std::make_error_code(std::errc::invalid_argument);
            return;
                        // throw std::invalid_argument("Months-diff value overflow. Must be "s+
            //                 std::to_string(std::numeric_limits<decltype(days_)>::min())+
            //                 "<=[VAL]]<="+std::to_string(std::numeric_limits<decltype(days_)>::max()));
        }
        else days_ =static_cast<decltype(days_)>(diff);
        if(diff = static_cast<double>((ymd_to.month()-ymd_from.month()).count()+months_)/number_of_intervals;
                        diff>std::numeric_limits<decltype(months_)>::max() || diff<std::numeric_limits<decltype(months_)>::min()){
            err = std::make_error_code(std::errc::invalid_argument);
            return;
            // throw std::invalid_argument("Months-diff value overflow. Must be "s+
            //                 std::to_string(std::numeric_limits<decltype(months_)>::min())+
            //                 "<=[VAL]]<="+std::to_string(std::numeric_limits<decltype(months_)>::max()));
        }
        else {
            if(ymd_to.month()<ymd_from.month()){
                months_ = static_cast<decltype(months_)>(diff);
                --years_;
            }
            else months_ =static_cast<decltype(months_)>(diff);

            if(double days_tmp = std::fmod(diff,1)*getDaysInMonth(ymd_from.year(),ymd_from.month());!is_integer(days_tmp)){
                err = std::make_error_code(std::errc::invalid_argument);
                return;
            }
            else days_+=static_cast<decltype(days_)>(days_tmp);
        }
        if(diff = static_cast<double>((ymd_to.year()-ymd_from.year()).count()+years_)/number_of_intervals;
                        diff>std::numeric_limits<decltype(years_)>::max() || diff<std::numeric_limits<decltype(years_)>::min()){
            err = std::make_error_code(std::errc::invalid_argument);
            return;
        }
            // throw std::invalid_argument("Years-diff value overflow. Must be "s+
            //                 std::to_string(std::numeric_limits<decltype(years_)>::min())+
            //                 "<=[VAL]]<="+std::to_string(std::numeric_limits<decltype(years_)>::max()));
        else {
            years_ = static_cast<decltype(years_)>(diff);
            if(double months_tmp = std::fmod(diff,1)*12;!is_integer(months_tmp)){
                err = std::make_error_code(std::errc::invalid_argument);
                return;
            }
            else months_+=static_cast<decltype(months_)>(months_tmp);
        }



        // std::cout<<"Resulted duration from constructor:\nyears:"<<static_cast<int>(years_)<<
        //             "\nmonths:"<<static_cast<int>(months_)<<"\ndays:"<<static_cast<int>(days_)<<
        //             "\nhours:"<<static_cast<int>(hours_)<<"\nminutes"<<static_cast<int>(minutes_)<<
        //             "\nseconds"<<static_cast<int>(seconds_)<<std::endl;
    }

    DateTimeDiff(const DateTimeDiff& other) noexcept:years_(other.years_),months_(other.months_),days_(other.days_),hours_(other.hours_),minutes_(other.minutes_),seconds_(other.seconds_){}
    DateTimeDiff(DateTimeDiff&& other) noexcept:years_(other.years_),months_(other.months_),days_(other.days_),hours_(other.hours_),minutes_(other.minutes_),seconds_(other.seconds_){}

    template<IsTimePoint ARG1_TP>
    std::decay_t<ARG1_TP> operator+(ARG1_TP&& tp) const {
        std::chrono::year_month_day ymd_tp(std::chrono::floor<std::chrono::days>(tp));
        return std::chrono::seconds(seconds_)+std::chrono::sys_days(std::chrono::year_month_day(   ymd_tp.year()+std::chrono::years(years_),
                                                                    ymd_tp.month()+std::chrono::months(months_),
                                                                    ymd_tp.day()+std::chrono::days(days_)));
    }

    bool operator==(const DateTimeDiff& other) const noexcept{
        return years_==other.years_ && months_==other.months_ && days_ == other.days_ &&
                hours_==other.hours_ && minutes_ == other.minutes_ && seconds_ == other.seconds_;
    }

    bool operator!=(const DateTimeDiff& other) const noexcept{
        return !(*this==other);
    }

    bool operator<(const DateTimeDiff& other) const noexcept{
        return !(*this>=other);
    }
    bool operator>(const DateTimeDiff& other) const noexcept{
        return !(*this<=other);
    }
    bool operator<=(const DateTimeDiff& other) const noexcept{
        if(years_>other.years_)
            return false;
        if(months_>other.months_)
            return false;
        if(days_>other.days_)
            return false;
        if(hours_>other.hours_)
            return false;
        if(minutes_>other.minutes_)
            return false;
        if(seconds_>other.seconds_)
            return false;
        return *this==other;
    }
    bool operator>=(const DateTimeDiff& other) const noexcept{
        if(years_>other.years_)
            return false;
        if(months_>other.months_)
            return false;
        if(days_>other.days_)
            return false;
        if(hours_>other.hours_)
            return false;
        if(minutes_>other.minutes_)
            return false;
        if(seconds_>other.seconds_)
            return false;
        return *this==other;
    }

    DateTimeDiff& operator=(const DateTimeDiff& other) noexcept{
        if(this!=&other){
            years_ = other.years_;
            months_ = other.months_;
            days_ = other.days_;
            hours_ = other.hours_;
            minutes_ = other.minutes_;
            seconds_ = other.seconds_;
        }
        return *this;
    }
};

template<IsTimePoint ARG1_TP>
std::decay_t<ARG1_TP> operator+(ARG1_TP&& tp, const DateTimeDiff& diff){
    return diff + std::forward<ARG1_TP>(tp);
}

template<IsTimePoint ARG1_TP>
std::decay_t<ARG1_TP> operator-(ARG1_TP&& tp, const DateTimeDiff& diff){
    std::chrono::year_month_day ymd_tp(std::chrono::floor<std::chrono::days>(tp));
    return std::chrono::time_point_cast<typename ARG1_TP::duration>(std::chrono::sys_days(std::chrono::year_month_day(   ymd_tp.year()-std::chrono::years(diff.years_),
                                                                ymd_tp.month()-std::chrono::months(diff.months_),
                                                                ymd_tp.day()-std::chrono::days(diff.days_)))-std::chrono::seconds(diff.seconds_));
}

template<>
struct std::hash<DateTimeDiff>{
    size_t operator()(const DateTimeDiff& val){
        return (std::hash<decltype(val.years_)>()(val.years_)<<(sizeof(size_t)-sizeof(val.years_))*8)|
            (std::hash<decltype(val.months_)>()(val.months_)<<(sizeof(size_t)-sizeof(val.years_)-sizeof(val.months_))*8)|
            (std::hash<decltype(val.days_)>()(val.days_)<<(sizeof(size_t)-sizeof(val.years_)-
            sizeof(val.months_)-sizeof(val.days_))*8)|
            (std::hash<decltype(val.hours_)>()(val.hours_)<<(sizeof(size_t)-sizeof(val.years_)-
            sizeof(val.months_)-sizeof(val.days_)-sizeof(val.hours_))*8)|
            (std::hash<decltype(val.minutes_)>()(val.minutes_)<<(sizeof(size_t)-sizeof(val.years_)-
            sizeof(val.months_)-sizeof(val.days_)-sizeof(val.hours_)-sizeof(val.minutes_))*8)|
            (std::hash<decltype(val.minutes_)>()(val.minutes_)<<(sizeof(size_t)-sizeof(val.years_)-
            sizeof(val.months_)-sizeof(val.days_)-sizeof(val.hours_)-sizeof(val.minutes_)-sizeof(val.seconds_))*8);
    }
};

class TimeSequence{
    __time_interval__<std::chrono::seconds> interval_;
    DateTimeDiff time_duration_;
    uint32_t intervals_ = 0;
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
    template<IsTimePoint ARG_TP>
    TimeSequence(ARG_TP init_time):interval_(time_point_cast<std::chrono::seconds>(init_time),
                                            time_point_cast<std::chrono::seconds>(init_time)),
                                                time_duration_(DateTimeDiff()){}
    template<IsTimePoint ARG1_TP,IsTimePoint ARG2_TP>
    explicit TimeSequence(ARG1_TP&& from, ARG2_TP&& to,const DateTimeDiff& diff,std::error_code& err):
        TimeSequence(from,to,err,std::chrono::years(diff.years_),std::chrono::months(diff.months_),std::chrono::days(diff.days_),
                                std::chrono::hours(diff.hours_),std::chrono::minutes(diff.minutes_),std::chrono::seconds(diff.seconds_)){}
    template<IsTimePoint ARG1_TP,IsTimePoint ARG2_TP>
    explicit TimeSequence(ARG1_TP&& from, ARG2_TP&& to, std::error_code& err,
                    std::chrono::years years,
                    std::chrono::months months = std::chrono::months(),
                    std::chrono::days days = std::chrono::days(),
                    std::chrono::hours hours = std::chrono::hours(),
                    std::chrono::minutes minutes = std::chrono::minutes(),
                    std::chrono::seconds seconds = std::chrono::seconds()):
                    TimeSequence(
                        [&from,&err](){
                            err = std::error_code();
                            return time_point_cast<std::chrono::seconds>(from);}(),
                        time_point_cast<std::chrono::seconds>(to),(from==to?0:
        TimeSequence::compute_number_of_intervals(
                        time_point_cast<std::chrono::seconds>(from<to?from:to),  
                        time_point_cast<std::chrono::seconds>(to>from?to:from),
                        DateTimeDiff(err,years,months,days,hours,minutes,seconds),err)),err){}

    template<IsTimePoint ARG1_TP,IsTimePoint ARG2_TP>
    explicit TimeSequence(ARG1_TP&& from, ARG2_TP&& to, std::error_code& err,
                    std::chrono::months months,
                    std::chrono::days days = std::chrono::days(0),
                    std::chrono::hours hours = std::chrono::hours(0),
                    std::chrono::minutes minutes = std::chrono::minutes(0),
                    std::chrono::seconds seconds = std::chrono::seconds(0)):
                    TimeSequence(from,to,err,
                                            std::chrono::years(0),
                                            months,
                                            days,
                                            hours,
                                            minutes,
                                            seconds){}

    template<IsTimePoint ARG1_TP,IsTimePoint ARG2_TP>
    explicit TimeSequence(ARG1_TP&& from, ARG2_TP&& to, std::error_code& err,
                    std::chrono::days days,
                    std::chrono::hours hours = std::chrono::hours(0),
                    std::chrono::minutes minutes = std::chrono::minutes(0),
                    std::chrono::seconds seconds = std::chrono::seconds(0)):
                    TimeSequence(from,to,err,
                                            std::chrono::years(0),
                                            std::chrono::months(0),
                                            days,
                                            hours,
                                            minutes,
                                            seconds){}

    template<IsTimePoint ARG1_TP,IsTimePoint ARG2_TP>
    explicit TimeSequence(ARG1_TP&& from, ARG2_TP&& to, std::error_code& err,
                    std::chrono::hours hours,
                    std::chrono::minutes minutes = std::chrono::minutes(0),
                    std::chrono::seconds seconds = std::chrono::seconds(0)):
                    TimeSequence(from,to,err,
                                            std::chrono::years(0),
                                            std::chrono::months(0),
                                            std::chrono::days(0),
                                            hours,
                                            minutes,
                                            seconds){}
    
    template<IsTimePoint ARG1_TP,IsTimePoint ARG2_TP>
    explicit TimeSequence(ARG1_TP&& from, ARG2_TP&& to, std::error_code& err,
                    std::chrono::minutes minutes,
                    std::chrono::seconds seconds = std::chrono::seconds(0)):
                    TimeSequence(from,to,err,
                                            std::chrono::years(0),
                                            std::chrono::months(0),
                                            std::chrono::days(0),
                                            std::chrono::hours(0),
                                            minutes,
                                            seconds){}
    
    template<IsTimePoint ARG1_TP,IsTimePoint ARG2_TP>
    explicit TimeSequence(ARG1_TP&& from, ARG2_TP&& to, std::error_code& err,
                    std::chrono::seconds seconds):
                    TimeSequence(from,to,err,
                                            std::chrono::years(0),
                                            std::chrono::months(0),
                                            std::chrono::days(0),
                                            std::chrono::hours(0),
                                            std::chrono::minutes(0),
                                            seconds){}
    template<IsTimePoint ARG1_TP,IsTimePoint ARG2_TP>
    TimeSequence(ARG1_TP&& from, ARG2_TP&& to, uint32_t number_of_intervals,std::error_code& err):  interval_(time_point_cast<std::chrono::seconds>(from),time_point_cast<std::chrono::seconds>(to)),
                                                                    time_duration_(DateTimeDiff(time_point_cast<std::chrono::seconds>(from),
                                                                    time_point_cast<std::chrono::seconds>(to),number_of_intervals,err)),
                                                                    intervals_(number_of_intervals)
    {
        if((number_of_intervals==0 && from!=to) || (number_of_intervals>0 && from==to)){
            err = std::make_error_code(std::errc::invalid_argument);
            return;
        }
        // std::cout<<"Resulted duration from constructor:\nyears:"<<static_cast<int>(time_duration_.years_)<<
        //             "\nmonths:"<<static_cast<int>(time_duration_.months_)<<"\ndays:"<<static_cast<int>(time_duration_.days_)<<
        //             "\nhours:"<<static_cast<int>(time_duration_.hours_)<<"\nminutes"<<static_cast<int>(time_duration_.minutes_)<<
        //             "\nseconds"<<static_cast<int>(time_duration_.seconds_)<<std::endl;
    }
    template<IsDuration INTERVAL_DUR>
    TimeSequence(__time_interval__<INTERVAL_DUR> interval):interval_(interval),time_duration_(interval_.from(),interval_.to()),intervals_(1){}
    template<IsTimePoint ARG_TP>
    TimeSequence(ARG_TP&& from, const DateTimeDiff& dtd, uint16_t number_of_intervals){
        std::chrono::year_month_day ymd_to((std::chrono::floor<std::chrono::days>(from)));
        ymd_to=ymd_to+(std::chrono::years(dtd.years_)*number_of_intervals);
        ymd_to=ymd_to+(std::chrono::months(dtd.months_)*number_of_intervals);
        interval_ = decltype(interval_)(from,std::chrono::sys_days(ymd_to)+(std::chrono::days(dtd.days_)+
                                                                            std::chrono::hours(dtd.hours_)+
                                                                            std::chrono::minutes(dtd.minutes_)+
                                                                            std::chrono::seconds(dtd.seconds_))*
                                                                            number_of_intervals);
        intervals_ = number_of_intervals;
        time_duration_ = dtd;
    }
    TimeSequence(const TimeSequence& other){
        operator=(other);
    }
    TimeSequence(TimeSequence&& other){
        operator=(std::move(other));
    }
    TimeSequence& operator=(const TimeSequence& other){
        if(this!=&other){
            time_duration_=other.time_duration();
            interval_ = other.get_interval();
            intervals_ = other.intervals_;
        }
        return *this;
    }
    TimeSequence& operator=(TimeSequence&& other) noexcept{
        if(this!=&other){
            time_duration_=other.time_duration();
            interval_ = other.interval_;
            intervals_ = other.intervals_;
        }
        return *this;
    }

    template<std::ranges::range CONTAINER_TP>
    static std::pair<TimeSequence,typename std::decay_t<CONTAINER_TP>::const_iterator> make_from_range(CONTAINER_TP&& time_series,std::error_code& err) 
        requires (IsTimePoint<typename std::decay_t<CONTAINER_TP>::value_type>)
    {
        using range_type = std::decay_t<decltype(time_series)>;
        using tp_type = range_type::value_type;
        if(time_series.size()>0){
            utc_tp_t<std::chrono::seconds> first = std::chrono::floor<std::chrono::seconds>(*time_series.begin());
            TimeSequence result(first);
            if(time_series.size()>1){
                typename std::decay_t<decltype(time_series)>::const_iterator iter = std::next(time_series.begin());
                //std::cout<<"trying to push "<< std::chrono::time_point_cast<std::chrono::seconds>(*iter)<<std::endl;
                while(iter!=time_series.end() && result.push_time_after(*iter,err)){
                    ++iter;
                }
                return std::make_pair(result,iter);
            }
            else return std::make_pair(result,time_series.end());
        }
        else {
            err= std::make_error_code(std::errc::invalid_argument);
            return std::make_pair(TimeSequence(),time_series.end());
        }
    }
    const __time_interval__<std::chrono::seconds>& get_interval() const noexcept{
        return interval_;
    }
    DateTimeDiff time_duration() const noexcept{
        return time_duration_;
    }

    template<IsDuration DUR_PRECISION>
    void reset(utc_tp_t<DUR_PRECISION> time) noexcept{
        time_duration_ = DateTimeDiff();
        interval_ = __time_interval__(time,time);
    }
    template<IsDuration DUR_PRECISION>
    bool push_time(utc_tp_t<DUR_PRECISION> time, std::error_code& err) noexcept{
        if((push_time_before(time,err) && err==std::error_code()) || (err == std::error_code() && push_time_after(time,err)))
            return err==std::error_code();
        else
            return false;
    }
    template<IsDuration DUR_PRECISION>
    bool push_time_before(utc_tp_t<DUR_PRECISION> time,std::error_code& err) noexcept{
        if(time<interval_.from()){
            if(time_duration_==DateTimeDiff()){
                TimeSequence tmp(time,interval_.to(),1,err);
                if(err!=std::error_code())
                    return false;
                operator=(std::move(tmp));
                ++intervals_;
                return true;
            }
            else{
                if(DateTimeDiff(time,interval_.from())==time_duration_){
                    interval_=__time_interval__(time,interval_.to());
                    ++intervals_;
                    return true;
                }
                else return false;
            }
            ++intervals_;
            return true;
        }
        else return false;
    }
    template<IsDuration DUR_PRECISION>
    bool push_time_after(utc_tp_t<DUR_PRECISION> time, std::error_code& err) noexcept{
        // std::cout<<"comparing times \"interval_.to()\" "<< std::chrono::time_point_cast<std::chrono::seconds>(interval_.to())<<"\n\"time\" "<<
        //         std::chrono::time_point_cast<std::chrono::seconds>(time)<<std::endl;
        if(time>interval_.to()){
            if(time_duration_==DateTimeDiff()){
                TimeSequence tmp(interval_.from(),time,1,err);
                if(err!=std::error_code())
                    return false;
                operator=(std::move(tmp));
                ++intervals_;
                return true;
            }
            else{
                // std::cout<<"comparing interval from "<< std::chrono::time_point_cast<std::chrono::seconds>(interval_.to())<<"\nto "<<
                // std::chrono::time_point_cast<std::chrono::seconds>(time)<<std::endl;
                if(DateTimeDiff(interval_.to(),time)==time_duration_){
                    interval_=__time_interval__(interval_.from(),time);
                    // std::cout<<"updated interval from "<< std::chrono::time_point_cast<std::chrono::seconds>(interval_.from())<<"\nto "<<
                    // std::chrono::time_point_cast<std::chrono::seconds>(interval_.to())<<std::endl;
                    ++intervals_;
                    return true;
                }
                else return false;
            }
            ++intervals_;
            return true;
        }
        else return false;
    }
    uint32_t number_of_intervals(std::error_code& err) const noexcept{
        return intervals_;
    }
    template<IsDuration FROM_DUR,IsDuration TO_DUR>
    static int32_t compute_number_of_intervals(utc_tp_t<FROM_DUR> from,utc_tp_t<TO_DUR> to, const DateTimeDiff& diff,std::error_code& err) noexcept{
        err = std::error_code();
        if(from==to)
            return 0;
        else{
            auto is_integer = [](const auto& number){
                bool is = std::fmod(std::fabs(number),1)<std::numeric_limits<std::decay_t<decltype(number)>>::epsilon();
                return is;
            };
            
            int32_t N = 0;
            std::chrono::year_month_day ymd_from(to>from?std::chrono::floor<std::chrono::days>(from):std::chrono::floor<std::chrono::days>(to)),
                                        ymd_to(to>from?std::chrono::floor<std::chrono::days>(to):std::chrono::floor<std::chrono::days>(from));
            if(diff.years_!=0){
                double years = static_cast<double>((ymd_to.year()-ymd_from.year()).count())/diff.years_;
                if(diff.months_==0 && diff.days_==0 && diff.hours_==0 && diff.minutes_==0 && diff.seconds_==0 && !is_integer(years)){
                    err = std::make_error_code(std::errc::invalid_argument);
                    return -1;
                }
                N=years;
            }

            if(diff.months_!=0){
                double months = static_cast<double>((ymd_to.month()-ymd_from.month() -(ymd_to.month()<ymd_from.month()?std::chrono::months(1):std::chrono::months(0))+ 
                        std::chrono::months(((ymd_to.year()-ymd_from.year()).count()-diff.years_-(ymd_to.month()<ymd_from.month()?1:0))*12)).count())/diff.months_;
                if(diff.days_==0 && diff.hours_==0 && diff.minutes_==0 && diff.seconds_==0 && !is_integer(months)){
                    err = std::make_error_code(std::errc::invalid_argument);
                    return -1;
                }
                if(months==0){
                    if(N!=0){
                        err = std::make_error_code(std::errc::invalid_argument);
                        return -1;
                    }
                }
                else{
                    if(N!=0){
                        if(N!=static_cast<int32_t>(months)){
                            err = std::make_error_code(std::errc::invalid_argument);
                            return -1;
                        }
                    }
                    else N=months;
                }
            }
            if(diff.days_!=0 || diff.hours_!=0 || diff.minutes_!=0 || diff.seconds_!=0){
                auto diff_days_time = (to>from?((to-std::chrono::floor<std::chrono::days>(to))-(from-std::chrono::floor<std::chrono::days>(from))):
                                ((from-std::chrono::floor<std::chrono::days>(from))-(to-std::chrono::floor<std::chrono::days>(to))))+
                                std::chrono::sys_days(ymd_to-std::chrono::months(N*diff.months_)-std::chrono::years(N*diff.years_))-
                                std::chrono::sys_days(ymd_from);
                if(double days_time = static_cast<double>(std::chrono::duration_cast<std::chrono::seconds>(diff_days_time).count())/
                    (std::chrono::days(diff.days_)+std::chrono::hours(diff.hours_)+std::chrono::minutes(diff.minutes_)+std::chrono::seconds(diff.seconds_)).count();!is_integer(days_time))
                {
                    err = std::make_error_code(std::errc::invalid_argument);
                    return -1;
                }
                else{
                    if(days_time==0){
                        if(N!=0){
                            err = std::make_error_code(std::errc::invalid_argument);
                            return -1;
                        }
                    }
                    else{
                        if(N!=0){
                            if(static_cast<uint32_t>(N)!=static_cast<uint32_t>(days_time)){
                                err = std::make_error_code(std::errc::invalid_argument);
                                return -1;
                            }
                        }
                        else N=days_time;
                    } 
                }
            } 
            return N;
        }
    }
    template<IsDuration OTHER_DUR>
    bool extend_by_interval(const __time_interval__<OTHER_DUR>& interval,std::error_code& err) noexcept{
        TimeSequence tmp(interval.from(),interval.to(),time_duration_,err);
        if(err != std::error_code() && !extendable_by(tmp))
            return false;
        interval_ = __time_interval__(interval.from()<interval_.from()?interval.from():interval_.from(),interval.to()>interval_.to()?interval.to():interval_.to());
        intervals_ = compute_number_of_intervals(interval_.from(),interval_.to(),time_duration_,err);
        return true;
    }
    bool extend_by_sequence(const TimeSequence& sequence,std::error_code& err) noexcept{
        if(!extendable_by(sequence))
            return false;
        auto interval = sequence.interval_;
        interval_ = __time_interval__(sequence.interval_.from()<interval_.from()?
                                sequence.interval_.from():
                                interval_.from(),
                                sequence.interval_.to()>interval_.to()?
                                sequence.interval_.to():
                                interval_.to());
        intervals_ = compute_number_of_intervals(interval_.from(),interval_.to(),time_duration_,err);
        return true;
    }
    bool extendable_by(const TimeSequence& other) const noexcept{
        if((other.time_duration()==DateTimeDiff() && time_duration_==DateTimeDiff()))
            return true;
        else if(time_duration_ == other.time_duration_){
            std::error_code err = std::error_code();
            if(other.interval_.to()!=interval_.to() || other.interval_.from()!=interval_.from()){
                std::error_code err;
                compute_number_of_intervals(other.interval_.from(),interval_.from(),time_duration_,err);
                if((err==std::error_code() &&
                (((other.interval_.from()<=interval_.to() + time_duration_ && 
                other.interval_.to()>=interval_.to() + time_duration_) || 
                (other.interval_.to()>=interval_.from() - time_duration_ && 
                other.interval_.from()<=interval_.from() - time_duration_)))))
                    return true;
                else if(DateTimeDiff interval_internal_extension(other.interval_.from(),interval_.from(),2,err);
                    err==std::error_code() &&
                    (interval_.from()+interval_internal_extension+interval_internal_extension == interval_.from()+time_duration_ || 
                    interval_.from()-interval_internal_extension-interval_internal_extension == interval_.from()-time_duration_) &&
                    (interval_.to()+interval_internal_extension+interval_internal_extension == interval_.to()+time_duration_ ||
                    interval_.to()-interval_internal_extension-interval_internal_extension == interval_.to()-time_duration_))
                    return true;
                else return false;
            }
            else return false;
        }
        else return false;
    }
    bool operator==(const TimeSequence& other) const;
    bool operator<(const TimeSequence& other) const;
};

template<IsDuration DUR_PRECISION>
struct std::hash<__time_interval__<DUR_PRECISION>>{
    size_t operator()(const __time_interval__<DUR_PRECISION>& val){
        return std::hash<size_t>{}(static_cast<size_t>(val.from().time_since_epoch().count())^(static_cast<size_t>(val.from().time_since_epoch().count()<<1)));
    }
};

template<IsDuration DUR_PRECISION>
struct std::equal_to<__time_interval__<DUR_PRECISION>>{
    template<IsDuration ARG1_DUR,IsDuration ARG2_DUR>
    bool operator()(const __time_interval__<ARG1_DUR>& lhs,const __time_interval__<ARG2_DUR>& rhs) const{
        return lhs==rhs;
    }
};

template<IsDuration DUR_PRECISION>
struct std::less<__time_interval__<DUR_PRECISION>>{
    template<IsDuration ARG1_DUR,IsDuration ARG2_DUR>
    bool operator()(const __time_interval__<ARG1_DUR>& lhs,const __time_interval__<ARG2_DUR>& rhs) const{
        return (lhs.from()<rhs.from())?true:(lhs.from()==rhs.from()?lhs.to()-lhs.from()<rhs.to()-rhs.from():false);
    }
};

template<>
struct std::hash<TimeSequence>{
    size_t operator()(const TimeSequence& val){
        return std::hash<__time_interval__<std::chrono::seconds>>{}(val.interval_)^(std::hash<DateTimeDiff>()(val.time_duration_)<<1);
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

template<IsDuration ARG1_DUR,IsDuration ARG2_DUR>
bool is_correct_interval(const utc_tp_t<ARG1_DUR>& from,const utc_tp_t<ARG2_DUR>& to){
    return (to-from).count()>=0;
}
template<IsDuration DUR_PRECISION>
bool is_correct_interval(const __time_interval__<DUR_PRECISION>& interval) noexcept{
    return is_correct_interval(interval.from(),interval.to());
}
template<IsDuration INTERVAL1_DUR,IsDuration INTERVAL2_DUR>
bool intervals_intersect(const __time_interval__<INTERVAL1_DUR>& lhs, const __time_interval__<INTERVAL2_DUR>& rhs){
    return !(lhs.from()>rhs.to()  || lhs.to()<rhs.from());
}
template<IsDuration FROM1_DUR,IsDuration FROM2_DUR,IsDuration TO1_DUR,IsDuration TO2_DUR>
bool intervals_intersect(const utc_tp_t<FROM1_DUR>& from_1, const utc_tp_t<TO1_DUR>& to_1,const utc_tp_t<FROM2_DUR>& from_2, const utc_tp_t<TO2_DUR>& to_2){
    return !(from_1>to_2  || to_1<from_2);
}

template<IsDuration INTERVAL1_DUR,IsDuration INTERVAL2_DUR,
    IsDuration RESULT_DUR = std::conditional_t<std::ratio_less_v<typename INTERVAL1_DUR::period,typename INTERVAL2_DUR::period>,INTERVAL1_DUR,INTERVAL2_DUR>>
std::optional<__time_interval__<RESULT_DUR>> interval_intersection(const __time_interval__<INTERVAL1_DUR>& lhs,const __time_interval__<INTERVAL2_DUR>& rhs) noexcept{
    if(intervals_intersect(lhs,rhs)){
        if(lhs.from()<rhs.from())
            return __time_interval__(rhs.from(),lhs.to()<rhs.to()?lhs.to():rhs.to());
        else
            return __time_interval__(lhs.from(),lhs.to()<rhs.to()?lhs.to():rhs.to());
    }
    else return std::nullopt;
}

template<IsDuration INTERVAL_DUR>
std::optional<std::pair<uint64_t,uint64_t>> interval_intersection_pos(const __time_interval__<INTERVAL_DUR>& to_seek, const TimeSequence& initial, std::error_code& err) noexcept{
    std::pair<int64_t,int64_t> result{0,0};
    if(!intervals_intersect(to_seek,initial.get_interval()))
        return std::nullopt;
    if(initial.time_duration()==DateTimeDiff())
        return std::nullopt;
    result.first = initial.compute_number_of_intervals(initial.get_interval().from(),to_seek.from(),initial.time_duration(),err);
    if(err!=std::error_code())
        return std::nullopt;
    result.second = initial.compute_number_of_intervals(to_seek.to(),initial.get_interval().to(),initial.time_duration(),err);
    if(err!=std::error_code())
        return std::nullopt;
    return result;
}

namespace serialization{
    template<bool NETWORK_ORDER,IsDuration DUR_PRECISION>
    struct Serialize<NETWORK_ORDER,__time_interval__<DUR_PRECISION>>{
        auto operator()(const __time_interval__<DUR_PRECISION>& val,std::vector<char>& buf) const noexcept{
            return serialize<NETWORK_ORDER>(val,buf,val.from(),val.to());
        }
    };

    template<bool NETWORK_ORDER,IsDuration DUR_PRECISION>
    struct Deserialize<NETWORK_ORDER,__time_interval__<DUR_PRECISION>>{
        auto operator()(__time_interval__<DUR_PRECISION>& val,std::span<const char> buf) const noexcept{
            return deserialize<NETWORK_ORDER>(val,buf,val.from_,val.to_);
        }
    };

    template<IsDuration DUR_PRECISION>
    struct Serial_size<__time_interval__<DUR_PRECISION>>{
        size_t operator()(const __time_interval__<DUR_PRECISION>& val) const noexcept{
            return serial_size(val.from(),val.to());
        }
    };

    template<IsDuration DUR_PRECISION>
    struct Min_serial_size<__time_interval__<DUR_PRECISION>>{
        using type = __time_interval__<DUR_PRECISION>;
        static constexpr size_t value = []()
        {
            return min_serial_size<decltype(type::from_),decltype(type::to_)>();
        }();
    };
     
    template<IsDuration DUR_PRECISION>
    struct Max_serial_size<__time_interval__<DUR_PRECISION>>{
        using type = __time_interval__<DUR_PRECISION>;
        static constexpr size_t value = []()
        {
            return max_serial_size<decltype(type::from_),decltype(type::to_)>();
        }();
    };

    template<bool NETWORK_ORDER>
    struct Serialize<NETWORK_ORDER,DateTimeDiff>{
        auto operator()(const DateTimeDiff& val,std::vector<char>& buf) const noexcept{
            return serialize<NETWORK_ORDER>(val,buf,val.years_,val.months_,val.days_,val.hours_,val.minutes_,val.seconds_);
        }
    };

    template<bool NETWORK_ORDER>
    struct Deserialize<NETWORK_ORDER,DateTimeDiff>{
        auto operator()(DateTimeDiff& val,std::span<const char> buf) const noexcept{
            return deserialize<NETWORK_ORDER>(val,buf,val.years_,val.months_,val.days_,val.hours_,val.minutes_,val.seconds_);
        }
    };

    template<>
    struct Serial_size<DateTimeDiff>{
        size_t operator()(const DateTimeDiff& val) const noexcept{
            return serial_size(val.years_,val.months_,val.days_,val.hours_,val.minutes_,val.seconds_);
        }
    };

    template<>
    struct Min_serial_size<DateTimeDiff>{
        using type = DateTimeDiff;
        static constexpr size_t value = []()
        {
            return min_serial_size<decltype(type::years_),decltype(type::months_),decltype(type::days_)
                                    ,decltype(type::hours_),decltype(type::minutes_),decltype(type::seconds_)>();
        }();
    };
     
    template<>
    struct Max_serial_size<DateTimeDiff>{
        using type = DateTimeDiff;
        static constexpr size_t value = []()
        {
            return max_serial_size<decltype(type::years_),decltype(type::months_),decltype(type::days_)
                                    ,decltype(type::hours_),decltype(type::minutes_),decltype(type::seconds_)>();
        }();
    };

    template<bool NETWORK_ORDER>
    struct Serialize<NETWORK_ORDER,TimeSequence>{
        auto operator()(const TimeSequence& val,std::vector<char>& buf) const noexcept{
            return serialize<NETWORK_ORDER>(val,buf,val.interval_,val.time_duration_);
        }
    };

    template<bool NETWORK_ORDER>
    struct Deserialize<NETWORK_ORDER,TimeSequence>{
        auto operator()(TimeSequence& val,std::span<const char> buf) const noexcept{
            return deserialize<NETWORK_ORDER>(val,buf,val.interval_,val.time_duration_);
        }
    };

    template<>
    struct Serial_size<TimeSequence>{
        size_t operator()(const TimeSequence& val) const noexcept{
            return serial_size(val.interval_,val.time_duration_);
        }
    };

    template<>
    struct Min_serial_size<TimeSequence>{
        using type = TimeSequence;
        static constexpr size_t value = []()
        {
            return min_serial_size<decltype(type::interval_),decltype(type::time_duration_)>();
        }();
    };
     
    template<>
    struct Max_serial_size<TimeSequence>{
        using type = TimeSequence;
        static constexpr size_t value = []()
        {
            return max_serial_size<decltype(type::interval_),decltype(type::time_duration_)>();
        }();
    };
}

#include "boost/json.hpp"
#include "boost/lexical_cast.hpp"
#include <expected>
#include "boost_functional/json.h"

#include "concepts.h"

using default_template_arg_time = std::chrono::seconds;

template<IsDuration DUR,IsDuration ARG_DURATION>
boost::json::value to_json(const ARG_DURATION& tp) {
    boost::json::value result;
    if constexpr(std::is_same_v<DUR,days>)
        result = std::format("{}d", std::chrono::duration_cast<DUR>(tp).count());
    else if constexpr(std::is_same_v<DUR,hours>)
        result = std::format("{}h", std::chrono::duration_cast<DUR>(tp).count());
    else if constexpr(std::is_same_v<DUR,minutes>)
        result = std::format("{}m", std::chrono::duration_cast<DUR>(tp).count());
    else result = std::format("{}s", std::chrono::duration_cast<DUR>(tp).count());
    return result;
}

template<>
boost::json::value to_json(const DateTimeDiff& diff);

template<IsDuration DUR,IsTimePoint ARG_TP>
boost::json::value to_json(const ARG_TP& tp) {
    boost::json::value result;
    result = std::format("{:%Y/%m/%d %H:%M:%S %Z}", time_point_cast<DUR>(tp));
    return result;
}

template<IsTimePoint TIME_T>
std::expected<TIME_T,std::exception> from_json(const boost::json::value& json_time){
    using namespace std::string_literals;
    if(!json_time.is_string())
        return std::unexpected(std::exception());
    std::istringstream stream_tmp(json_time.as_string().subview());
    TIME_T result;
    stream_tmp>>std::chrono::parse("%Y/%m/%d %H:%M:%S %Z",result);
    if(stream_tmp.fail())
        return std::unexpected(std::exception());
    return result;
}

template<>
std::expected<DateTimeDiff,std::exception> from_json<DateTimeDiff>(const boost::json::value& json_time);

template<IsDuration ARG_DURATION>
boost::json::value to_json(const ARG_DURATION& tp) {
    return to_json<ARG_DURATION,ARG_DURATION>(tp);
}

template<IsTimePoint ARG_TP>
boost::json::value to_json(const ARG_TP& tp) {
    return to_json<typename ARG_TP::duration,ARG_TP>(tp);
}

namespace boost {
template<typename TP>
TP lexical_cast( std::string const& s ) requires(IsTimePoint<TP>)
{

    std::istringstream is(s);
    is.imbue(std::locale("en_US.utf-8"));

    TP val;
    std::chrono::from_stream(is,"%y/%m/%d %H:%M:%S", val);
    if (is.fail())
        throw boost::bad_lexical_cast();

    return val;
}

// из time_point  →  std::string
template<typename TP>
std::string lexical_cast(const TP& tp )requires(IsTimePoint<TP>)
{
    return std::format("{:%y/%m/%d %H:%M:%S}", tp);
}

template<>
DateTimeDiff lexical_cast(const std::string& input);
template<>
std::string lexical_cast(const DateTimeDiff& input);
}

static_assert(IsTimePoint<utc_tp>);
static_assert(IsDuration<utc_diff>);
static_assert(!IsTimePoint<utc_diff>);
static_assert(!IsDuration<utc_tp>);