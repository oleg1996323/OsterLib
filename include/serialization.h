#pragma once
#include <optional>
#include <vector>
#include <bit>
#include "float_conv.h"
#include "byte_order.h"
#include <chrono>
#include <cstring>
#include <expected>
#include <concepts>
#include <tuple>
#include <utility>
#include <cstring>
#include <fstream>
#include <chrono>
#include <variant>
#include <cassert>
#include "concepts.h"
#include "variant.h"
#include "serialization/multi_buffer.h"

namespace serialization{
    
    enum class SerializationEC{
        NONE,
        BUFFER_SIZE_LESSER,
        UNMATCHED_TYPE,
        BUFFER_OVERFLOW,
        FILE_WRITING_ERROR,
        UNEXPECTED_EOF,
        FILE_READING_ERROR
    };

    template<bool NETWORK_ORDER,typename T>
    struct Serialize;

    template<bool NETWORK_ORDER,typename T>
    struct Deserialize;

    template<typename T>
    struct Serial_size;

    template<typename T>
    struct Max_serial_size;

    template<typename T>
    struct Min_serial_size;

    template<bool NETWORK = false,typename T>
    SerializationEC serialize_to_file(const T& val,std::ofstream& fstream) noexcept;
    template<bool NETWORK = false,typename T>
    SerializationEC deserialize_from_file(T& val,std::ifstream& fstream) noexcept;

    template<typename T>
    SerializationEC serialize_native(const T& val,std::vector<char>& buf) noexcept;
    template<typename T>
    SerializationEC serialize_network(const T& val,std::vector<char>& buf) noexcept;
    template<typename T>
    SerializationEC deserialize_native(T& to_deserialize,MultiBufferView& buf) noexcept;
    template<typename T>
    SerializationEC deserialize_network(T& to_deserialize,MultiBufferView& buf) noexcept;

    template<typename T,typename... ARGS>
    SerializationEC serialize_native(const T& val,std::vector<char>& buf,const ARGS&... args) noexcept;
    template<typename T,typename... ARGS>
    SerializationEC serialize_network(const T& val,std::vector<char>& buf,const ARGS&... args) noexcept;
    template<typename T,typename... ARGS>
    SerializationEC deserialize_native(T& to_deserialize,MultiBufferView& buf,ARGS&... args) noexcept;
    template<typename T,typename... ARGS>
    SerializationEC deserialize_network(T& to_deserialize,MultiBufferView& buf,ARGS&... args) noexcept;

    template<typename... ARGS>
    size_t serial_size(const ARGS&... val) noexcept;
    template<typename... ARGS>
    constexpr size_t min_serial_size() noexcept;
    template<typename... ARGS>
    constexpr size_t max_serial_size() noexcept;
    template<typename... ARGS>
    size_t min_serial_size(const ARGS&... val) noexcept;
    template<typename... ARGS>
    size_t max_serial_size(const ARGS&...val) noexcept;

    template<bool NETWORK_ORDER,typename T>
    SerializationEC serialize(const T& val,std::vector<char>& buf) noexcept;
    template<bool NETWORK_ORDER,typename T>
    SerializationEC deserialize(T& val,MultiBufferView& buf) noexcept;

    template<bool NETWORK_ORDER,typename T,typename... ARGS>
    requires (sizeof...(ARGS)>0)
    SerializationEC serialize(const T& val,std::vector<char>& buf,const ARGS&... args) noexcept;
    template<bool NETWORK_ORDER,typename T,typename... ARGS>
    requires (sizeof...(ARGS)>0)
    SerializationEC deserialize(const T& to_deserialize,MultiBufferView& buf,ARGS&... args) noexcept;

    template<bool NETWORK_ORDER,typename T>
    concept serialize_concept = 
    requires(const T& val, std::vector<char>& buf){
        { Serialize<NETWORK_ORDER,T>{}(val, buf) } -> std::same_as<SerializationEC>;
    };

    template<bool NETWORK_ORDER,typename T>
    concept deserialize_concept = 
    requires(T& val,MultiBufferView& buf){
        { Deserialize<NETWORK_ORDER,T>{}(val,buf) } -> std::same_as<SerializationEC>;
    };

    template<typename T>
    concept serial_size_concept = 
    requires(const T& val){
        { Serial_size<T>{}(val) } -> std::same_as<size_t>;
    };

    template<typename... ARGS>
    size_t serial_size(const ARGS&... val) noexcept{
        if constexpr (sizeof...(ARGS)>1)
            return ((serial_size(val)) + ...);
        else if constexpr(sizeof...(ARGS)==1){
            return Serial_size<ARGS...>{}(val...);
        }
        else{
            static_assert(false,"Must be at least 1 argument");
            return 0;
        }
    }

    template<class T, bool = std::is_enum_v<std::remove_cvref_t<T>>>
    struct RawTypeImpl
    {
        using type = std::remove_cvref_t<T>;
    };

