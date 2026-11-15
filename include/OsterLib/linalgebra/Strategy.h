#pragma once
#include "OsterLib/linalgebra/definitions.h"


namespace linalgebra{
    struct Strategy{};

    struct Sum:Strategy{
        template<bool V1,bool V2,typename VAL1,typename VAL2,
            std::size_t... RANKS>
        auto operator()(const LinearObject<V1,VAL1,RANKS...>& lo1,const LinearObject<V2,VAL2,RANKS...>& lo2) const{
            using result_t = LinearObject<false,std::common_type_t<VAL1,VAL2>,RANKS...>;
            result_t result = {};
            for(std::size_t i=0;i<LinearObject<V1,VAL1,RANKS...>::total();++i)
                result.data()[i]=lo1.data()[i]+lo2.data()[i];
            return result;
        }
    };

    struct Sub:Strategy{
        template<bool V1,bool V2,typename VAL1,typename VAL2,
            std::size_t... RANKS>
        auto operator()(const LinearObject<V1,VAL1,RANKS...>& lo1,const LinearObject<V2,VAL2,RANKS...>& lo2) const{
            using result_t = LinearObject<false,std::common_type_t<VAL1,VAL2>,RANKS...>;
            result_t result = {};
            for(std::size_t i=0;i<LinearObject<V1,VAL1,RANKS...>::total();++i)
                result.data()[i]=lo1.data()[i]-lo2.data()[i];
            return result;
        }
    };

    struct ElementMultiplies:Strategy{
        template<bool V1,bool V2,typename VAL1,typename VAL2,
            std::size_t... RANKS>
        auto operator()(const LinearObject<V1,VAL1,RANKS...>& lo1,const LinearObject<V2,VAL2,RANKS...>& lo2) const{
            using result_t = LinearObject<false,std::common_type_t<VAL1,VAL2>,RANKS...>;
            result_t result = {};
            for(std::size_t i=0;i<LinearObject<V1,VAL1,RANKS...>::total();++i)
                result.data()[i]=lo1.data()[i]*lo2.data()[i];
            return result;
        }
    };
    struct OuterProduct:Strategy{
        template<bool V1,bool V2,typename VAL1,typename VAL2,
            std::size_t... RANKS1,std::size_t... RANKS2>
        auto operator()(const LinearObject<V1,VAL1,RANKS1...>& lo1,const LinearObject<V2,VAL2,RANKS2...>& lo2) const{
            using result_t = LinearObject<false,std::common_type_t<VAL1,VAL2>,RANKS1...,RANKS2...>;
            constexpr std::size_t total_1 = (RANKS1 * ... * 1);
            constexpr std::size_t total_2 = (RANKS2 * ... * 1);
            constexpr std::size_t total_R = total_1 * total_2;
            result_t result{};
            for(std::size_t i=0;i<total_R;++i)
                result.data()[i] =
                    lo1.data()[i / total_2]
                    * lo2.data()[i % total_2];
            return result;
        }
    };
    
