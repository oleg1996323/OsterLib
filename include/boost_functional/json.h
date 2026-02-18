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

template<typename T>
boost::json::value to_json(const T& val);

template<numeric_types_concept T>
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
    else{
        static_assert(std::is_integral_v<T>);
        if constexpr(std::is_same_v<T,bool>){
            if(val.is_bool())
                return val.as_bool();
            else return std::unexpected(std::invalid_argument("Not boolean data type"));
        }
        else if constexpr(!std::is_signed_v<T>){
            if(val.is_uint64() || (val.is_int64() && val.as_int64()>=0))
                return val.to_number<T>();
            else return std::unexpected(std::invalid_argument("Not unsigned integer data type"));
        }
        else {
            if(val.is_int64() || (val.is_uint64() && val.as_uint64()<std::numeric_limits<int64_t>::max()))
                return val.to_number<T>();
            else return std::unexpected(std::invalid_argument("Not signed integer data type"));
        }
    }
}

template<String T>
std::expected<T,std::exception> from_json(const boost::json::value& val){
    if(val.is_string())
        return val.as_string().subview();
    else return std::unexpected(std::invalid_argument("Not string data type"));
}

template<IsOptional T>
std::expected<T,std::exception> from_json(const boost::json::value& val){
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

template<pair_concept T>
std::expected<T,std::exception> from_json(const boost::json::value& val){
    std::pair<typename T::first_type,typename T::second_type> result;
    if(val.is_array() && val.as_array().size()<3){
        if(auto first_result = from_json<typename T::first_type>(val.as_array()[0]);first_result.has_value()){
            if(auto second_result = from_json<typename T::second_type>(val.as_array()[1]);second_result.has_value())
                return std::make_pair<typename T::first_type,typename T::second_type>(std::move(first_result.value()),std::move(second_result.value()));
            else return std::unexpected(second_result.error());
        }
        else return std::unexpected(first_result.error());
    }
    else return std::unexpected(std::exception());
}

template<String T>
boost::json::value to_json(const T& val){
    return boost::json::value(std::string(val));
}

template<numeric_types_concept T>
boost::json::value to_json(const T& val){
    if constexpr (std::is_enum_v<T>){
        return to_json(static_cast<std::underlying_type_t<T>>(val));
    }
    else{
        static_assert(std::is_floating_point_v<T> || std::is_integral_v<T>);
        boost::json::value result;
        if constexpr (std::is_floating_point_v<T>)
            result.emplace_double();
        else if constexpr(std::is_signed_v<T>)
            result.emplace_int64();
        else result.emplace_uint64();
        result = val;
        return result;
    }
}

template<pair_concept T>
boost::json::value to_json(const T& val){
    boost::json::array result;
    result.emplace_back(to_json(val.first));
    result.emplace_back(to_json(val.second));
    return result;
}

template<IsOptional T>
boost::json::value to_json(const T& val){
    boost::json::value result;
    if(val.has_value())
        result = to_json(val.value());
    return result;
}

template<std::ranges::range RANGE>
requires (!String<RANGE>)
std::expected<RANGE,std::exception> from_json(const boost::json::value& val){
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
            if constexpr(requires{std::declval<RANGE>().reserve(0);})
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

template<std::ranges::range RANGE>
requires (!String<RANGE>)
boost::json::value to_json(const RANGE& range){
    if constexpr(is_associative_container_v<RANGE>){
        if constexpr(String<typename RANGE::key_value>){
            boost::json::object result;
            for(const auto& [key,value]:range)
                result[key] = to_json(value);
            return result;
        }
        else{
            boost::json::array result;
            result.reserve(range.size());
            for(auto& [key,val]:range){
                auto& key_val = result.emplace_back(boost::json::array()).as_array();
                key_val.resize(2);
                key_val.at(0) = to_json(key);
                key_val.at(1) = to_json(val);
            }
            return result;
        }
    }
    else{
        boost::json::array result;
        result.reserve(range.size());
        for(const auto& val:range)
            result.push_back(to_json(val));
        return result;
    }
}