    template<class T>
    struct RawTypeImpl<T, true>
    {
        using type = std::underlying_type_t<std::remove_cvref_t<T>>;
    };

    template<class T>
    using RawType_t = typename RawTypeImpl<T>::type;


    template<bool NETWORK_ORDER,typename T>
    struct Serialize{
        SerializationEC operator()(const T& val,std::vector<char>& buf) const noexcept{
            if constexpr (numeric_types_concept<T>){
                if constexpr (std::is_integral_v<T> || std::is_enum_v<T>){
                    using RawType = RawType_t<T>;
                    RawType raw_val = static_cast<RawType>(val);
                    if constexpr(sizeof(T)==1)
                        buf.push_back(static_cast<char>(raw_val));
                    else{
                        if constexpr(NETWORK_ORDER)
                            if(is_little_endian())
                                raw_val = std::byteswap(raw_val);
                        const auto* begin = reinterpret_cast<const char*>(&raw_val);
                        buf.insert(buf.end(),begin,begin+sizeof(raw_val));
                    }
                }
                else{
                    return serialize<NETWORK_ORDER>(to_integer(val),buf);
                }
                return SerializationEC::NONE;
            }
            else if constexpr (time_point_concept<T>){
                int64_t time_count = std::chrono::duration_cast<std::chrono::nanoseconds>(val.time_since_epoch()).count();
                return serialize<NETWORK_ORDER>(time_count,buf);
            }
            else if constexpr (duration_concept<T>){
                int64_t time_count = std::chrono::duration_cast<std::chrono::nanoseconds>(val).count();
                return serialize<NETWORK_ORDER>(time_count,buf);
            }
            else if constexpr (smart_pointer_concept<std::decay_t<T>>){
                SerializationEC err;
                if(val){
                    if(err=serialize<NETWORK_ORDER>(true,buf);err==SerializationEC::NONE)
                        return serialize<NETWORK_ORDER>(*(val.get()),buf);
                    else return err;
                }
                else return serialize<NETWORK_ORDER>(false,buf);
            }
            else if constexpr (weak_pointer_concept<std::decay_t<T>>){
                SerializationEC err;
                if(!val.expired())
                    return serialize<NETWORK_ORDER>(val.lock(),buf);
                else return serialize<NETWORK_ORDER>(false,buf);
            }
            else if constexpr(std::is_empty_v<T>)
                return SerializationEC::NONE;
            else if constexpr(pair_concept<T>){
                if(SerializationEC err = serialize<NETWORK_ORDER>(val.first,buf);err==SerializationEC::NONE)
                    return serialize<NETWORK_ORDER>(val.second,buf);
                else
                    return err;
            }
            else{
                static_assert(false, "serialize unspecified");
            }
        }
    };

    template<bool NETWORK_ORDER,typename T>
    struct Deserialize{
        /// @brief Deserialize data from buffer to specified type
        /// @tparam T Supported types: integral, floating-point, enum (including scoped enum)
        /// @tparam NETWORK_ORDER If true, converts from network (big-endian) byte order
        /// @param buf Input data buffer (read-only)
        /// @return std::expected<T, SerializationEC> - value or error code
        /// @note Supports both runtime and constexpr contexts
        /// @warning Buffer must be properly aligned for type T
        SerializationEC operator()(T& to_deserialize,MultiBufferView& buf) const noexcept{
            if constexpr (numeric_types_concept<std::decay_t<T>>){
                static_assert(std::is_trivially_copyable_v<std::decay_t<T>>, 
                    "Type T must be trivially copyable");
                if(buf.size()<sizeof(std::decay_t<T>))
                    return SerializationEC::BUFFER_SIZE_LESSER;
                if constexpr (std::is_integral_v<T> || std::is_enum_v<T>){
                    using RawType = RawType_t<T>;
                    RawType value;
                    if constexpr (sizeof(RawType) == 1){
                        to_deserialize = static_cast<std::decay_t<T>>(*buf.data());
                        return SerializationEC::NONE;
                    }
                    if (std::is_constant_evaluated())
                        std::copy_n(buf.data(), sizeof(RawType), reinterpret_cast<char*>(&value));
                    else
                        std::memcpy(&value,buf.data(),sizeof(RawType));
                    if constexpr(NETWORK_ORDER && sizeof(RawType)>1)
                        if(is_little_endian())
                            value = std::byteswap(value);
                    to_deserialize = static_cast<std::decay_t<T>>(value);
                    return SerializationEC::NONE;
                }
                else{
                    using IntType = oster::detail::to_integer_type<sizeof(std::decay_t<T>)>;
                    IntType int_val;
                    if(auto code = deserialize<NETWORK_ORDER>(int_val,buf);code==SerializationEC::NONE){
                        to_deserialize = to_float(int_val);
                        return SerializationEC::NONE;
                    }
                    else return code;
                }
            }
            else if constexpr (time_point_concept<T>){
                if(buf.size()<min_serial_size(to_deserialize))
                    return SerializationEC::BUFFER_SIZE_LESSER;
                int64_t IntVal = 0;
                SerializationEC code = deserialize<NETWORK_ORDER>(IntVal,buf);
                if(code==SerializationEC::NONE)
                    to_deserialize = T(std::chrono::duration_cast<typename T::duration>(std::chrono::nanoseconds(IntVal)));
                return code;
            }
            else if constexpr (duration_concept<T>){
                if(buf.size()<min_serial_size(to_deserialize))
                    return SerializationEC::BUFFER_SIZE_LESSER;
                int64_t IntVal = 0;
                SerializationEC code = deserialize<NETWORK_ORDER>(IntVal,buf);
                if(code==SerializationEC::NONE)
                    to_deserialize = std::chrono::duration_cast<T>(std::chrono::nanoseconds(IntVal));
                return code;
            }
            else if constexpr (smart_pointer_concept<T>){
                bool has_value = false;
                auto code = deserialize<NETWORK_ORDER>(has_value,buf);
                if(code!=SerializationEC::NONE)
                    return code;
                else {
                    if(has_value){
                        if constexpr (requires{std::is_same_v<T,std::unique_ptr<typename T::element_type,
                                                            typename T::deleter_type>>;})
                            to_deserialize = std::make_unique<typename T::element_type>();
                        else
                            to_deserialize = std::make_shared<typename T::element_type>();
                        return  deserialize<NETWORK_ORDER>(*to_deserialize,buf.subspan(sizeof(has_value)));                        
                    }
                    else {
                        to_deserialize.reset();
                        return SerializationEC::NONE;
                    }
                }
            }
            else if constexpr(std::is_empty_v<T>)
                return SerializationEC::NONE;
            else if constexpr(pair_concept<T>){
                return deserialize<NETWORK_ORDER>(to_deserialize,buf,to_deserialize.first,to_deserialize.second);
            }
            else {
                static_assert(false,"deserialize unspecified operator()");
                return SerializationEC::NONE;
            }
        }
    };

