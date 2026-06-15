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
#include "serialization/definitions.h"
#include "serialization/multi_buffer.h"
#include <unordered_map>


namespace serialization{
    class StreamSerializer{
        MultiBufferView mbv_;
        std::unordered_map<size_t,std::vector<std::array<std::byte,16>>> registered_; //registered values
        size_t begin{0};
        size_t end{0};
        size_t iterator{0};
        size_t level_=0;
        void flush_registered() noexcept{
            registered_.erase(registered_.find(level_),registered_.end());
        }
        void reset_iterator() noexcept{
            iterator = begin;
        }
        public:
        StreamSerializer() = default;
        void push_view(auto&& buffer) noexcept
        requires (std::ranges::random_access_range<std::decay_t<decltype(buffer)>> &&
        std::ranges::contiguous_range<std::decay_t<decltype(buffer)>>)
        {
            if(std::size(buffer)!=0){
                mbv_.push(std::forward<decltype(buffer)>(buffer));
                reset_iterator();
            }
        }
        size_t position() const noexcept{
            return mbv_.position();
        }
        template<typename T>
        bool trivial_deserialized(T& value) const noexcept{
            if constexpr(min_serial_size<T>()==max_serial_size<T>()){
                return end-iterator>=serial_size(value);
            }
            else return false;
        }

        size_t flush() noexcept{
            size_t sz = mbv_.flush_deserialized();
            return sz;
        }
        void reset_all() noexcept{
            mbv_ = MultiBufferView();
            registered_.clear(); //container sizes
            begin=0;
            end=0;
        }
        template<numeric_types_concept T>
        bool try_advance(T& value) noexcept{
            if(trivial_deserialized(value)){
                iterator+=serial_size(value);
                return true;
            }
            else return false;
        }

        template<numeric_types_concept T>
        void register_value(const T& value) noexcept{
            auto& last = registered_[level_].emplace_back();
            std::memcpy(last.data(),&value,sizeof(value));
        }

        template<numeric_types_concept T>
        void get_value_at(T& value,size_t sz) noexcept{
            std::memcpy(&value,
                registered_.at(level_).at(sz).data(),sizeof(value));
        }

        template<numeric_types_concept T>
        void update_value_at(const T& value,size_t sz) noexcept{
            std::memcpy(registered_.at(level_).at(sz).data(),
                &value,
                sizeof(value));
        }
        size_t registered_size() const noexcept{
            if(registered_.contains(level_))
                return registered_.at(level_).size();
            else return 0;
        }

        template<bool NETWORK_ORDER,numeric_types_concept T>
        SerializationEC deserialize(T& value) noexcept{
            if(mbv_.read_trivial<T,NETWORK_ORDER>(value)){
                    size_t sz = serialization::serial_size(value);
                    end += sz;
                    iterator += sz;
                    return serialization::SerializationEC::NONE;
            }
            else return serialization::SerializationEC::BUFFER_SIZE_LESSER;
        }

        template<bool NETWORK_ORDER,typename T>
        SerializationEC deserialize(T& value) noexcept{
            ++level_;
            SerializationEC result;
            result = Deserialize<NETWORK_ORDER,T>{}(value,*this);                                       
            if(result==SerializationEC::NONE)
            {
                if(level_==1){
                    //assert(mbv_.advance(iterator-begin));
                    mbv_.flush_deserialized();
                    begin=end;
                    iterator=begin;
                    registered_.clear();
                    flush_registered();
                }
                else registered_.erase(level_);
            }
            if(result!=SerializationEC::NONE)
                reset_iterator();
            --level_;
            return result;
        }
        template<bool NETWORK_ORDER,typename... ARGS>
        requires (sizeof...(ARGS)>1)
        SerializationEC deserialize(ARGS&... args) noexcept{
            SerializationEC result_code;
            auto deserialize_field = [&](auto& field) mutable noexcept->SerializationEC
            {
                using type = std::decay_t<decltype(field)>;
                SerializationEC code = this->deserialize<NETWORK_ORDER>(field);
                result_code = code;
                return code;
            };
            ((deserialize_field(args)==SerializationEC::NONE) && ...);
            return result_code;
        }
    };
}

