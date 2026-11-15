#pragma once
#include <cstdint>
#include <cstddef>
#include <type_traits>

namespace osterlib{
    template<uint64_t UINT1,uint64_t UINT2>
    struct Max{
        static constexpr uint64_t value = (UINT1<UINT2)?UINT2:UINT1;
    };

    template<uint64_t UINT1,uint64_t UINT2>
    struct Min{
        static constexpr uint64_t value = (UINT1>UINT2)?UINT2:UINT1;
    };

    template<typename S1, typename S2>
    struct PickImpl : std::false_type {};

    template<std::size_t... I, std::size_t... M>
    struct PickImpl<std::integer_sequence<std::size_t, I...>,
                    std::integer_sequence<std::size_t, M...>>
    {
        static constexpr std::size_t arr[] = { I... };
        using type = std::integer_sequence<std::size_t, arr[M]...>;
    };

    template<typename S1, typename S2>
    using MatchedIdx = typename PickImpl<S1, S2>::type;

    static_assert(std::is_same_v<MatchedIdx<std::integer_sequence<std::size_t,1,2,3>,std::integer_sequence<std::size_t,0,2>>,
                    std::integer_sequence<std::size_t,1,3>>);

    template<typename S1, typename S2>
    struct ExcludeImpl;

    template<std::size_t... I, std::size_t... M>
    struct ExcludeImpl<std::integer_sequence<std::size_t, I...>,
                    std::integer_sequence<std::size_t, M...>>
    {
        static constexpr std::size_t N = sizeof...(I);
        static constexpr std::array<std::size_t, N> arr{I...};

        static constexpr bool excluded(std::size_t pos) noexcept {
            return ((M == pos) || ...);
        }

        static constexpr std::size_t count = [] {
            std::size_t c = 0;
            for (std::size_t k = 0; k < N; ++k)
                if (!excluded(k)) ++c;
            return c;
        }();

        static constexpr std::array<std::size_t, count> kept = [] {
            std::array<std::size_t, count> out{};
            std::size_t j = 0;
            for (std::size_t k = 0; k < N; ++k)
                if (!excluded(k)) out[j++] = arr[k];
            return out;
        }();

        template<std::size_t... Ks>
        static constexpr auto make(std::index_sequence<Ks...>) {
            return std::integer_sequence<std::size_t, kept[Ks]...>{};
        }

        using type = decltype(make(std::make_index_sequence<count>{}));
    };

    template<std::size_t... IDS>
    struct ReverseSequence {
        static constexpr std::size_t N = sizeof...(IDS);

        template<std::size_t... Is>
        static constexpr auto make(std::index_sequence<Is...>) {
            constexpr std::array<std::size_t, N> arr{IDS...};
            return std::integer_sequence<std::size_t, arr[N - 1 - Is]...>{};
        }

        using type = decltype(make(std::make_index_sequence<N>{}));
    };

    template<std::size_t... IDS>
    using ReverseSequence_t = typename ReverseSequence<IDS...>::type;

    template<typename S1, typename S2>
    using ExcludedIdx = typename ExcludeImpl<S1, S2>::type;

    template<typename VALS>
    struct Sum;
    
    template<std::size_t... VALS>
    struct Sum<std::integer_sequence<std::size_t, VALS...>>
        : std::integral_constant<std::size_t, (std::size_t{0} + ... + VALS)> {};

    template<typename VALS>
    struct Product;

    template<std::size_t... VALS>
    struct Product<std::integer_sequence<std::size_t, VALS...>>
        : std::integral_constant<std::size_t, (std::size_t{1} * ... * VALS)> {};
}