    template<typename T>
    struct Serial_size{
        size_t operator()(const T& val) const noexcept{
            if constexpr (numeric_types_concept<T>)
                return sizeof(val);
            else if constexpr (time_point_concept<T>)
                return sizeof(val.time_since_epoch().count());
            else if constexpr (duration_concept<T>)
                return sizeof(val.count());
            else if constexpr (smart_pointer_concept<T>)
                return sizeof(bool)+(val?serial_size(*val):0);
            else if constexpr(weak_pointer_concept<T>)
                return sizeof(bool)+(!val.expired()?serial_size(val.lock()):0);
            else if constexpr(std::is_empty_v<T>)
                return 0;
            else if constexpr(pair_concept<T>)
                return serial_size(val.first)+serial_size(val.second);
            else {
                static_assert(false,"serial_size unspecified operator()");
                return 0;
            }
        }
    };

    template<typename T>
    struct Min_serial_size{
        static constexpr size_t value = []()
        {
            if constexpr (numeric_types_concept<T>)
                return sizeof(T);
            else if constexpr (time_point_concept<T>)
                return sizeof(T);
            else if constexpr (duration_concept<T>)
                return sizeof(T);
            else if constexpr (smart_pointer_concept<T> || weak_pointer_concept<T>)
                return sizeof(bool);
            else if constexpr(std::is_empty_v<T>)
                return 0;
            else if constexpr(pair_concept<T>){
                return Min_serial_size<typename T::first_type>::value+
                        Min_serial_size<typename T::second_type>::value;
            }
            else{
                static_assert(false,"Min_serial_size unspecified structure for current type");
                return 0;
            }
        }();
    };

    template<typename T>
    struct Max_serial_size{
        static constexpr size_t value = []()
        {
            if constexpr (numeric_types_concept<T>)
                return sizeof(T);
            else if constexpr (time_point_concept<T>)
                return sizeof(T);
            else if constexpr (duration_concept<T>)
                return sizeof(T);
            else if constexpr (smart_pointer_concept<T>||weak_pointer_concept<T>)
                return Max_serial_size<typename T::element_type>::value == std::numeric_limits<size_t>::max()?
                        Max_serial_size<typename T::element_type>::value:Max_serial_size<typename T::element_type>::value+sizeof(bool);
            else if constexpr(std::is_empty_v<T>)
                return 0;
            else if constexpr(pair_concept<T>)
                return Max_serial_size<typename T::first_type>::value+
                        Max_serial_size<typename T::second_type>::value;
            else{
                static_assert(false,"Max_serial_size unspecified structure for current type");
                return 0;
            }
        }();
    };

    template<typename... ARGS>
    constexpr size_t min_serial_size() noexcept{
        if constexpr (sizeof...(ARGS)>1)
            return ((min_serial_size<ARGS>()) + ...);
        else if constexpr(sizeof...(ARGS)==1){
            return Min_serial_size<ARGS...>::value;
        }
        else {
            static_assert(false,"Must be at least 1 template argument");
            return 0;
        }
    }

