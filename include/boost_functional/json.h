#pragma once
#include <boost/json.hpp>
#include <expected>
#include <fstream>
#include <filesystem>
#include <vector>
#include <string>
#include <stdexcept>
#include "concepts.h"

namespace fs = std::filesystem;

std::expected<boost::json::value,std::error_code> parse_json_from_file(const fs::path& path) noexcept;
std::expected<boost::json::value,std::error_code> parse_json_from_buffer(const std::string& input) noexcept;
std::expected<boost::json::value,std::error_code> parse_json_from_buffer(std::string_view input) noexcept;

template<typename T>
std::expected<T,std::exception> from_json(const boost::json::value& val);
template<typename RANGE>
std::expected<RANGE,std::exception> from_json(const boost::json::value& val) requires(std::ranges::range<RANGE>);

template<typename RANGE>
std::expected<RANGE,std::exception> from_json(const boost::json::value& val) requires(std::ranges::range<RANGE>){
    if constexpr(is_associative_container_v<RANGE>){
        RANGE result;
        if(val.is_array()){
            for(const auto& arr_val:val.as_array()){
                if(arr_val.is_array()){
                    if(arr_val.as_array().size()==2){
                        if(auto tmp = from_json<typename RANGE::value_type>(arr_val);tmp.has_value())
                            result.insert(std::move(tmp.value()));
                        return std::unexpected(std::invalid_argument("Invalid range value type"));
                    }
                    else return std::unexpected(std::invalid_argument("Invalid size of key-value range segment"));
                }
                else return std::unexpected(std::invalid_argument("Invalid type in mapped structure"));
            }
            return result;
        }
        else if(val.is_object()){
            if constexpr(String<typename RANGE::key_value>){
                for(const auto& [key,value]:val.as_object()){
                    if(auto tmp = from_json<typename RANGE::value_type>(value);tmp.has_value())
                        result.insert(std::make_pair<std::decay_t<decltype(key)>,typename std::decay_t<decltype(tmp)>::value_type>(key,std::move(tmp.value())));
                    return std::unexpected(std::invalid_argument("Invalid range value type"));
                }
                return result;
            }
            else return std::unexpected(std::invalid_argument("Invalid key type: must be string"));
        }
        else return std::unexpected(std::invalid_argument("Not structured data type"));
    }
    else{
        RANGE result;
        if(val.is_array()){
            if constexpr(requires{std::declval<RANGE>().reserve();})
                result.reserve(val.as_array().size());
            for(const auto& arr_val:val.as_array()){
                if(auto tmp = from_json<typename RANGE::value_type>(arr_val);tmp.has_value())
                    result.insert(result.end(),std::move(tmp.value()));
                else return std::unexpected(std::invalid_argument("Invalid range value type"));
            }
            return result;
        }
        else return std::unexpected(std::invalid_argument("Not structured data type"));
    }
}

template<typename T>
std::expected<T,std::exception> from_json(const boost::json::value& val){
    if constexpr (std::is_enum_v<T>){
	if(auto tmp = from_json<std::underlying_type_t<T>>(val);!tmp.has_value())
		return std::unexpected(tmp.error());
	else return static_cast<T>(tmp.value());
    }
    else if constexpr(std::is_floating_point_v<T>){
            if(val.is_double())
                return val.to_number<T>();
            else return std::unexpected(std::invalid_argument("Not floating point data type"));
	}
    else if constexpr (std::is_integral_v<T>){

        if constexpr(!std::is_signed_v<T>){
            if(val.is_uint64())
                return val.to_number<T>();
            else return std::unexpected(std::invalid_argument("Not unsigned integer data type"));
        }
        else {
            if(val.is_int64())
                return val.to_number<T>();
            else return std::unexpected(std::invalid_argument("Not signed integer data type"));
        }
    }
    else if constexpr (std::is_same_v<std::string,T> || std::is_same_v<boost::json::string,T>){
        if(val.is_string())
            return val.as_string();
        else return std::unexpected(std::invalid_argument("Not string data type"));
    }
    else if constexpr (pair_concept<T>){
        std::pair<typename T::first_type,typename T::second_type> result;
        if(auto first_result = from_json<typename T::first_type>(val);first_result.has_value()){
            if(auto second_result = from_json<typename T::second_type>(val);second_result.has_value())
                return std::make_pair<typename T::first_type,typename T::second_type>(std::move(first_result.value()),std::move(second_result.value()));
            else return std::unexpected(second_result.error());
        }
        else return std::unexpected(first_result.error());
    }
    else static_assert(false,"Not implemented from_json function");
}

template<typename T>
boost::json::value to_json(const T& val);

template<typename T>
boost::json::value to_json(const std::optional<T>& val){
    boost::json::value result;
    if(val.has_value())
        result = to_json(val.value());
    return result;
}

template<typename T>
std::expected<std::optional<T>,std::exception> to_json(const boost::json::value& val){
    if(val.is_null())
        return std::unexpected(std::exception());
    else{
        if(auto opt_res = from_json<T>(val);!opt_res.has_value())
            return std::unexpected(std::exception());
        else{
            std::optional<T> result = std::move(opt_res.value());
            return result;
        }
    }
}

template<typename T>
boost::json::value to_json(const T& val){
    if constexpr (std::is_enum_v<T>){
        return to_json(static_cast<T>(val));
    }
    else if constexpr(std::is_floating_point_v<T> || std::is_integral_v<T> || std::is_same_v<std::string,T> || std::is_same_v<std::string_view,T>){
        return val;
    }
    else if constexpr (pair_concept<T>){
        boost::json::array result;
        result.emplace_back(to_json(val.first));
        result.emplace_back(to_json(val.second));
        return result;
    }
    else static_assert(false,"Not implemented to_json function");
}
