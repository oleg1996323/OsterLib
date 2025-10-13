#include "time_interval.h"

bool TimeInterval::operator==(const TimeInterval& other) const{
    return std::equal_to<TimeInterval>()(*this,other);
}
bool TimeInterval::operator<(const TimeInterval& other) const{
    return std::less<TimeInterval>()(*this,other);
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
    return (to-from).count()>0;
}
bool intervals_intersect(const TimeInterval& lhs, const TimeInterval& rhs){
    return !(lhs.from_>rhs.to_  || lhs.to_<rhs.from_);
}
bool intervals_intersect(const utc_tp& from_1, const utc_tp& to_1,const utc_tp& from_2, const utc_tp& to_2){
    return !(from_1>to_2  || to_1<from_2);
}
#include <iostream>
std::pair<uint16_t,uint16_t> interval_intersection_pos(const TimeInterval& to_seek, const TimeInterval& initial, const utc_diff& discret) noexcept{
    std::pair<uint16_t,uint16_t> result{0,0};
    if(discret.count()==0)
        return result;
    if(to_seek.from_<=initial.from_)
        result.first = 0;
    else
        result.first = (to_seek.from_-initial.from_)/discret;

    if(to_seek.to_>=initial.to_)
        result.second = (initial.to_-initial.from_)/discret;
    else{
        result.second = (to_seek.to_-initial.from_)/discret;
    }
    //result.first = to_seek.from_<=initial.from_?0:(to_seek.from_-initial.from_)/discret;
    return result;
}

std::optional<TimeInterval> interval_intersection(const TimeInterval& lhs,const TimeInterval& rhs) noexcept{
    if(intervals_intersect(lhs,rhs)){
        if(lhs.from_<rhs.from_)
            return TimeInterval{.from_ = rhs.from_,.to_ = (lhs.to_<rhs.to_?lhs.to_:rhs.to_)};
        else
            return TimeInterval{.from_ = lhs.from_,.to_ = (lhs.to_<rhs.to_?lhs.to_:rhs.to_)};
    }
    else return std::nullopt;
}