    template<typename... ARGS>
    constexpr size_t max_serial_size() noexcept{
        if constexpr (sizeof...(ARGS)>1){
            size_t result = 0;
            auto add_sz = [&result](size_t value)mutable -> bool
            {
                if(value<std::numeric_limits<size_t>::max()-result){
                    result+=value;
                    return true;
                }
                else {
                    result = std::numeric_limits<size_t>::max();
                    return false;
                }
            };
            (add_sz(max_serial_size<ARGS>()) && ...);
            return result;
        }
        else if constexpr(sizeof...(ARGS)==1){
            return Max_serial_size<ARGS...>::value;
        }
        else{
            static_assert(false,"Must be at least 1 template argument");
            return 0;
        }
    }

    template<typename... ARGS>
    size_t min_serial_size(const ARGS&... val) noexcept{
        return min_serial_size<ARGS...>();
    }
    template<typename... ARGS>
    size_t max_serial_size(const ARGS&...val) noexcept{
        return max_serial_size<ARGS...>();
    }

    template<bool NETWORK_ORDER,typename T>
    requires (serialize_concept<NETWORK_ORDER,T>)
    struct Serialize<NETWORK_ORDER,std::optional<T>>{
        SerializationEC operator()(const std::optional<T>& val,std::vector<char>& buf) const noexcept{
            SerializationEC err = serialize<NETWORK_ORDER>(val.has_value(),buf);
            if(err!=SerializationEC::NONE)
                return err;
            if(!val.has_value())
                return SerializationEC::NONE;
            err = serialize<NETWORK_ORDER>(val.value(),buf);
            if(err!=SerializationEC::NONE)
                return err;
            return SerializationEC::NONE;
        }
    };

    template<bool NETWORK_ORDER,typename T>
    requires deserialize_concept<NETWORK_ORDER,T>
    struct Deserialize<NETWORK_ORDER,std::optional<T>>{
        /// @brief Deserialize data from buffer to specified type
        /// @tparam T Supported types: integral, floating-point, enum (including scoped enum)
        /// @tparam NETWORK_ORDER If true, converts from network (big-endian) byte order
        /// @param buf Input data buffer (read-only)
        /// @return std::expected<T, SerializationEC> - value or error code
        /// @note Supports both runtime and constexpr contexts
        /// @warning Buffer must be properly aligned for type T
        SerializationEC operator()(std::optional<T>& to_deserialize,MultiBufferView& buf) const noexcept{
            to_deserialize.reset();
            if(buf.size()<min_serial_size(to_deserialize))
                return SerializationEC::BUFFER_SIZE_LESSER;
            bool has_value = false;
            SerializationEC code = deserialize<NETWORK_ORDER>(has_value,buf);
            if(code!=SerializationEC::NONE)
                return code;
            if(!has_value)
                return SerializationEC::NONE;
            T value;
            code = deserialize<NETWORK_ORDER>(value,buf.subspan(serial_size(has_value)));
            if(code!=SerializationEC::NONE)
                return code;
            to_deserialize.emplace(std::move(value));
            return SerializationEC::NONE;
        }
    };

    template<typename T>
    struct Serial_size<std::optional<T>>{
        size_t operator()(const std::optional<T>& val) const noexcept{
            return sizeof(bool) + (val.has_value() ? serial_size(*val) : 0);
        }
    };

    template<typename T>
    struct Min_serial_size<std::optional<T>>{
        static constexpr size_t value = []()
        {
            return Min_serial_size<bool>::value;
        }();
    };

    template<typename T>
    struct Max_serial_size<std::optional<T>>{
        static constexpr size_t value = []()
        {
            return ((Max_serial_size<T>::value == std::numeric_limits<size_t>::max())?
                    Max_serial_size<T>::value:(Max_serial_size<T>::value+sizeof(bool)));
        }();
    };

    template<bool NETWORK_ORDER,std::ranges::range T>
    struct Serialize<NETWORK_ORDER,T>{
        SerializationEC operator()(const T& range,std::vector<char>& buf) const noexcept{
            static_assert(serialize_concept<NETWORK_ORDER,std::ranges::range_value_t<T>>);
            SerializationEC err = serialize<NETWORK_ORDER>(range.size(),buf);
            for(const auto& item:range){
                err = serialize<NETWORK_ORDER>(item,buf);
                if(err==SerializationEC::NONE)
                    continue;
                else
                    return err;
            }
            return SerializationEC::NONE;
        }
    };

