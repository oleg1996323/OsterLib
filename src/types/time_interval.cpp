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

#include "parsing.h"

template<>
DateTimeDiff boost::lexical_cast(const std::string& input){
    using namespace std::string_literals;
    DateTimeDiff result;
    std::vector<std::string_view> tokens = split<std::string_view>(std::string_view(input),":");
    if(!tokens.empty()){
        for(std::string_view token:tokens){
            auto tmp(from_chars<int>(token.substr(1)));
            if(!tmp.has_value())
                throw std::invalid_argument(input);
            else{
                if(tmp.value()<0){
                    throw std::invalid_argument("Invalid time offset token input "s+std::string(token));
                }
                else if(tmp.value()==0){
                    std::cout<<"Ignored value: "s<<token<<std::endl;
                    continue;
                }
            }
            if(token.size()>0){
                if(iend_with(token,std::string_view("h")) &&
                (tmp.value()<=std::numeric_limits<decltype(DateTimeDiff::hours_)>::max() &&
                tmp.value()>=std::numeric_limits<decltype(DateTimeDiff::hours_)>::min()))
                    result.hours_ = tmp.value();
                else if(iend_with(token,std::string_view("y")) &&
                (tmp.value()<=std::numeric_limits<decltype(DateTimeDiff::years_)>::max() &&
                tmp.value()>=std::numeric_limits<decltype(DateTimeDiff::years_)>::min()))
                    result.years_ = tmp.value();
                else if(iend_with(token,std::string_view("m")) &&
                (tmp.value()<=std::numeric_limits<decltype(DateTimeDiff::months_)>::max() &&
                tmp.value()>=std::numeric_limits<decltype(DateTimeDiff::months_)>::min()))
                    result.months_ = tmp.value();
                else if(iend_with(token,std::string_view("d")) &&
                (tmp.value()<=std::numeric_limits<decltype(DateTimeDiff::days_)>::max() &&
                tmp.value()>=std::numeric_limits<decltype(DateTimeDiff::days_)>::min()))
                    result.days_ = tmp.value();
                else if(iend_with(token,std::string_view("min")) &&
                (tmp.value()<=std::numeric_limits<decltype(DateTimeDiff::minutes_)>::max() &&
                tmp.value()>=std::numeric_limits<decltype(DateTimeDiff::minutes_)>::min()))
                    result.days_ = tmp.value();
                else if(iend_with(token,std::string_view("s")) &&
                (tmp.value()<=std::numeric_limits<decltype(DateTimeDiff::seconds_)>::max() &&
                tmp.value()>=std::numeric_limits<decltype(DateTimeDiff::seconds_)>::min()))
                    result.days_ = tmp.value();
                else{
                    std::cout<<"Unknown time offset token"<<std::endl;
                    throw std::invalid_argument(input);
                }
            }
            else{
                std::cout<<"Missed time offset token"<<std::endl;
                throw std::invalid_argument(input);
            }
        }
        return result;
    }
    else{
        std::cout<<"Empty string at time offset definition"<<std::endl;
        throw std::invalid_argument(input);
    }
}
template<>
std::string boost::lexical_cast(const DateTimeDiff& input){
    std::string result;
    if(input.years_>0)
        result+=std::to_string(input.years_)+"y";
    if(input.months_>0)
        result+=std::to_string(input.months_)+"m";
    if(input.days_>0)
        result+=std::to_string(input.days_)+"d";
    if(input.hours_>0)
        result+=std::to_string(input.hours_)+"h";
    if(input.minutes_>0)
        result+=std::to_string(input.hours_)+"min";
    if(input.seconds_>0)
        result+=std::to_string(input.hours_)+"s";
    if(result.empty())
        result+="0s";
    return result;
}