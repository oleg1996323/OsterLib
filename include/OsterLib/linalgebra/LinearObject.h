#pragma once
#include "OsterLib/linalgebra/definitions.h"
#include "OsterLib/linalgebra/RankDissolve.h"
#include "OsterLib/traits.h"

namespace linalgebra{
    template<typename VALUE_T,std::size_t SKIP,std::size_t... RANKS>
    using DropRanks = typename __detail__::RankDropper<VALUE_T,SKIP,RANKS...>::type;

    template<bool VIEW,typename VALUE_T,std::size_t... RANKS>
    class LinearObject{
        static_assert(((RANKS > 0) && ...), "dimensions must be positive");
        protected:
        static constexpr std::size_t total_ = (RANKS * ... * 1);
        static constexpr std::size_t order_ = sizeof...(RANKS);
        static constexpr std::array<std::size_t, order_> ranks_{RANKS...};
        std::conditional_t<VIEW,VALUE_T*,std::array<VALUE_T, total_>> data_{};
        private:
        template<std::size_t... IDS>
        static constexpr std::size_t compute_offset() noexcept {
            constexpr std::array<std::size_t, sizeof...(IDS)> ids{IDS...};
            std::size_t idx = 0;
            for (std::size_t k = 0; k < order_; ++k) {
                const std::size_t v = (k < sizeof...(IDS)) ? ids[k] : 0;
                idx = idx * ranks_[k] + v;
            }
            return idx;
        }
        template<std::size_t... IDS>
        static constexpr bool bounds_ok() noexcept {
            constexpr std::array<std::size_t, sizeof...(IDS)> ids{IDS...};
            const std::size_t n = sizeof...(IDS) < order_ ? sizeof...(IDS) : order_;
            for (std::size_t k = 0; k < n; ++k)
                if (ids[k] >= ranks_[k]) return false;
            return true;
        }
        template<bool, typename, std::size_t...> friend class LinearObject;
        public:
        static constexpr std::array<std::size_t, order_>& ranks() noexcept{
            return ranks_;
        }
        static constexpr std::size_t total() noexcept{
            return total_;
        }
        using element_type = VALUE_T;

        std::decay_t<decltype(data_)>& data() noexcept{
            return data_;
        }

