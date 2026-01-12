#include "coord.h"

bool is_correct_pos(const Coord* pos){
	if(pos->lon_>=0 && pos->lon_<=180 && pos->lat_<=90 && pos->lat_>=-90)
		return true;
	else return false;
}

bool is_correct_pos(const Coord& pos){
	if(pos.lon_>=0 && pos.lon_<=180 && pos.lat_<=90 && pos.lat_>=-90)
		return true;
	else return false;
}

template<>
boost::json::value to_json<Coord>(const Coord& coord){
    boost::json::object result;
    result["lat"] = to_json(coord.lat_);
    result["lon"] = to_json(coord.lon_);
    return result;
}

template<>
std::expected<Coord,std::exception> from_json<Coord>(const boost::json::value& json){
    if(!json.is_object())
        return std::unexpected(std::exception());
    else{
        const boost::json::object& obj = json.as_object();
        if(obj.contains("lat") && obj.contains("lon")){
            
            if(auto lat = from_json<Lat>(obj.at("lat"));lat.has_value() && *lat>=-90. && *lat<=90.){
                if(auto lon = from_json<Lon>(obj.at("lon"));lon.has_value() && *lon>=-180. && *lon<=180.){
                    return Coord{.lat_=*lat,.lon_=*lon};
                }
                else return std::unexpected(std::exception());
            }
            else return std::unexpected(std::exception());
        }
        else return std::unexpected(std::exception());
    }
}