    template<bool NETWORK_ORDER,std::ranges::range T>
    struct Deserialize<NETWORK_ORDER,T>{
        SerializationEC operator()(T& to_deserialize,MultiBufferView& buf) const noexcept{
            static_assert(deserialize_concept<NETWORK_ORDER,std::ranges::range_value_t<T>>);
            if(buf.size()<min_serial_size(to_deserialize))
                return SerializationEC::BUFFER_SIZE_LESSER;
            size_t range_sz = 0;
            SerializationEC code = deserialize<NETWORK_ORDER>(range_sz,buf);
            if(code!=SerializationEC::NONE)
                return code;
            
            buf = buf.subspan(serial_size(range_sz));
            for (size_t i = 0; i < range_sz; ++i) {
                
                if constexpr(is_associative_container_v<T>){
                    std::pair<typename T::key_type,typename T::mapped_type> item{};
                    code = deserialize<NETWORK_ORDER>(item, buf);
                    if (code != SerializationEC::NONE)
                        return code;
                    buf = buf.subspan(serial_size(item));                    
                    to_deserialize.insert(std::move(item));
                }
                else {
                    std::ranges::range_value_t<T> item{};
                    code = deserialize<NETWORK_ORDER>(item, buf);
                    if (code != SerializationEC::NONE)
                        return code;
                    buf = buf.subspan(serial_size(item));
                    to_deserialize.insert(to_deserialize.end(),std::move(item));
                }
            }
            return SerializationEC::NONE;
        }
    };

    template<std::ranges::range T>
    requires serial_size_concept<std::ranges::range_value_t<T>>
    struct Serial_size<T>{
        size_t operator()(const T& range) const noexcept{
            constexpr size_t size_sz = sizeof(size_t);
            size_t total = size_sz;
            if constexpr(is_associative_container_v<T>)
                for(const auto& [key,val]:range){
                    total+=serial_size(key)+serial_size(val);
                }
            else
                for(const auto& item:range)
                    total+=serial_size(item);
            return total;
        }
    };

    template<std::ranges::range T>
    struct Min_serial_size<T>{
        static constexpr size_t value = []()->size_t
        {
            return sizeof(size_t);
        }();
    };

    template<std::ranges::range T>
    struct Max_serial_size<T>{
        static constexpr size_t value = []()->size_t
        {
            return std::numeric_limits<size_t>::max();
        }();
    };

template<bool NETWORK_ORDER,typename T>
    requires (serialize_concept<NETWORK_ORDER,T>)
    struct Serialize<NETWORK_ORDER,std::reference_wrapper<T>>{
        SerializationEC operator()(const std::reference_wrapper<T>& val,std::vector<char>& buf) const noexcept{
            return serialize<NETWORK_ORDER>(val.get(),buf);
        }
    };

    template<typename T>
    struct Serial_size<std::reference_wrapper<T>>{
        size_t operator()(const std::reference_wrapper<T>& val) const noexcept{
            return serial_size(val.get());
        }
    };

    template<typename T>
    struct Min_serial_size<std::reference_wrapper<T>>{
        static constexpr size_t value = []()
        {
            return Min_serial_size<T>::value;
        }();
    };

    template<typename T>
    struct Max_serial_size<std::reference_wrapper<T>>{
        static constexpr size_t value = []()
        {
            return Max_serial_size<T>::value;
        }();
    };

    template<bool NETWORK_ORDER,typename T>
    requires IsStdVariant<T>
    struct Serialize<NETWORK_ORDER,T>{
        using type = std::decay_t<T>;
        SerializationEC operator()(const type& val, std::vector<char>& buf) const noexcept{
            auto serializer = [&val,&buf](auto& item)
            {
                SerializationEC err = SerializationEC::NONE;
                if(err = serialize<NETWORK_ORDER>(val.index(),buf);err==SerializationEC::NONE)
                    return serialize<NETWORK_ORDER>(item,buf);
                else return err;
            };

            return std::visit(serializer,val);
        }
    };

    template<bool NETWORK_ORDER,typename T>
    requires IsStdVariant<T>
    struct Deserialize<NETWORK_ORDER,T>{
        using type = std::decay_t<T>;
        SerializationEC operator()(type& val, MultiBufferView& buf) const noexcept{
            using factory = ::VariantFactory<type>;
            size_t index = std::numeric_limits<size_t>::max();
            if(SerializationEC err = deserialize<NETWORK_ORDER>(index,buf);err!=SerializationEC::NONE)
                return err;
            else{
                if(!factory::emplace(val,index))
                    return SerializationEC::UNMATCHED_TYPE;
                buf = buf.subspan(serial_size(index));
                auto deserializer = [&buf](auto& item)
                {
                    return deserialize<NETWORK_ORDER>(item,buf);
                };

                return std::visit(deserializer,val);
            }
        }
    };

    template<typename T>
    requires IsStdVariant<T>
    struct Serial_size<T>{
        using type = std::decay_t<T>;
        size_t operator()(const type& val) const noexcept{
            auto serial_sz = [](auto& item)
            {
                return serial_size(item)+sizeof(size_t);
            };
            return std::visit(serial_sz,val);
        }
    };

