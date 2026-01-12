#include "time_interval.h"

bool TimeSequence::operator==(const TimeSequence& other) const{
    return std::equal_to<TimeSequence>()(*this,other);
}
bool TimeSequence::operator<(const TimeSequence& other) const{
    return std::less<TimeSequence>()(*this,other);
}

template<>
boost::json::value to_json(const DateTimeDiff& diff) {
    boost::json::value result;
    result = std::format("%dY %dM %dD %dh %dm %ds",
                        diff.years_,diff.months_,
                        diff.days_,diff.hours_,
                        diff.minutes_,diff.seconds_);
    return result;
}

template<>
std::expected<DateTimeDiff,std::exception> from_json<DateTimeDiff>(const boost::json::value& json_time){
    using namespace std::string_literals;
    if(!json_time.is_string())
        return std::unexpected(std::exception());
    std::istringstream stream_tmp(json_time.as_string().subview());
    DateTimeDiff result;
    if(std::sscanf(json_time.as_string().c_str(),"%dY %dM %dD %dh %dm %ds",
    result.years_,result.months_,result.days_,result.hours_,result.minutes_,result.seconds_)!=0)
        return std::unexpected(std::exception());
    else return result;
}