    template<std::size_t... IDS>
    struct Contraction:Strategy{
        template<bool VIEW,typename VALUE_T,std::size_t... RANKS>
        FromIntegerSequence::LinearObject<false, VALUE_T, osterlib::ExcludedIdx<
                std::integer_sequence<std::size_t, RANKS...>,
                std::integer_sequence<std::size_t, IDS...>>> operator()(const LinearObject<VIEW,VALUE_T,RANKS...>& lo) const{
            static_assert(sizeof...(IDS) > 0);
            using matched = osterlib::ExcludedIdx<
                std::integer_sequence<std::size_t, RANKS...>,
                std::integer_sequence<std::size_t, IDS...>>;
            using result_t = FromIntegerSequence::LinearObject<false, VALUE_T, matched>;
            constexpr std::size_t p = sizeof...(RANKS);
            constexpr std::size_t k = sizeof...(IDS);
            constexpr std::array<std::size_t, p> ranks{RANKS...};
            constexpr std::array<std::size_t, k> ids{IDS...};
            constexpr std::array<std::size_t, p> stride = [&ranks] {
                std::array<std::size_t, p> s{};
                s[p-1] = 1;
                for (std::size_t i = p-1; i > 0; --i) s[i-1] = s[i] * ranks[i];
                return s;
            }();

            constexpr std::size_t n_free = p - k;
            constexpr std::array<std::size_t, n_free> free_axes = [&ids] {
                std::array<std::size_t, n_free> f{};
                std::size_t j = 0;
                for (std::size_t i = 0; i < p; ++i) {
                    bool contr = false;
                    for (auto id : ids) if (id == i) { contr = true; break; }
                    if (!contr) f[j++] = i;
                }
                return f;
            }();
            constexpr std::size_t total_result = osterlib::Product<matched>::value;
            constexpr std::size_t total_contr  = lo.total() / total_result;
            result_t result{};
            std::array<std::size_t, n_free> free_idx{};
            std::size_t base = 0;

            for (std::size_t r = 0; r < total_result; ++r) {
                std::array<std::size_t, k> contr_idx{};
                std::size_t inner = 0;
                VALUE_T sum{};
                for (std::size_t c = 0; c < total_contr; ++c) {
                    sum += lo.data()[base + inner];
                    for (std::size_t m = k; m-- > 0; ) {
                        ++contr_idx[m];
                        inner += stride[ids[m]];
                        if (contr_idx[m] < ranks[ids[m]]) break;
                        contr_idx[m] = 0;
                        inner -= ranks[ids[m]] * stride[ids[m]];
                    }
                }
                result.data()[r] = sum;

                if (r + 1 < total_result) {
                    for (std::size_t m = n_free; m-- > 0; ) {
                        ++free_idx[m];
                        base += stride[free_axes[m]];
                        if (free_idx[m] < ranks[free_axes[m]]) break;
                        free_idx[m] = 0;
                        base -= ranks[free_axes[m]] * stride[free_axes[m]];
                    }
                }
            }
            return result;
        }
    };
    template<std::size_t... IDS>
    struct CrossMultiplies:Strategy{
        template<bool V1,bool V2,typename VAL1,typename VAL2,
            std::size_t... RANKS1,std::size_t... RANKS2>
        std::common_type_t<VAL1,VAL2> operator()(const LinearObject<V1,VAL1,RANKS1...>& lo1,const LinearObject<V2,VAL2,RANKS2...>& lo2) const{
            return Contraction<IDS...>()(OuterProduct()(lo1,lo2));
        }
    };
    struct InnerProduct:Strategy{
        template<bool V1,bool V2,typename VAL1,typename VAL2,
            std::size_t... RANKS>
        std::common_type_t<VAL1,VAL2> operator()(const LinearObject<V1,VAL1,RANKS...>& lo1,const LinearObject<V2,VAL2,RANKS...>& lo2) const{
            using result_t = std::common_type_t<VAL1,VAL2>;
            result_t result = {};
            for(std::size_t i=0;i<LinearObject<V1,VAL1,RANKS...>::total();++i)
                result+=lo1.data()[i]*lo2.data()[i];
            return result;
        }
    };
    
    struct Transpose:Strategy{
        template<bool VIEW,typename VALUE_T,std::size_t... RANKS>
        FromIntegerSequence::LinearObject<false,VALUE_T,osterlib::ReverseSequence_t<RANKS...>> 
            operator()(const LinearObject<VIEW,VALUE_T,RANKS...>& lo) const
        {
            using result_t = FromIntegerSequence::LinearObject<false, VALUE_T,
                osterlib::ReverseSequence_t<RANKS...>>;

            constexpr std::size_t p = sizeof...(RANKS);
            constexpr std::size_t total = (RANKS * ... * 1);

            result_t result{};
            if constexpr (p == 0) {
                result.data()[0] = lo.data()[0];
                return result;
            } else {
                constexpr std::array<std::size_t, p> src_ranks{RANKS...};
                constexpr auto dst_stride = [&src_ranks] {
                    std::array<std::size_t, p> s{};
                    s[p-1] = 1;
                    for (std::size_t k = p-1; k > 0; --k)
                        s[k-1] = s[k] * src_ranks[p-1-k];
                    return s;
                }();
                constexpr auto src_to_dst = [&dst_stride] {
                    std::array<std::size_t, p> a{};
                    for (std::size_t m = 0; m < p; ++m)
                        a[m] = dst_stride[p-1-m];
                    return a;
                }();

                std::array<std::size_t, p> src_idx{};
                std::size_t dst_flat = 0;

                for (std::size_t i = 0; i < total; ++i) {
                    result.data()[dst_flat] = lo.data()[i];
                    for (std::size_t m = p; m-- > 0; ) {
                        ++src_idx[m];
                        dst_flat += src_to_dst[m];
                        if (src_idx[m] < src_ranks[m]) break;
                        src_idx[m] = 0;
                        dst_flat -= src_ranks[m] * src_to_dst[m];
                    }
                }
            }
            return result;
        }
    };
}