    template<typename T>
    requires IsStdVariant<T>
    struct Min_serial_size<T>{
        using type = std::decay_t<T>;
        static constexpr size_t value = []() ->size_t
        {
            constexpr size_t calc_size = []<size_t... Is>(std::index_sequence<Is...>) noexcept
            {   
                size_t size_min = std::numeric_limits<size_t>::max();
                ((size_min = std::min(min_serial_size<std::variant_alternative_t<Is,type>>(),size_min)),...);
                size_min+=sizeof(size_t);
                return size_min;
            }(std::make_index_sequence<std::variant_size_v<type>>{});
            return calc_size;
        }();
    };

    template<typename T>
    requires IsStdVariant<T>
    struct Max_serial_size<T>{
        using type = std::decay_t<T>;
        static constexpr size_t value = []() ->size_t
        {
            constexpr size_t calc_size = []<size_t... Is>(std::index_sequence<Is...>) noexcept
            {   
                size_t size_max = 0;
                ((size_max = std::max(max_serial_size<std::variant_alternative_t<Is,type>>(),size_max)),...);
                size_max = (std::numeric_limits<size_t>::max()-sizeof(size_t)<size_max?std::numeric_limits<size_t>::max():size_max+sizeof(size_t));
                return size_max;
            }(std::make_index_sequence<std::variant_size_v<type>>{});
            return calc_size;
        }();
    };

    template<bool NETWORK_ORDER,typename T>
    SerializationEC serialize(const T& val,std::vector<char>& buf) noexcept{
        return Serialize<NETWORK_ORDER,T>{}(val,buf);
    }
    template<bool NETWORK_ORDER,typename T>
    SerializationEC deserialize(T& val,MultiBufferView& buf) noexcept{
        return Deserialize<NETWORK_ORDER,T>{}(val,buf);
    }

    template<bool NETWORK_ORDER,typename T,typename... ARGS>
    requires (sizeof...(ARGS)>0)
    SerializationEC serialize(const T& val,std::vector<char>& buf,const ARGS&... args) noexcept{
        SerializationEC err = SerializationEC::NONE;
        static_assert(min_serial_size<T>()==min_serial_size<ARGS...>(),
        "Minimal serial size of serialized struct must be equal to the minimal serial size of all its members to be serialized");
        static_assert(max_serial_size<T>()==max_serial_size<ARGS...>(),
        "Maximal serial size of serialized struct must be equal to the maximal serial size of all its members to be serialized");
        (((err=serialize<NETWORK_ORDER>(args, buf))==SerializationEC::NONE) && ...);
        return err;
    }
    template<bool NETWORK_ORDER,typename T,typename... ARGS>
    requires (sizeof...(ARGS)>0)
    SerializationEC deserialize(const T& val,MultiBufferView& buf, ARGS&... args) noexcept{
        static_assert(min_serial_size<T>()==min_serial_size<ARGS...>(),"Expected equal minimal serial size of object and its fields' summary minimal serial size");
        static_assert(min_serial_size<T>()==min_serial_size<ARGS...>(),"Expected equal maximal serial size of object and its fields' summary maximal serial size");
        SerializationEC result_code = SerializationEC::NONE;
        if (buf.size() < min_serial_size(val))
            return SerializationEC::BUFFER_SIZE_LESSER;

        auto deserialize_field = [&](auto& field) mutable noexcept->SerializationEC
        {
            SerializationEC code = deserialize<NETWORK_ORDER>(field,buf);
            if(code==SerializationEC::NONE)
                buf = buf.subspan(serial_size(field));
            return code;
        };
        (((result_code = deserialize_field(args))==SerializationEC::NONE) && ...);
        return result_code;
    }

    template<typename T>
    SerializationEC serialize_native(const T& val,std::vector<char>& buf) noexcept{
        return Serialize<false,T>{}(val,buf);
    }
    template<typename T>
    SerializationEC serialize_network(const T& val,std::vector<char>& buf) noexcept{
        return Serialize<true,T>{}(val,buf);
    }
    template<typename T>
    SerializationEC deserialize_native(T& to_deserialize,MultiBufferView& buf) noexcept{
        return Deserialize<false,T>{}(to_deserialize,buf);
    }
    template<typename T>
    SerializationEC deserialize_network(T& to_deserialize,MultiBufferView& buf) noexcept{
        return Deserialize<true,T>{}(to_deserialize,buf);
    }

