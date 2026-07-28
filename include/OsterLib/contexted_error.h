#pragma once
#include <system_error>
#include <vector>
#include <utility>
#include <format>
#include <iterator>

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

class ContextedError {
    std::string context;
    std::vector<Field> fields_;
    std::error_code code_;

    public:
    ContextedError() = default;
    ContextedError(std::error_code ec, std::string ctx = "")
        : code_(ec), context(std::move(ctx)) {}

    template<typename ERRC>
    ContextedError(ERRC ec, std::string ctx = "")
        : ContextedError(make_error_code(ec), std::move(ctx))
    {
        static_assert(std::is_error_code_enum_v<ERRC>,
                "ERRC must be a registered error_code enumeration");
    }
    ContextedError(const ContextedError& other)=default;
    ContextedError(ContextedError&& other) noexcept=default;
    ContextedError& operator=(const ContextedError& other)=default;
    ContextedError& operator=(ContextedError&& other) noexcept=default;
    std::error_code code() const noexcept{
        return code_;
    }
    bool empty() const noexcept{
        return !code_ && context.empty() && fields_.empty();
    }
    explicit operator bool() const noexcept{
        return code_.value()!=0;
    }
    template<typename ERRC>
    ContextedError& error(ERRC code,std::string ctx = "")& noexcept{
        static_assert(std::is_error_code_enum_v<ERRC>,
                "ERRC must be a registered error_code enumeration");
        fields_.clear();
        code_ = make_error_code(code);
        context = std::move(ctx);
        return *this;
    }
    ContextedError& error(std::error_code code,std::string ctx = "")& noexcept{
        fields_.clear();
        code_ = code;
        context = std::move(ctx);
        return *this;
    }
    template<typename ERRC>
    ContextedError&& error(ERRC code,std::string ctx = "")&& noexcept{
        static_assert(std::is_error_code_enum_v<ERRC>,
                "ERRC must be a registered error_code enumeration");
        fields_.clear();
        code_ = make_error_code(code);
        context = std::move(ctx);
        return std::move(*this);
    }
    ContextedError&& error(std::error_code code,std::string ctx = "")&& noexcept{
        fields_.clear();
        code_ = code;
        context = std::move(ctx);
        return std::move(*this);
    }
    void clear() noexcept{
        code_.clear();
        context.clear();
        fields_.clear();
        fields_.shrink_to_fit();
    }
    ContextedError& with_field(std::string key, std::string value) &{
        if(key.empty())
            return *this;
        fields_.push_back(Field(std::move(key),std::move(value)));
        return *this;
    }
    ContextedError&& with_field(std::string key, std::string value) &&{
        if(key.empty())
            return std::move(*this);
        fields_.push_back(Field(std::move(key),std::move(value)));
        return std::move(*this);
    }
    template<typename T>
    ContextedError& with_field(std::string key, T value) &{
        if constexpr(std::is_arithmetic_v<T>){
            if(key.empty())
                return *this;
            fields_.push_back(Field(std::move(key),std::to_string(value)));
            return *this;
        }
        else if constexpr(std::is_convertible_v<T,std::string> ||
            std::is_same_v<std::decay_t<T>,std::string_view>){
            return with_field(std::move(key),std::string(value));
        }
        else static_assert(std::is_arithmetic_v<T> || 
            std::is_convertible_v<T,std::string> ||
            std::is_same_v<std::decay_t<T>,std::string_view>);
        return *this;
    }
    template<typename T>
    ContextedError&& with_field(std::string key, T value) &&{
        if constexpr(std::is_arithmetic_v<T>){
            if(key.empty())
                return std::move(*this);
            fields_.push_back(Field(std::move(key),std::to_string(value)));
            return std::move(*this);
        }
        else if constexpr(std::is_convertible_v<T,std::string> ||
            std::is_same_v<std::decay_t<T>,std::string_view>){
            return std::move(*this).with_field(std::move(key),std::string(value));
        }
        else static_assert(std::is_arithmetic_v<T> || 
            std::is_convertible_v<T,std::string> ||
            std::is_same_v<std::decay_t<T>,std::string_view>);
        return std::move(*this);
    }
    ContextedError& with_context(std::string additional) &{
        if(context.empty())
            context = std::move(additional);
        else{
            context +="; ";
            context +=additional;
        }
        return *this;
    }
    ContextedError&& with_context(std::string additional) &&{
        if(context.empty())
            context = std::move(additional);
        else{
            context +="; ";
            context +=additional;
        }
        return std::move(*this);
    }
    std::string what() const {
        std::string msg = code_.message();
        if (!context.empty())
            msg += ": " + context;
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
        return msg;
    }
};

using error = ContextedError;
}
