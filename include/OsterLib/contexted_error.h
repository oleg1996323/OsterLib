#pragma once
#include <system_error>
#include <vector>
#include <utility>
#include <format>
#include <iterator>
#include <stdexcept>

namespace std{
    template<typename ERRC>
    error_code make_error_code(ERRC code) noexcept{
        static_assert(std::is_error_code_enum_v<ERRC> && 
                std::is_enum_v<ERRC> && 
                !std::is_same_v<errc,ERRC> && 
                !std::is_same_v<errc,io_errc>);
        return std::make_error_code(code);
    }
}

namespace osterlib{

template<typename ERRC>
std::error_code make_error_code(ERRC errc) noexcept{
    return std::make_error_code<ERRC>(errc);
}

class Field{
    std::string key_;
    std::string value_;
    public:
    Field(std::string key,
        std::string value) noexcept
    {
        key_ = std::move(key);
        value_ = std::move(value);
    }
    Field(const Field& other)
    {
        key_ = other.key_;
        value_ = other.value_;
    }
    Field(Field&& other) noexcept
    {
        key_ = std::move(other.key_);
        value_ = std::move(other.value_);
    }
    Field& operator=(const Field& other)
    {
        if(this!=&other){
            key_ = other.key_;
            value_ = other.value_;
        }
        return *this;
    }
    Field& operator=(Field&& other) noexcept
    {
        if(this!=&other){
            key_ = std::move(other.key_);
            value_ = std::move(other.value_);
        }
        return *this;
    }
    const std::string& key() const noexcept{
        return key_;
    }
    const std::string& value() const noexcept{
        return value_;
    }
};

class ContextedError:public std::exception {
    std::string context_;
    std::vector<Field> fields_;
    std::error_code code_;
    mutable std::string cached_string_;

    public:
    ContextedError() = default;
    ContextedError(std::error_code ec, std::string ctx = "")
        : code_(ec), context_(std::move(ctx)) {}

    template<typename ERRC>
    ContextedError(ERRC ec, std::string ctx = "")
        : ContextedError(make_error_code(ec), std::move(ctx))
    {
        static_assert(std::is_error_code_enum_v<ERRC>,
                "ERRC must be a registered error_code enumeration");
    }
    ContextedError(const ContextedError& other):
        context_(other.context_),
        fields_(other.fields_),
        code_(other.code_),
        cached_string_(other.cached_string_){}
    ContextedError(ContextedError&& other) noexcept:
        context_(std::move(other.context_)),
        fields_(std::move(other.fields_)),
        code_(std::move(other.code_)),
        cached_string_(std::move(other.cached_string_)){}
    ContextedError& operator=(const ContextedError& other){
        if(this!=&other){
            context_ = other.context_;
            fields_=other.fields_;
            code_ = other.code_;
            cached_string_ = other.cached_string_;
        }
        return *this;
    }
    ContextedError& operator=(ContextedError&& other) noexcept{
        if(this!=&other){
            context_ = std::move(other.context_);
            fields_ = std::move(other.fields_);
            code_ = std::move(other.code_);
            cached_string_ = std::move(other.cached_string_);
        }
        return *this;
    }
    std::error_code code() const noexcept{
        return code_;
    }
    bool empty() const noexcept{
        return !code_ && context_.empty() && fields_.empty();
    }
    explicit operator bool() const noexcept{
        return code_.value()!=0;
    }
    template<typename ERRC>
    ContextedError& error(ERRC code,std::string ctx = "")& noexcept{
        static_assert(std::is_error_code_enum_v<ERRC>,
                "ERRC must be a registered error_code enumeration");
        cached_string_.clear();
        fields_.clear();
        code_ = make_error_code(code);
        context_ = std::move(ctx);
        return *this;
    }
    ContextedError& error(std::error_code code,std::string ctx = "")& noexcept{
        cached_string_.clear();
        fields_.clear();
        code_ = code;
        context_ = std::move(ctx);
        return *this;
    }
    template<typename ERRC>
    ContextedError&& error(ERRC code,std::string ctx = "")&& noexcept{
        static_assert(std::is_error_code_enum_v<ERRC>,
                "ERRC must be a registered error_code enumeration");
        cached_string_.clear();
        fields_.clear();
        code_ = make_error_code(code);
        context_ = std::move(ctx);
        return std::move(*this);
    }
    ContextedError&& error(std::error_code code,std::string ctx = "")&& noexcept{
        cached_string_.clear();
        fields_.clear();
        code_ = code;
        context_ = std::move(ctx);
        return std::move(*this);
    }
    void clear() noexcept{
        cached_string_.clear();
        code_.clear();
        context_.clear();
        fields_.clear();
        fields_.shrink_to_fit();
    }
    ContextedError& with_field(std::string key, std::string value) &{
        cached_string_.clear();
        if(key.empty())
            return *this;
        fields_.push_back(Field(std::move(key),std::move(value)));
        return *this;
    }
    ContextedError&& with_field(std::string key, std::string value) &&{
        cached_string_.clear();
        if(key.empty())
            return std::move(*this);
        fields_.push_back(Field(std::move(key),std::move(value)));
        return std::move(*this);
    }
    template<typename T>
    ContextedError& with_field(std::string key, T value) &{
        cached_string_.clear();
        if constexpr(std::is_arithmetic_v<T>){
            if(key.empty())
                return *this;
            fields_.push_back(Field(std::move(key),std::to_string(value)));
            return *this;
        }
        else if constexpr(std::is_enum_v<T>){
            if(key.empty())
                return *this;
            fields_.push_back(Field(std::move(key),std::to_string(static_cast<std::underlying_type_t<T>>(value))));
            return *this;
        }
        else if constexpr(std::is_convertible_v<T,std::string> ||
            std::is_same_v<std::decay_t<T>,std::string_view>){
            return with_field(std::move(key),std::string(value));
        }
        else static_assert(std::is_arithmetic_v<T> || 
            std::is_convertible_v<T,std::string> ||
            std::is_same_v<std::decay_t<T>,std::string_view> ||
            std::is_enum_v<T>);
        return *this;
    }
    template<typename T>
    ContextedError&& with_field(std::string key, T value) &&{
        cached_string_.clear();
        if constexpr(std::is_arithmetic_v<T>){
            if(key.empty())
                return std::move(*this);
            fields_.push_back(Field(std::move(key),std::to_string(value)));
            return std::move(*this);
        }
        else if constexpr(std::is_enum_v<T>){
            if(key.empty())
                return std::move(*this);
            fields_.push_back(Field(std::move(key),std::to_string(static_cast<std::underlying_type_t<T>>(value))));
            return std::move(*this);
        }
        else if constexpr(std::is_convertible_v<T,std::string> ||
            std::is_same_v<std::decay_t<T>,std::string_view>){
            return std::move(*this).with_field(std::move(key),std::string(value));
        }
        else static_assert(std::is_arithmetic_v<T> || 
            std::is_convertible_v<T,std::string> ||
            std::is_same_v<std::decay_t<T>,std::string_view> ||
            std::is_enum_v<T>);
        return std::move(*this);
    }
    ContextedError& with_context(std::string additional) &{
        cached_string_.clear();
        if(context_.empty())
            context_ = std::move(additional);
        else{
            context_ +="; ";
            context_ +=additional;
        }
        return *this;
    }
    ContextedError&& with_context(std::string additional) &&{
        cached_string_.clear();
        if(context_.empty())
            context_ = std::move(additional);
        else{
            context_ +="; ";
            context_ +=additional;
        }
        return std::move(*this);
    }
    const char* what() const noexcept override{
        if(cached_string_.empty()){
            std::string msg = code_.message();
            if (!context_.empty())
                msg += ": " + context_;
            if(!fields_.empty()){
                msg.reserve(64);
                msg+='(';
                for (size_t i = 0; i < fields_.size(); ++i) {
                    if (i > 0) std::format_to(std::back_inserter(msg), ", ");
                    std::format_to(std::back_inserter(msg),
                    "{}={}", fields_[i].key(), fields_[i].value());
                }
                std::format_to(std::back_inserter(msg), ")");
            }
            cached_string_.swap(msg);
        }
        return cached_string_.c_str();
    }
    const std::string& context() const noexcept{
        return context_;
    }
    const std::vector<Field> fields() const noexcept{
        return fields_;
    }
};

using ctx_error = ContextedError;
}