        const std::decay_t<decltype(data_)>& data() const noexcept{
            return data_;
        }
        constexpr LinearObject(){
            if constexpr(VIEW)
                data_ = nullptr;
        }
        constexpr LinearObject(const LinearObject<true,VALUE_T,RANKS...>& other){
            if constexpr(VIEW)
                data_ = other.data_;
            else std::copy_n(other.data_,total_,data_.begin());
        }
        constexpr LinearObject(const LinearObject<false,VALUE_T,RANKS...>& other){
            if constexpr(VIEW)
                *this = other.take_view();
            else std::copy_n(other.data_.begin(),total_,data_.begin());
        }
        template<std::size_t N = sizeof...(RANKS), std::enable_if_t<N == 0 && !VIEW, int> = 0>
        constexpr explicit LinearObject(VALUE_T val) : data_{val} {}
        template<bool B = VIEW,std::enable_if_t<!B, int> = 0>
        constexpr LinearObject(LinearObject<false,VALUE_T,RANKS...>&& other):
            data_(other.data_){}
        LinearObject<true,VALUE_T,RANKS...> take_view() noexcept{
            LinearObject<true,VALUE_T,RANKS...> view;
            if constexpr(!VIEW)
                view.data_ = data_.data();
            else view.data_ = data_;
            return view;
        }
        LinearObject<true,const VALUE_T,RANKS...> take_view() const noexcept{
            LinearObject<true,const VALUE_T,RANKS...> view;
            if constexpr(!VIEW)
                view.data_ = data_.data();
            else view.data_ = data_;
            return view;
        }
        constexpr LinearObject<false,VALUE_T>& operator=(VALUE_T val){
            data_[0] = val;
            return *this;
        }
        template<bool OTHER_VIEW, std::enable_if_t<VIEW == OTHER_VIEW, int> = 0>
        constexpr LinearObject& operator=(LinearObject<OTHER_VIEW, VALUE_T, RANKS...>&& other) {
            if (this != &other) {
                if constexpr (!VIEW) 
                    data_ = std::move(other.data_);
                else data_ = other.data_;
            }
            return *this;
        }
        template<bool OTHER_VIEW>
        constexpr LinearObject& operator=(const LinearObject<OTHER_VIEW,VALUE_T,RANKS...>& other){
            if(this!=&other){
                if constexpr(VIEW)
                    if constexpr(OTHER_VIEW)
                        data_ = other.data_;
                    else static_assert(!VIEW,
                            "Unable assign by copy from constant object");
                else{
                    if constexpr(OTHER_VIEW){
                        if(other.data_!=data_.data())
                            std::copy_n(other.data_,total_,data_.data());
                    }
                    else std::copy_n(other.data_.begin(),total_,data_.begin());
                }
            }
            return *this;
        }
        template<bool OTHER_VIEW>
        constexpr LinearObject& operator=(LinearObject<OTHER_VIEW,VALUE_T,RANKS...>& other){
            if(this!=&other){
                if constexpr(VIEW){
                    if constexpr(!OTHER_VIEW)
                        data_ = other.data_.data();
                    else data_ = other.data_;
                }
                else{
                    if constexpr(OTHER_VIEW){
                        if(other.data_!=data_.data())
                            std::copy_n(other.data_,total_,data_.data());
                    }
                    else std::copy_n(other.data_.begin(),total_,data_.begin());
                }
            }
            return *this;
        }
        template<bool OTHER_VIEW>
        constexpr bool operator==(const LinearObject<OTHER_VIEW,VALUE_T,RANKS...>& other) const noexcept{
            if constexpr (VIEW){
                for(std::size_t i = 0;i<total_;++i){
                    if constexpr(OTHER_VIEW){
                        if(*(data_+i)!=*(other.data_+i))
                            return false;
                    }
                    else{
                        if(*(data_+i)!=other.data_[i])
                            return false;
                    }
                }
                return true;
            }
            else{
                if constexpr(OTHER_VIEW){
                    for(std::size_t i = 0;i<total_;++i)
                        if(data_[i]!=*(other.data_+i))
                            return false;
                    return true;
                }
                else return data_ == other.data_;
            }
        }
        static constexpr std::size_t order() noexcept{
            return order_;
        }
        template<std::size_t... IDS>
        constexpr decltype(auto) element() const noexcept{
            static_assert(sizeof...(IDS)<=order_,
                "too many indices");
            static_assert(bounds_ok<IDS...>(),
                "index out of bounds");
            if constexpr(sizeof...(IDS)==order_){
                if constexpr (!VIEW)
                    return data_[compute_offset<IDS...>()];
                else return *(data_+compute_offset<IDS...>());
            }
            else{
                using Result = std::conditional_t<VIEW,
                DropRanks<VALUE_T, sizeof...(IDS), RANKS...>,
                DropRanks<const VALUE_T, sizeof...(IDS), RANKS...>>;
                Result result;
                if constexpr (VIEW){
                    result.data_ = data_+compute_offset<IDS...>();
                    return result;
                }
                else {
                    result.data_ = data_.data()+compute_offset<IDS...>();
                    return result;
                }
            }
        }
        template<std::size_t... IDS>
        constexpr decltype(auto) element() noexcept{
           static_assert(sizeof...(IDS)<=order_,
                "too many indices");
            static_assert(bounds_ok<IDS...>(),
                "index out of bounds");
            if constexpr(sizeof...(IDS)==order_){
                if constexpr (!VIEW)
                    return data_[compute_offset<IDS...>()];
                else return *(data_+compute_offset<IDS...>());
            }
            else{
                using Result = DropRanks<VALUE_T, sizeof...(IDS), RANKS...>;
                Result result;
                if constexpr (VIEW){
                    result.data_ = data_+compute_offset<IDS...>();
                    return result;
                }
                else {
                    result.data_ = data_.data()+compute_offset<IDS...>();
                    return result;
                }
            }
        }
    };
}