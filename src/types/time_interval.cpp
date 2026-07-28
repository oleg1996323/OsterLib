#include "OsterLib/types/time_interval.h"

bool TimeSequence::operator==(const TimeSequence& other) const{
    return std::equal_to<TimeSequence>()(*this,other);
}
bool TimeSequence::operator<(const TimeSequence& other) const{
    return std::less<TimeSequence>()(*this,other);
}

template<>
boost::json::value to_json(const DateTimeDiff& diff) {
    boost::json::value result;
    result = std::format("{}Y {}M {}D {}h {}m {}s",
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
    int years,months,days,hours,minutes,seconds;    
    if(std::sscanf(json_time.as_string().c_str(),"%dY %dM %dD %dh %dm %ds",
    &years,&months,&days,&hours,&minutes,&seconds)!=6)
        return std::unexpected(std::exception());
    else{
        std::error_code err;
        using namespace std;
        DateTimeDiff result(err,
            chrono::years(years),
            chrono::months(months),
            chrono::days(days),
            chrono::hours(hours),
            chrono::minutes(minutes),
            chrono::seconds(seconds));
        return result;
    }
}

#include "OsterLib/parsing.h"

int32_t find_char(std::string_view input){
    if(input.empty())
        return -1;
    int32_t i=0;
    for(i;i<input.size();++i){
        if(input[i]>47 && input[i]<58)
            continue;
        else break;
    }
    return i;
}

template<>
DateTimeDiff boost::lexical_cast(const std::string& input){
    using namespace std::string_literals;
    DateTimeDiff result;
    std::string_view current(input);
    for(;;)
    {
        int32_t pos=find_char(current);
        if(pos==-1 || current.size()<=pos)
            std::runtime_error("invalid DateTimeDiff input");
        int32_t tmp;
        auto fcerr = std::from_chars(
            current.data(),
            current.data()+pos,
            tmp);
        if(fcerr.ec!=std::errc() || tmp<0)
            std::runtime_error("invalid DateTimeDiff input");
        if(iend_with(current.substr(pos,1),std::string_view("h")) &&
        (tmp<=std::numeric_limits<decltype(DateTimeDiff::hours_)>::max() &&
        tmp>=std::numeric_limits<decltype(DateTimeDiff::hours_)>::min())){
            result.hours_ = tmp;
            current = current.substr(pos+1);
        }
        else if(iend_with(current.substr(pos,1),std::string_view("y")) &&
        (tmp<=std::numeric_limits<decltype(DateTimeDiff::years_)>::max() &&
        tmp>=std::numeric_limits<decltype(DateTimeDiff::years_)>::min())){
            result.years_ = tmp;
            current = current.substr(pos+1);
        }
        else if(iend_with(current.substr(pos,1),std::string_view("m")) &&
        (tmp<=std::numeric_limits<decltype(DateTimeDiff::months_)>::max() &&
        tmp>=std::numeric_limits<decltype(DateTimeDiff::months_)>::min())){
            result.months_ = tmp;
            current = current.substr(pos+1);
        }
        else if(iend_with(current.substr(pos,1),std::string_view("d")) &&
        (tmp<=std::numeric_limits<decltype(DateTimeDiff::days_)>::max()) &&
        (tmp>=std::numeric_limits<decltype(DateTimeDiff::days_)>::min())){
            result.days_ = tmp;
            current = current.substr(pos+1);
        }
        else if(iend_with(current.substr(pos,3),std::string_view("min")) &&
        (tmp<=std::numeric_limits<decltype(DateTimeDiff::minutes_)>::max()) &&
        (tmp>=std::numeric_limits<decltype(DateTimeDiff::minutes_)>::min())){
            result.days_ = tmp;
            current = current.substr(pos+3);
        }
        else if(iend_with(current.substr(pos,1),std::string_view("s")) &&
        (tmp<=std::numeric_limits<decltype(DateTimeDiff::seconds_)>::max() &&
        tmp>=std::numeric_limits<decltype(DateTimeDiff::seconds_)>::min())){
            result.days_ = tmp;
            current = current.substr(pos+1);
        }
        else{
            using namespace std::string_literals;
            throw std::runtime_error("invalid DateTimeDiff input: "s+input);
        }
        if(current.empty())
            return result;
    }
    using namespace std::string_literals;
    throw std::runtime_error("invalid DateTimeDiff input: "s+input+". Empty string");
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

template<>
boost::json::value to_json(const TimeSequence& tp) {
    boost::json::object result;
    result["start"] = to_json(tp.get_interval().from());
    result["end"] = to_json(tp.get_interval().to());
    result["intervals"] = to_json(tp.number_of_intervals());
    return result;
}
template<>
std::expected<TimeSequence,std::exception> 
    from_json(const boost::json::value& ts)
{
    if(ts.is_object()){
        auto& obj = ts.as_object();
        if(!obj.contains("start"))
            return std::unexpected(std::invalid_argument(
            "missing \"start\" field at parsing json data of TimeSequence type"));
        if(!obj.contains("end"))
            return std::unexpected(std::invalid_argument(
            "missing \"end\" field at parsing json data of TimeSequence type"));
        if(!obj.contains("intervals"))
            return std::unexpected(std::invalid_argument(
            "missing \"intervals\" field at parsing json data of TimeSequence type"));
        auto start_parse = from_json<TimeSequence::from_t>(obj.at("start"));
        auto end_parse = from_json<TimeSequence::to_t>(obj.at("end"));
        auto intervals_parse = from_json<TimeSequence::intervals_t>(obj.at("intervals"));
        if(!start_parse.has_value())
            return std::unexpected(start_parse.error());
        if(!end_parse.has_value())
            return std::unexpected(end_parse.error());
        if(!intervals_parse.has_value())
            return std::unexpected(intervals_parse.error());
        std::error_code err;
        TimeSequence result(start_parse.value(),end_parse.value(),intervals_parse.value(),err);
        if(err)
            return std::unexpected(std::invalid_argument("invalid TimeSequence initialization from json data"));
        else return result;
    }
    else if(ts.is_null()){
        return TimeSequence();
    }
    else return std::unexpected(std::invalid_argument(
        "not object-type detected at parsing json data for TimeSequence"));
}