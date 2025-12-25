#include "time_interval.h"

bool TimeInterval::operator==(const TimeInterval& other) const{
    return from_==other.from_ && to_==other.to_;
}
bool TimeInterval::operator<(const TimeInterval& other) const{
    return (from_<other.from_)?true:(from_==other.from_?to_-from_<other.to_-other.from_:false);
}

bool TimeSequence::operator==(const TimeSequence& other) const{
    return std::equal_to<TimeSequence>()(*this,other);
}
bool TimeSequence::operator<(const TimeSequence& other) const{
    return std::less<TimeSequence>()(*this,other);
}

template<>
utc_tp boost::lexical_cast(const std::string& input){
    std::istringstream s(input);
    s.imbue(std::locale("en_US.utf-8"));
    utc_tp tp;
    s>>std::chrono::parse("{:%D-%T}",tp);
    if(s.fail())
        throw boost::bad_lexical_cast();
    return tp;
}

template<>
std::string boost::lexical_cast(const utc_tp& input){
    return std::format("{:%D-%T}",input);
}

bool is_correct_interval(const std::chrono::system_clock::time_point& from,const std::chrono::system_clock::time_point& to){
    return (to-from).count()>=0;
}
bool is_correct_interval(const TimeInterval& interval) noexcept{
    return is_correct_interval(interval.from(),interval.to());
}
bool intervals_intersect(const TimeInterval& lhs, const TimeInterval& rhs){
    return !(lhs.from()>rhs.to()  || lhs.to()<rhs.from());
}
bool intervals_intersect(const utc_tp& from_1, const utc_tp& to_1,const utc_tp& from_2, const utc_tp& to_2){
    return !(from_1>to_2  || to_1<from_2);
}
#include <iostream>
//А если не пересекаются?
//@brief При отсутствии пересечения интервалов возвращает 0,0
std::optional<std::pair<uint64_t,uint64_t>> interval_intersection_pos(const TimeInterval& to_seek, const TimeSequence& initial) noexcept{
    std::pair<int64_t,int64_t> result{0,0};
    if(!intervals_intersect(to_seek,initial.get_interval()))
        return std::nullopt;
    if(initial.discret().count()==0)
        return result;
    if(to_seek.from()<=initial.get_interval().from())
        result.first = 0;
    else
        result.first = (to_seek.from()-initial.get_interval().from())/initial.discret();

    if(to_seek.to()>=initial.get_interval().to())
        result.second = (initial.get_interval().to()-initial.get_interval().from())/initial.discret();
    else{
        result.second = (to_seek.to()-initial.get_interval().from())/initial.discret();
    }
    //result.first = to_seek.from_<=initial.from_?0:(to_seek.from_-initial.from_)/discret;
    return result;
}

std::optional<TimeInterval> interval_intersection(const TimeInterval& lhs,const TimeInterval& rhs) noexcept{
    if(intervals_intersect(lhs,rhs)){
        if(lhs.from()<rhs.from())
            return TimeInterval(rhs.from(),lhs.to()<rhs.to()?lhs.to():rhs.to());
        else
            return TimeInterval(lhs.from(),lhs.to()<rhs.to()?lhs.to():rhs.to());
    }
    else return std::nullopt;
}

template<>
boost::json::value to_json(const utc_tp& time){
    boost::json::string result;
    result.subview() = std::format("{}",time);
    return result;
}

template<>
std::expected<utc_tp,std::exception> from_json<utc_tp>(const boost::json::value& json_time){
    if(json_time.is_string()){
        std::istringstream stream_tmp(json_time.as_string().subview());
        utc_tp result;
        stream_tmp>>std::chrono::parse("{}",result);
        if(stream_tmp.fail())
            return std::unexpected(std::exception());
        else return result;
    }
    else return std::unexpected(std::exception());
}

template<>
boost::json::value to_json(const utc_tp_t<std::chrono::seconds>& time){
    boost::json::string result;
    result.subview() = std::format("{:%Y/%m/%D %H:%M:%S}",time);
    return result;
}

template<>
std::expected<utc_tp_t<std::chrono::seconds>,std::exception> from_json<utc_tp_t<std::chrono::seconds>>(const boost::json::value& json_time){
    if(json_time.is_string()){
        std::istringstream stream_tmp(json_time.as_string().subview());
        utc_tp_t<std::chrono::seconds> result;
        stream_tmp>>std::chrono::parse("{:%Y/%m/%D %H:%M:%S}",result);
        if(stream_tmp.fail())
            return std::unexpected(std::exception());
        else return result;
    }
    else return std::unexpected(std::exception());
}