#include "OsterLib/serialization.h"

namespace serialization{
    template<bool NETWORK_ORDER>
    struct Serialize<NETWORK_ORDER,osterlib::Field>{
        using type = osterlib::Field;
        SerializationEC operator()(const type& msg, std::vector<char>& buf) const noexcept{
            return serialize<NETWORK_ORDER>(msg,buf,msg.key(),msg.value());
        }
    };

    template<bool NETWORK_ORDER>
    struct Deserialize<NETWORK_ORDER,osterlib::Field>{
        using type = osterlib::Field;
        SerializationEC operator()(type& msg, StreamSerializer& buf) const noexcept{
            std::string key;
            if(auto deser_res = deserialize<NETWORK_ORDER>(key,buf);
                deser_res!=SerializationEC::NONE)
                return deser_res;
            std::string value;
            if(auto deser_res = deserialize<NETWORK_ORDER>(value,buf);
                deser_res!=SerializationEC::NONE)
                return deser_res;
            msg = std::move(osterlib::Field(std::move(key),std::move(value)));
            return SerializationEC::NONE;
        }
    };

    template<>
    struct Serial_size<osterlib::Field>{
        using type = osterlib::Field;
        size_t operator()(const type& msg) const noexcept{
            return serial_size(msg.key(),msg.value());
        }
    };

    template<>
    struct Min_serial_size<osterlib::Field>{
        using type = osterlib::Field;
        static constexpr size_t value = []()
        {
            return min_serial_size<
                std::decay_t<std::invoke_result_t<decltype(&type::key),type*>>,
                std::decay_t<std::invoke_result_t<decltype(&type::value),type*>>>();
        }();
    };

    template<>
    struct Max_serial_size<osterlib::Field>{
        using type = osterlib::Field;
        static constexpr size_t value = []()
        {
            return max_serial_size<
                std::decay_t<std::invoke_result_t<decltype(&type::key),type*>>,
                std::decay_t<std::invoke_result_t<decltype(&type::value),type*>>>();
        }();
    };
}