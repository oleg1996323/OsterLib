#include "types/time_interval.h"
#include <gtest/gtest.h>
#include <string>

TEST(LexicalCast,utc_tp_lexical_cast_test){
    utc_tp tp = std::chrono::sys_days(1991y/1/1)+std::chrono::hours(12)+std::chrono::minutes(30);
    std::string str = boost::lexical_cast(tp);
    utc_tp other = boost::lexical_cast<utc_tp>(str);
}

int main(int argc,char* argv[]){
    testing::InitGoogleTest(&argc,argv);
    return RUN_ALL_TESTS();
}