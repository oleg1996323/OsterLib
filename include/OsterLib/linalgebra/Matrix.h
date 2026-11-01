#pragma once
#include "OsterLib/linalgebra/LinearObject.h"

namespace linalgebra{
    template<std::size_t H_RANK, std::size_t V_RANK,typename VALUE_T>
    class Matrix;
    class MatrixWrapper;

    template<std::size_t H_RANK, std::size_t V_RANK,typename VALUE_T>
    class Matrix:public LinearObject<false,VALUE_T,H_RANK,V_RANK>{
        public:
        Matrix() = default;
        Matrix(const Matrix& other) = default;
        Matrix(Matrix&& other) = default;
        Matrix(std::array<std::array<VALUE_T,V_RANK>,H_RANK> arr):
            LinearObject()
        {
            for(std::size_t i=0;i<H_RANK;++i)
                for(std::size_t j=0;j<V_RANK;++j)
                    data_[i*V_RANK+j] = arr[i][j];
        }
        Matrix(std::array<VALUE_T, H_RANK>){
            data_ = std::move(arr);
        }
        Matrix<2,2,long double> operator*(const Matrix)
    };

    template<std::size_t H_RANK,typename VALUE_T>
    Matrix(std::array<VALUE_T, H_RANK>) -> Matrix<H_RANK, 1, VALUE_T>;
}