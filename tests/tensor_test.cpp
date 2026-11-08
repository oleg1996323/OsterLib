#include "OsterLib/linalgebra/Strategy.h"
#include "OsterLib/linalgebra/LinearObject.h"
#include <gtest/gtest.h>


TEST(LinAlgebra,StrategyTest){
    linalgebra::LinearObject<false,long double,2,2,2> lo;
    lo.element<0,0,0>() = 1;
    lo.element<1,0,0>() = 2;
    lo.element<0,1,0>() = 3;
    lo.element<1,1,0>() = 4;
    lo.element<0,0,1>() = 5;
    lo.element<1,0,1>() = 6;
    lo.element<0,1,1>() = 7;
    lo.element<1,1,1>() = 8;
    linalgebra::Contraction<1,2>()(lo);
}

int main(int argc,char* argv[]){
    testing::InitGoogleTest(&argc,argv);
    return RUN_ALL_TESTS();
}