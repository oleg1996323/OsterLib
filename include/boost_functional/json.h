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
template<template<typename VAL> typename RANGE,typename VAL>
std::expected<RANGE<VAL>,std::exception> from_json(const boost::json::value& val) requires(std::ranges::range<RANGE<VAL>>);

template<template<typename VAL> typename RANGE,typename VAL>
std::expected<RANGE<VAL>,std::exception> from_json(const boost::json::value& val) requires(std::ranges::range<RANGE<VAL>>){
    if constexpr(is_associative_container_v<RANGE<VAL>>){
        RANGE<VAL> result;
        if(val.is_array()){
            for(auto arr_val:val.as_array())
                result.insert(from_json<VAL>(arr_val));
            return result;
        }
        else if(val.is_object()){
            for(auto arr_val:val.as_object())
                result.insert(from_json<VAL>(arr_val));
            return result;
        }
        else return std::unexpected(std::invalid_argument("Not structured data type"));
    }
    else{
        RANGE<VAL> result;
        if(val.is_array()){
            if constexpr(requires{std::declval<RANGE<VAL>>().reserve();})
                result.reserve(val.as_array().size());
            for(auto arr_val:val.as_array())
                result.insert(result.end(),from_json<VAL>(arr_val));
            return result;
        }
        else return std::unexpected(std::invalid_argument("Not structured data type"));
    }
}

template<typename T>
std::expected<T,std::exception> from_json(const boost::json::value& val){
    if constexpr (std::is_enum_v<T>)
        return from_json<std::underlying_type_t<T>>(val);
    if constexpr (std::is_integral_v<T>){
        if constexpr(std::is_floating_point_v<T>){
            if(val.is_double())
                return val.to_number<T>();
            else return std::unexpected(std::invalid_argument("Not floating point data type"));
        }
        else if constexpr(!std::is_signed_v<T>){
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
    else static_assert(false,"Not implemented from_json function");
}

template<typename T>
boost::json::value to_json(const T& val);