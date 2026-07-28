#pragma once
#include <ranges>
#include <concepts>
#include <memory>
#include <chrono>

template<typename T>
concept numeric_types_concept = std::is_integral_v<T> || std::is_floating_point_v<T> || std::is_enum_v<T>;

template<typename T>
concept common_types_concept = numeric_types_concept<T> || std::is_class_v<T>;

template<typename T>
concept pair_concept = requires(const T& p) {
    { p.first } -> std::convertible_to<const typename T::first_type&>;
    { p.second } -> std::convertible_to<const typename T::second_type&>;
};

static_assert(pair_concept<std::pair<int,int>>);
/**
 * @details Serialization for weak pointer is not implemented due to uncertainty in assignment to shared pointer
 */
template<typename T>
concept smart_pointer_concept = 
requires(T ptr) {
    typename T::element_type;
    { ptr.get() } -> std::convertible_to<typename T::element_type*>;
    { bool(ptr) } -> std::convertible_to<bool>;
};
static_assert(smart_pointer_concept<std::shared_ptr<int>>);
static_assert(smart_pointer_concept<std::unique_ptr<int>>);

template<typename T>
concept weak_pointer_concept =
requires(T ptr) {
    {ptr.lock()}->std::same_as<std::shared_ptr<typename std::decay_t<T>::element_type>>;
    std::is_same_v<T, std::weak_ptr<typename T::element_type>>;
};
static_assert(weak_pointer_concept<std::weak_ptr<int>>);

template<typename T>
concept time_point_concept = requires (const T& time){
    time.time_since_epoch().count();
};
static_assert(time_point_concept<std::chrono::system_clock::time_point>);
template<typename T>
concept duration_concept = requires (const T& duration){
    duration.count();
};

static_assert(duration_concept<std::chrono::system_clock::duration>);

template<typename T>
concept AssociativeContainer = requires(const T& cont){
    typename std::decay_t<T>::key_type;
    typename std::decay_t<T>::mapped_type;
    requires std::ranges::range<std::decay_t<T>>;
};

template<typename T>
concept String  = requires{
    requires(
        std::ranges::range<std::decay_t<T>>&&
        std::is_same_v<std::ranges::range_value_t<
        std::decay_t<T>>,char>&&
        std::ranges::bidirectional_range<T>
    )||
    std::is_convertible_v<T,std::string_view>;
};

template<typename T>
concept RangeOfStrings  = requires{
    requires std::ranges::range<std::ranges::range_value_t<std::decay_t<T>>>;
    requires String<std::ranges::range_value_t<std::decay_t<T>>>;
};

template<typename T>
inline constexpr bool is_associative_container_v = AssociativeContainer<T>;

#include <variant>
template<typename T>
concept IsStdVariant = requires(const T& val){
    std::variant_size<std::decay_t<T>>::value;
};

// удобный alias
template<typename T>
inline constexpr bool is_std_variant_v = IsStdVariant<T>;

template<typename T>
concept IsTimePoint = requires(T d) {
    typename std::decay_t<T>::rep;
    typename std::decay_t<T>::period;
    typename std::decay_t<T>::clock;
    { d.time_since_epoch() } -> std::same_as<typename std::decay_t<T>::duration>;
    requires std::is_same_v<std::decay_t<T>, std::chrono::time_point<typename std::decay_t<T>::clock,typename std::decay_t<T>::duration>>;
};

template<typename T>
concept IsDuration = requires(T d) {
    typename std::decay_t<T>::rep;
    typename std::decay_t<T>::period;
    { d.count() } -> std::same_as<typename std::decay_t<T>::rep>;
    { d + d } -> std::same_as<std::decay_t<T>>;
    { d - d } -> std::same_as<std::decay_t<T>>;
    requires !IsTimePoint<std::decay_t<T>>;
};

template<typename T>
concept IsOptional = requires(T opt) {
    typename std::decay_t<T>::value_type;
    {std::declval<std::decay_t<T>>().has_value()}->std::same_as<bool>;
    std::is_same_v<std::optional<typename std::decay_t<T>::value_type>,std::decay_t<T>>;
};

template<typename T>
concept IsReferenceWrapper = 
    requires(T t) {
        typename T::type;
        requires std::same_as<std::remove_cvref_t<T>, 
                             std::reference_wrapper<typename T::type>>;
    };

static_assert(IsDuration<std::chrono::nanoseconds>);
static_assert(IsDuration<std::chrono::nanoseconds>);
static_assert(IsTimePoint<std::chrono::system_clock::time_point>);
static_assert(IsTimePoint<std::chrono::steady_clock::time_point>);
static_assert(IsTimePoint<std::chrono::file_clock::time_point>);
static_assert(IsTimePoint<std::chrono::tai_clock::time_point>);
static_assert(IsReferenceWrapper<std::reference_wrapper<int>>);