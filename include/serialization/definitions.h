#pragma once
#include <fstream>
#include <vector>
#include <type_traits>


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

    class StreamSerializer;

    template<bool NETWORK = false,typename T>
    SerializationEC serialize_to_file(const T& val,std::ofstream& fstream) noexcept;
    template<bool NETWORK = false,typename T>
    SerializationEC deserialize_from_file(T& val,std::ifstream& fstream) noexcept;

    template<typename T>
    SerializationEC serialize_native(const T& val,std::vector<char>& buf) noexcept;
    template<typename T>
    SerializationEC serialize_network(const T& val,std::vector<char>& buf) noexcept;
    template<typename T>
    SerializationEC deserialize_native(T& to_deserialize,StreamSerializer& buf) noexcept;
    template<typename T>
    SerializationEC deserialize_network(T& to_deserialize,StreamSerializer& buf) noexcept;

    template<typename T,typename... ARGS>
    SerializationEC serialize_native(const T& val,std::vector<char>& buf,const ARGS&... args) noexcept;
    template<typename T,typename... ARGS>
    SerializationEC serialize_network(const T& val,std::vector<char>& buf,const ARGS&... args) noexcept;
    template<typename T,typename... ARGS>
    SerializationEC deserialize_native(T& to_deserialize,StreamSerializer& buf,ARGS&... args) noexcept;
    template<typename T,typename... ARGS>
    SerializationEC deserialize_network(T& to_deserialize,StreamSerializer& buf,ARGS&... args) noexcept;

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
    SerializationEC deserialize(T& val,StreamSerializer& buf) noexcept;

    template<bool NETWORK_ORDER,typename T,typename... ARGS>
    requires (sizeof...(ARGS)>0)
    SerializationEC serialize(const T& val,std::vector<char>& buf,const ARGS&... args) noexcept;
    template<bool NETWORK_ORDER,typename T,typename... ARGS>
    requires (sizeof...(ARGS)>0)
    SerializationEC deserialize(const T& to_deserialize,StreamSerializer& buf,ARGS&... args) noexcept;

    template<bool NETWORK_ORDER,typename T>
    concept serialize_concept = 
    requires(const T& val, std::vector<char>& buf){
        { Serialize<NETWORK_ORDER,T>{}(val, buf) } -> std::same_as<SerializationEC>;
    };

    template<bool NETWORK_ORDER,typename T>
    concept deserialize_concept = 
    requires(T& val,StreamSerializer& buf){
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

}