namespace serialization{
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
    static_assert(std::is_trivially_copyable_v<bool>);
    template<bool NETWORK_ORDER,typename T>
    struct Deserialize{
        /// @brief Deserialize data from buffer to specified type
        /// @tparam T Supported types: integral, floating-point, enum (including scoped enum)
        /// @tparam NETWORK_ORDER If true, converts from network (big-endian) byte order
        /// @param buf Input data buffer (read-only)
        /// @return std::expected<T, SerializationEC> - value or error code
        /// @note Supports both runtime and constexpr contexts
        /// @warning Buffer must be properly aligned for type T
        SerializationEC operator()(T& to_deserialize,StreamSerializer& buf) const noexcept{
            if constexpr (numeric_types_concept<T>)
                return buf.deserialize<NETWORK_ORDER>(to_deserialize);
            else if constexpr (time_point_concept<T>){
                int64_t IntVal = 0;
                if(auto ser_res = deserialize<NETWORK_ORDER>(IntVal,buf);ser_res == SerializationEC::NONE){
                    to_deserialize = T(std::chrono::duration_cast<typename T::duration>(std::chrono::nanoseconds(IntVal)));
                    return ser_res;
                }
                else{
                    return ser_res;
                }
            }
            else if constexpr (duration_concept<T>){
                int64_t IntVal = 0;
                if(auto ser_res = buf.deserialize<NETWORK_ORDER>(IntVal);
                        ser_res == SerializationEC::NONE)
                {
                    to_deserialize = std::chrono::duration_cast<T>(std::chrono::nanoseconds(IntVal));
                    return ser_res;
                }
                else{
                    return ser_res;
                }
            }
            else if constexpr (smart_pointer_concept<T>){
                static_assert(std::is_default_constructible_v<typename T::element_type>,
                        "smart pointer deserializable elements must be default constructible");
                bool has_value = false;
                if(buf.try_advance(has_value)){
                    if(to_deserialize.get())
                        return buf.deserialize<NETWORK_ORDER>(*to_deserialize);
                    else return SerializationEC::NONE;
                }
                else{
                    if(auto ser_res = buf.deserialize<NETWORK_ORDER>(has_value);
                            ser_res == SerializationEC::NONE)
                    {
                        if(has_value){
                            if constexpr (requires{std::is_same_v<T,std::unique_ptr<typename T::element_type,
                                                                typename T::deleter_type>>;})
                                to_deserialize = std::make_unique<typename T::element_type>();
                            else
                                to_deserialize = std::make_shared<typename T::element_type>();
                            if(auto ser_res = deserialize<NETWORK_ORDER>(*to_deserialize,buf);
                                    ser_res == SerializationEC::NONE)
                            {
                                return SerializationEC::NONE;
                            }
                            else{
                                return ser_res;
                            }
                        }
                        else {
                            to_deserialize.reset();
                            return SerializationEC::NONE;
                        }
                    }
                    else return ser_res;
                }
            }
            else if constexpr(std::is_empty_v<T>)
                return SerializationEC::NONE;
            else if constexpr(pair_concept<T>){
                return deserialize<NETWORK_ORDER>(to_deserialize,
                        buf,to_deserialize.first,to_deserialize.second);
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

    template<bool NETWORK_ORDER,typename T,size_t SZ>
    struct Serialize<NETWORK_ORDER,std::array<T,SZ>>{
        SerializationEC operator()(const std::array<T,SZ>& val,std::vector<char>& buf) const noexcept{
            SerializationEC err = serialize<NETWORK_ORDER>(val.size(),buf);
            if constexpr(sizeof(T)==1){
                size_t old_sz = buf.size();
                buf.resize(buf.size()+SZ);
                std::memcpy(buf.begin()+old_sz,val.data(),SZ);
                return SerializationEC::NONE;
            }
            else if constexpr(sizeof(T)==0)
                return SerializationEC::NONE;

            for(const auto& item:val){
                err = serialize<NETWORK_ORDER>(item,buf);
                if(err==SerializationEC::NONE)
                    continue;
                else
                    return err;
            }
            return SerializationEC::NONE;
        }
    };
    template<bool NETWORK_ORDER,typename T,size_t SZ>
    struct Deserialize<NETWORK_ORDER,std::array<T,SZ>>{
        /// @brief Deserialize data from buffer to specified type
        /// @tparam T Supported types: integral, floating-point, enum (including scoped enum)
        /// @tparam NETWORK_ORDER If true, converts from network (big-endian) byte order
        /// @param buf Input data buffer (read-only)
        /// @return std::expected<T, SerializationEC> - value or error code
        /// @note Supports both runtime and constexpr contexts
        /// @warning Buffer must be properly aligned for type T
        SerializationEC operator()(std::array<T,SZ>& to_deserialize,StreamSerializer& buf) const noexcept{
            size_t remain = SZ;
            if(buf.registered_size()>0)
                buf.get_value_at(remain,0);
            for (size_t i = SZ-remain; i < remain; ++i){
                SerializationEC code = buf.deserialize<NETWORK_ORDER>(to_deserialize[i]);
                buf.update_value_at(remain-1,0);
                if (code != SerializationEC::NONE){
                    return code;
                }
            }
            return SerializationEC::NONE;
        }
    };

    template<typename T,size_t SZ>
    struct Serial_size<std::array<T,SZ>>{
        size_t operator()(const std::array<T,SZ>& val) const noexcept{
            size_t result = 0;
            if constexpr (min_serial_size<T>()==max_serial_size<T>())
                return SZ*min_serial_size<T>();
            else{
                for(size_t i=0;i<SZ;++i)
                    result+=serial_size(val[i]);
                return result;
            }
        }
    };

    template<typename T, size_t SZ>
    struct Min_serial_size<std::array<T,SZ>> {
        static constexpr size_t value = []() -> size_t {
            constexpr size_t elem_min = Min_serial_size<T>::value;
            // Проверка переполнения
            if constexpr (elem_min != 0) {
                if (elem_min > std::numeric_limits<size_t>::max() / SZ)
                    return std::numeric_limits<size_t>::max();
            }
            return elem_min * SZ;
        }();
    };

    template<typename T, size_t SZ>
    struct Max_serial_size<std::array<T,SZ>> {
        static constexpr size_t value = []() -> size_t {
            constexpr size_t elem_max = Max_serial_size<T>::value;
            if constexpr (elem_max != 0) {
                if (elem_max > std::numeric_limits<size_t>::max() / SZ)
                    return std::numeric_limits<size_t>::max();
            }
            return elem_max * SZ;
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
    requires (sizeof...(ARGS)>0)
    size_t min_serial_size(const ARGS&... val) noexcept{
        return min_serial_size<ARGS...>();
    }
    template<typename... ARGS>
    requires (sizeof...(ARGS)>0)
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
        SerializationEC operator()(std::optional<T>& to_deserialize,StreamSerializer& buf) const noexcept
        {
            bool has_value = false;
            if(buf.try_advance(has_value)){
                if(to_deserialize.has_value())
                    return buf.deserialize<NETWORK_ORDER>(to_deserialize.value());
                else return SerializationEC::NONE;
            }
            else{
                SerializationEC code = buf.deserialize<NETWORK_ORDER>(has_value);
                if(code!=SerializationEC::NONE)
                    return code;
                if(!has_value)
                    return SerializationEC::NONE;
                to_deserialize.emplace();
                return buf.deserialize<NETWORK_ORDER>(to_deserialize.value());
            }
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
        SerializationEC operator()(T& to_deserialize,StreamSerializer& buf) const noexcept{
            static_assert(deserialize_concept<NETWORK_ORDER,std::ranges::range_value_t<T>>);
            size_t range_sz = 0;
            SerializationEC code;
            if(buf.try_advance(range_sz)){
                buf.get_value_at(range_sz,0);
            }
            else{
                to_deserialize.clear();
                code = buf.deserialize<NETWORK_ORDER>(range_sz);
                if(code!=SerializationEC::NONE)
                    return code;
                buf.register_value(range_sz);
            }
            size_t sz = std::size(to_deserialize);
            for (size_t i = 0; i < range_sz-sz; ++i){
                if constexpr(is_associative_container_v<T>){
                    std::pair<typename T::key_type,typename T::mapped_type> item{};
                    code = buf.deserialize<NETWORK_ORDER>(item);
                    if (code != SerializationEC::NONE)
                        return code;
                    to_deserialize.insert(std::move(item));
                }
                else {
                    std::ranges::range_value_t<T> item{};
                    code = buf.deserialize<NETWORK_ORDER>(item);
                    if (code != SerializationEC::NONE){
                        return code;
                    }
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
        SerializationEC operator()(type& val, StreamSerializer& buf) const noexcept{
            using factory = ::VariantFactory<type>;
            size_t index = std::numeric_limits<size_t>::max();
            if(!buf.try_advance(index)){
                if(SerializationEC err = buf.deserialize<NETWORK_ORDER>(index);
                    err!=SerializationEC::NONE)
                    return err;
                if(!factory::emplace(val,index))
                    return SerializationEC::UNMATCHED_TYPE;
            }
            auto deserializer = [&buf](auto& item)
            {   
                return buf.deserialize<NETWORK_ORDER>(item);
            };

            if(auto ser_res = std::visit(deserializer,val);
                ser_res!=SerializationEC::NONE){
                return ser_res;
            }
            else{
                return ser_res;
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
    SerializationEC deserialize(T& val,StreamSerializer& buf) noexcept{
        return buf.deserialize<NETWORK_ORDER>(val);
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
    SerializationEC deserialize(const T& val,StreamSerializer& buf, ARGS&... args) noexcept{
        static_assert(min_serial_size<T>()==min_serial_size<ARGS...>(),"Expected equal minimal serial size of object and its fields' summary minimal serial size");
        static_assert(min_serial_size<T>()==min_serial_size<ARGS...>(),"Expected equal maximal serial size of object and its fields' summary maximal serial size");
        return buf.deserialize<NETWORK_ORDER>(args...);
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
    SerializationEC deserialize_native(T& to_deserialize,StreamSerializer& buf) noexcept{
        return buf.deserialize<false>(to_deserialize);
    }
    template<typename T>
    SerializationEC deserialize_network(T& to_deserialize,StreamSerializer& buf) noexcept{
        return buf.deserialize<true>(to_deserialize);
    }

    template<typename T,typename... ARGS>
    requires (sizeof...(ARGS)>0)
    SerializationEC serialize_native(const T& val,std::vector<char>& buf,const ARGS&... args) noexcept{
        if constexpr (sizeof...(ARGS)>1)
            return serialize<false,std::decay_t<T>>(val,buf,args...);
        else return Serialize<false,std::decay_t<T>>{}(val,buf);
    }
    template<typename T,typename... ARGS>
    requires (sizeof...(ARGS)>0)
    SerializationEC serialize_network(const T& val,std::vector<char>& buf,const ARGS&... args) noexcept{
        if constexpr (sizeof...(ARGS)>1)
            return serialize<true,std::decay_t<T>>(val,buf,args...);
        else return Serialize<true,std::decay_t<T>>{}(val,buf);
    }
    template<typename T,typename... ARGS>
    requires (sizeof...(ARGS)>0)
    SerializationEC deserialize_native(const T& to_deserialize,StreamSerializer& buf,ARGS&... args) noexcept{
        if constexpr (sizeof...(ARGS)>1)
            return deserialize<false,std::decay_t<T>>(to_deserialize,buf,args...);
        else return deserialize<false>(to_deserialize,buf);
    }
    template<typename T,typename... ARGS>
    requires (sizeof...(ARGS)>0)
    SerializationEC deserialize_network(const T& to_deserialize,StreamSerializer& buf,ARGS&... args) noexcept{
        if constexpr (sizeof...(ARGS)>1)
            return deserialize<true,std::decay_t<T>>(to_deserialize,buf,args...);
        else return deserialize<true>(to_deserialize,buf);
    }

    template<bool NETWORK_ORDER,typename T>
    SerializationEC serialize_to_file(const T& val,std::ofstream& fstream) noexcept{
        SerializationEC err;
        std::vector<char> buf;
        buf.reserve(serial_size(val));
        if constexpr (!NETWORK_ORDER){
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
    template<bool NETWORK_ORDER,typename T>
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
        StreamSerializer mbv;
        mbv.push_view(buf);
        if constexpr(!NETWORK_ORDER){
            err = deserialize_native(val,mbv);
            if(err!=SerializationEC::NONE)
                return err;
        }
        else{
            err = deserialize_network(val,mbv);
            if(err!=SerializationEC::NONE)
                return err;
        }
        //if read more than necessairy - return to the position after read value
        fstream.seekg(serial_size(val)-buf.size(),
            std::ios::cur);
        return SerializationEC::NONE;
    }

    template<bool NETWORK_ORDER = false,typename... ARGS>
    requires (sizeof...(ARGS)>1)
    SerializationEC serialize_to_file(std::ofstream& fstream,const ARGS&... val) noexcept{
        SerializationEC err;
        std::vector<char> buf;
        buf.reserve(serial_size(val...));
        auto serialize_variadic = [&buf,&err](auto&& value) ->SerializationEC
        {
            if constexpr(!NETWORK_ORDER){
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
    template<bool NETWORK_ORDER = false,typename... ARGS>
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
        StreamSerializer multi_buffer;
        multi_buffer.push_view(buf);
        if(fstream.eof())
            return SerializationEC::UNEXPECTED_EOF;
        else if(fstream.fail())
            return SerializationEC::FILE_READING_ERROR;

        size_t offset = 0;
        auto deserialize_variadic = [&multi_buffer,&err,&offset](auto& value) ->SerializationEC
        {
            if constexpr(!NETWORK_ORDER){
                err = deserialize_native(value,multi_buffer);
                offset+=serial_size(value);
                return err;
            }
            else {
                err = deserialize_network(value,multi_buffer);
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