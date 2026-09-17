#include "OsterLib/types/position.h"

template<>
boost::json::value to_json<Position>(const Position& coord){
    boost::json::object result;
    result["y"] = to_json(coord.y_);
    result["x"] = to_json(coord.x_);
    return result;
}

template<>
std::expected<Position,std::exception> from_json<Position>(const boost::json::value& json){
    if(!json.is_object())
        return std::unexpected(std::exception());
    else{
        const boost::json::object& obj = json.as_object();
        if(obj.contains("y") && obj.contains("x")){
            
            if(auto y = from_json<Y>(obj.at("y"));y.has_value()){
                if(auto x = from_json<X>(obj.at("x"));x.has_value()){
                    return Position{.y_=*y,.x_=*x};
                }
                else return std::unexpected(std::exception());
            }
            else return std::unexpected(std::exception());
        }
        else return std::unexpected(std::exception());
    }
}