    template<typename T,typename... ARGS>
    SerializationEC serialize_native(const T& val,std::vector<char>& buf,const ARGS&... args) noexcept{
        if constexpr (sizeof...(ARGS)>0)
            return serialize<false,std::decay_t<T>>(val,buf,args...);
        else return Serialize<false,std::decay_t<T>>{}(val,buf);
    }
    template<typename T,typename... ARGS>
    SerializationEC serialize_network(const T& val,std::vector<char>& buf,const ARGS&... args) noexcept{
        if constexpr (sizeof...(ARGS)>0)
            return serialize<true,std::decay_t<T>>(val,buf,args...);
        else return Serialize<true,std::decay_t<T>>{}(val,buf);
    }
    template<typename T,typename... ARGS>
    SerializationEC deserialize_native(const T& to_deserialize,MultiBufferView& buf,ARGS&... args) noexcept{
        if constexpr (sizeof...(ARGS)>0)
            return deserialize<false,std::decay_t<T>>(to_deserialize,buf,args...);
        else return Deserialize<false,std::decay_t<T>>{}(to_deserialize,buf);
    }
    template<typename T,typename... ARGS>
    SerializationEC deserialize_network(const T& to_deserialize,MultiBufferView& buf,ARGS&... args) noexcept{
        if constexpr (sizeof...(ARGS)>0)
            return deserialize<true,std::decay_t<T>>(to_deserialize,buf,args...);
        else return Deserialize<true,std::decay_t<T>>{}(to_deserialize,buf);
    }

    template<bool NETWORK,typename T>
    SerializationEC serialize_to_file(const T& val,std::ofstream& fstream) noexcept{
        SerializationEC err;
        std::vector<char> buf;
        buf.reserve(serial_size(val));
        if constexpr (!NETWORK){
            if(err = serialize_native(val,buf); err!=SerializationEC::NONE)
                return err;
        }
        else{
            if(err = serialize_network(val,buf); err!=SerializationEC::NONE)
                return err;
        }
        fstream.write(buf.data(),buf.size());
        if(fstream.fail())
            return SerializationEC::FILE_WRITING_ERROR;
        return SerializationEC::NONE;
    }
    template<bool NETWORK,typename T>
    SerializationEC deserialize_from_file(T& val,std::ifstream& fstream) noexcept{
        SerializationEC err;
        size_t sz = 0;
        {
            size_t cur = fstream.tellg();
            sz = fstream.seekg(0,std::ios::end).tellg()-cur;
            fstream.seekg(cur,std::ios::beg);
        }
        if(fstream.fail())
            return SerializationEC::FILE_READING_ERROR;
        else if(sz<min_serial_size(val))
            return SerializationEC::BUFFER_SIZE_LESSER;
        std::vector<char> buf;
        buf.resize(sz>max_serial_size(val)?max_serial_size(val):sz);
        fstream.read(buf.data(),
            sz>max_serial_size(val)?max_serial_size(val):sz);
        if(fstream.eof())
            return SerializationEC::UNEXPECTED_EOF;
        else if(fstream.fail())
            return SerializationEC::FILE_READING_ERROR;
        if constexpr(!NETWORK){
            err = deserialize_native(val,MultiBufferView&(buf));
            if(err!=SerializationEC::NONE)
                return err;
        }
        else{
            err = deserialize_network(val,MultiBufferView&(buf));
            if(err!=SerializationEC::NONE)
                return err;
        }
        //if read more than necessairy - return to the position after read value
        fstream.seekg(serial_size(val)-buf.size(),
            std::ios::cur);
        return SerializationEC::NONE;
    }

    template<bool NETWORK = false,typename... ARGS>
    requires (sizeof...(ARGS)>1)
    SerializationEC serialize_to_file(std::ofstream& fstream,const ARGS&... val) noexcept{
        SerializationEC err;
        std::vector<char> buf;
        buf.reserve(serial_size(val...));
        auto serialize_variadic = [&buf,&err](auto&& value) ->SerializationEC
        {
            if constexpr(!NETWORK){
                err = serialize_native(std::forward<decltype(value)>(value),buf);
                return err;
            }
            else {
                err = serialize_network(std::forward<decltype(value)>(value),buf);
                return err;
            }
        };
        ((serialize_variadic(val)==SerializationEC::NONE) && ...);
        if(err!=SerializationEC::NONE)
            return err;
        fstream.write(buf.data(),buf.size());
        if(fstream.fail())
            return SerializationEC::FILE_WRITING_ERROR;
        return err;
    }
    template<bool NETWORK = false,typename... ARGS>
    requires (sizeof...(ARGS)>1)
    SerializationEC deserialize_from_file(std::ifstream& fstream,ARGS&... val) noexcept{
        SerializationEC err;
        size_t sz = 0;
        {
            size_t cur = fstream.tellg();
            sz = fstream.seekg(0,std::ios::end).tellg()-cur;
            fstream.seekg(cur,std::ios::beg);
        }
        if(fstream.fail())
            return SerializationEC::FILE_READING_ERROR;
        else if(sz<(min_serial_size(val)+...))
            return SerializationEC::BUFFER_SIZE_LESSER;
        std::vector<char> buf;
        size_t to_reserve = 0;
        buf.resize(sz>max_serial_size(val...)?max_serial_size(val...):sz);
        fstream.read(buf.data(),
            sz>max_serial_size(val...)?max_serial_size(val...):sz);
        if(fstream.eof())
            return SerializationEC::UNEXPECTED_EOF;
        else if(fstream.fail())
            return SerializationEC::FILE_READING_ERROR;

        size_t offset = 0;
        auto deserialize_variadic = [&buf,&err,&offset](auto& value) ->SerializationEC
        {
            if constexpr(!NETWORK){
                err = deserialize_native(value,MultiBufferView&(buf).subspan(offset));
                offset+=serial_size(value);
                return err;
            }
            else {
                err = deserialize_network(value,MultiBufferView&(buf).subspan(offset));
                offset+=serial_size(value);
                return err;
            }
        };

        ((deserialize_variadic(val)==SerializationEC::NONE) && ...);
        if(err!=SerializationEC::NONE)
            return err;
        //if read more than necessairy - return to the position after read value
        fstream.seekg((serial_size(val)+...)-buf.size(),
            std::ios::cur);
        return err;
    }

