#pragma once
#include "OsterLib/linalgebra/LinearObject.h"
#include "OsterLib/linalgebra/operators.h"

namespace linalgebra{
    template<typename VALUE_T,std::size_t RANK>
    class Vector:public LinearObject<false,VALUE_T,RANK>{
        public:
        Vector() = default;
        Vector(const Vector& other) = default;
        Vector(Vector&& other) = default;
        Vector(std::array<VALUE_T, H_RANK>){
            data_ = std::move(arr);
        }
        template<typename OTHER_VALUE_T,std::size_t OTHER_RANK>
        Matrix<std::common_type_t<VALUE_T,OTHER_VALUE_T>,RANK,OTHER_RANK> operator*(const Vector<OTHER_VALUE_T,OTHER_RANK>& other){
            Matrix<std::common_type_t<VALUE_T,OTHER_VALUE_T>,RANK,OTHER_RANK> result;
            []<std::size_t... Is1>(std::index_sequence<Is1...>){
                auto calculate_matrix = []<size_t VID1,std::size_t... Is2>(std::index_sequence<Is2>){
                    return other.element<
                };
            }(std::make_index_sequence<RANK>());
            for(std::size_t i = 0;i<RANK;++i)
                for(std::size_t j = 0;j<OTHER_RANK;++j)
                    result.data_
        }
    };
}