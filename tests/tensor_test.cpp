#include "OsterLib/linalgebra/Strategy.h"
#include "OsterLib/linalgebra/LinearObject.h"
#include <gtest/gtest.h>


TEST(LinAlgebra,ContractionTest_Tensor_3_Rank_1){
    linalgebra::LinearObject<false,long double,2,2,2> lo;
    lo.element<0,0,0>() = 1;
    lo.element<1,0,0>() = 2;
    lo.element<0,1,0>() = 3;
    lo.element<1,1,0>() = 4;
    lo.element<0,0,1>() = 5;
    lo.element<1,0,1>() = 6;
    lo.element<0,1,1>() = 7;
    lo.element<1,1,1>() = 8;
    linalgebra::print(linalgebra::Contraction<1,2>()(lo));
    //[16, 20]
}

TEST(LinAlgebra,ContractionTest_Tensor_3_Rank_2){
    linalgebra::LinearObject<false, long double, 2, 2, 2> t;
    t.element<0,0,0>() = 0; t.element<0,0,1>() = 1;
    t.element<0,1,0>() = 2; t.element<0,1,1>() = 3;
    t.element<1,0,0>() = 4; t.element<1,0,1>() = 5;
    t.element<1,1,0>() = 6; t.element<1,1,1>() = 7;

    linalgebra::print(linalgebra::Contraction<0>()(t));

    //[[4, 6], [8, 10]]
}

TEST(LinAlgebra,ContractionTest_Tensor_3_Rank_3){
    linalgebra::LinearObject<false, long double, 2, 3, 2> t;
    // flat: i*6 + j*2 + k
    for (std::size_t i = 0; i < 12; ++i) t.data()[i] = static_cast<long double>(i);

    linalgebra::print(linalgebra::Contraction<0,2>()(t));
    //[14, 22, 30]
}

TEST(LinAlgebra,ContractionTest_Tensor_3_Rank_4){
    linalgebra::LinearObject<false, long double, 3, 2, 2> t;
    // flat: i*4 + j*2 + k
    for (std::size_t i = 0; i < 12; ++i) t.data()[i] = static_cast<long double>(i);

    linalgebra::print(linalgebra::Contraction<1>()(t));
    //[[2, 4], [10, 12], [18, 20]]
}

TEST(LinAlgebra,ContractionTest_Tensor_3_Rank_5){
    linalgebra::LinearObject<false, long double, 2, 2, 2, 2> t;
    // flat: i*8 + j*4 + k*2 + l
    for (std::size_t i = 0; i < 16; ++i) t.data()[i] = static_cast<long double>(i);

    linalgebra::print(linalgebra::Contraction<1,3>()(t));
    //[[10, 18], [42, 50]]
}

TEST(LinAlgebra,ContractionTest_Tensor_4_Rank_1){
    linalgebra::LinearObject<false, long double, 2, 2, 2, 2> t;
    for (std::size_t i = 0; i < 16; ++i) t.data()[i] = static_cast<long double>(i);

    linalgebra::print(linalgebra::Contraction<0,1,2,3>()(t));
    //120
}

TEST(LinAlgebra,ContractionTest_Tensor_5_Rank_1){
    linalgebra::LinearObject<false, long double, 2, 2, 2, 2, 2> t;
    // flat: i*16 + j*8 + k*4 + l*2 + m
    for (std::size_t i = 0; i < 32; ++i) t.data()[i] = static_cast<long double>(i);

    linalgebra::print(linalgebra::Contraction<0,2,4>()(t));
    //[[84, 100], [148, 164]]
}

TEST(LinAlgebra,InnerProductTest_Tensor_5_Rank_1){
    linalgebra::LinearObject<false, long double, 2, 2, 2, 2, 2> t1;
    // flat: i*16 + j*8 + k*4 + l*2 + m
    for (std::size_t i = 0; i < 32; ++i) t1.data()[i] = static_cast<long double>(i);

    linalgebra::LinearObject<false, long double, 2, 2, 2, 2, 2> t2;
    // flat: i*16 + j*8 + k*4 + l*2 + m
    for (std::size_t i = 0; i < 32; ++i) t2.data()[i] = static_cast<long double>(i);

    std::cout<<linalgebra::InnerProduct()(t1,t2)<<std::endl;
    //[[84, 100], [148, 164]]
}

TEST(LinAlgebra,TransposeTest_Tensor_5_Rank_1){
    linalgebra::LinearObject<false, long double, 2, 2, 2, 2, 2> t;
    // flat: i*16 + j*8 + k*4 + l*2 + m
    for (std::size_t i = 0; i < 32; ++i) t.data()[i] = static_cast<long double>(i);

    linalgebra::print(t);
    linalgebra::print(linalgebra::Transpose()(t));
    //[[84, 100], [148, 164]]
}

int main(int argc,char* argv[]){
    testing::InitGoogleTest(&argc,argv);
    return RUN_ALL_TESTS();
}