    template<typename T, bool NETWORK_ORDER>
    SerializationEC deserialize(T& value,std::span<MultiBufferView&>& buffer) noexcept{
        size_t buffered = 0;
        int sections_buffered = 0;
        for(sections_buffered=0;i<buffer.size();++sections_buffered){
            buffered+=buffer[sections_buffered].size();
            if(buffered>=min_serial_size(value))
                break;
        }
        if(buffered<min_serial_size(value))
            return serialization::SerializationEC::BUFFER_SIZE_LESSER;
        if(sections_buffered>1){
            std::vector<const char> buf_loc;
            buf_loc.reserve(buffered);
            for(int i=0;i<sections_buffered;++i)
                buf_loc.append_range(buffer[i]);
            
        }
        else return deserialize(value,buffer);
    }
}

#include <list>
#include <unordered_map>
#include <unordered_set>
#include <set>
#include <map>
#include <deque>

static_assert(serialization::serialize_concept<true,bool>);
static_assert(serialization::serialize_concept<false,bool>);
static_assert(serialization::deserialize_concept<true,bool>);
static_assert(serialization::deserialize_concept<false,bool>);

static_assert(serialization::serialize_concept<true,std::chrono::system_clock::duration>);
static_assert(serialization::serialize_concept<false,std::chrono::system_clock::duration>);
static_assert(serialization::deserialize_concept<true,std::chrono::system_clock::duration>);
static_assert(serialization::deserialize_concept<false,std::chrono::system_clock::duration>);

static_assert(serialization::Min_serial_size<int>::value==sizeof(int));
static_assert(serialization::Min_serial_size<double>::value==sizeof(double));
static_assert(serialization::Min_serial_size<std::optional<int>>::value==sizeof(bool));
static_assert(serialization::Min_serial_size<std::unique_ptr<int>>::value==sizeof(bool));
static_assert(serialization::Min_serial_size<std::shared_ptr<int>>::value==sizeof(bool));
static_assert(serialization::Min_serial_size<std::vector<int>>::value==sizeof(size_t));
static_assert(serialization::Min_serial_size<std::list<int>>::value==sizeof(size_t));
static_assert(serialization::Min_serial_size<std::deque<int>>::value==sizeof(size_t));
static_assert(serialization::Min_serial_size<std::set<int>>::value==sizeof(size_t));
static_assert(serialization::Min_serial_size<std::unordered_set<int>>::value==sizeof(size_t));
static_assert(serialization::Min_serial_size<std::unordered_map<int,int>>::value==sizeof(size_t));
static_assert(serialization::Min_serial_size<std::map<int,int>>::value==sizeof(size_t));
static_assert(serialization::Min_serial_size<std::optional<std::map<int,int>>>::value==sizeof(bool));

static_assert(serialization::Max_serial_size<int>::value==sizeof(int));
static_assert(serialization::Max_serial_size<double>::value==sizeof(double));
static_assert(serialization::Max_serial_size<std::optional<int>>::value==sizeof(bool)+serialization::max_serial_size<int>());
static_assert(serialization::Max_serial_size<std::unique_ptr<int>>::value==sizeof(bool)+serialization::max_serial_size<int>());
static_assert(serialization::Max_serial_size<std::shared_ptr<int>>::value==sizeof(bool)+serialization::max_serial_size<int>());
static_assert(serialization::Max_serial_size<std::vector<int>>::value==std::numeric_limits<size_t>::max());
static_assert(serialization::Max_serial_size<std::list<int>>::value==std::numeric_limits<size_t>::max());
static_assert(serialization::Max_serial_size<std::deque<int>>::value==std::numeric_limits<size_t>::max());
static_assert(serialization::Max_serial_size<std::set<int>>::value==std::numeric_limits<size_t>::max());
static_assert(serialization::Max_serial_size<std::unordered_set<int>>::value==std::numeric_limits<size_t>::max());
static_assert(serialization::Max_serial_size<std::unordered_map<int,int>>::value==std::numeric_limits<size_t>::max());
static_assert(serialization::Max_serial_size<std::map<int,int>>::value==std::numeric_limits<size_t>::max());
static_assert(serialization::Max_serial_size<std::optional<std::map<int,int>>>::value==std::numeric_limits<size_t>::max());