#include "OsterLib/types/coord.h"

bool is_correct_pos(const Coord* pos){
	if(std::abs(pos->lon_)<=180 && std::abs(pos->lat_)<=90)
		return true;
	else return false;
}

bool is_correct_pos(const Coord& pos){
	if(std::abs(pos.lon_)<=180 && std::abs(pos.lat_)<=90)
		return true;
	else return false;
}