#include <gtest/gtest.h>
#include "types/time_interval.h"

using namespace std::chrono;

TEST(TimeIntervalTest,interval_intersection_pos_test){
    auto beg_end = interval_intersection_pos(TimeInterval{.from_=sys_days(1990y/month(1)/day(1)),.to_=sys_days(year(1990)/month(12)/day(25))},
                            TimeInterval{.from_=sys_days(year(1990)/month(1)/day(1)),.to_=sys_days(year(1991)/month(1)/day(1))},days(1));
    EXPECT_EQ(beg_end.first,0);
    EXPECT_EQ(beg_end.second,358);
    beg_end = interval_intersection_pos(TimeInterval{.from_=sys_days(1990y/month(1)/day(1))+days(30),.to_=sys_days(year(1990)/month(12)/day(25))-days(30)},
                            TimeInterval{.from_=sys_days(year(1990)/month(1)/day(1)),.to_=sys_days(year(1991)/month(1)/day(1))},days(1));
    EXPECT_EQ(beg_end.first,30);
    EXPECT_EQ(beg_end.second,328);
    beg_end = interval_intersection_pos(TimeInterval{.from_=sys_days(1990y/month(1)/day(1))-days(30),.to_=sys_days(year(1990)/month(12)/day(25))+days(30)},
                            TimeInterval{.from_=sys_days(year(1990)/month(1)/day(1)),.to_=sys_days(year(1991)/month(1)/day(1))},days(1));
    EXPECT_EQ(beg_end.first,0);
    EXPECT_EQ(beg_end.second,(sys_days(year(1991)/month(1)/day(1))-sys_days(year(1990)/month(1)/day(1)))/days(1));
}

TEST(TimeIntervalTest,interval_instersection_test){
    auto res = interval_intersection(TimeInterval{.from_=sys_days(1990y/month(1)/day(1)),.to_=sys_days(year(1990)/month(1)/day(10))},
                            TimeInterval{.from_=sys_days(year(1990)/month(2)/day(1)),.to_=sys_days(year(1991)/month(1)/day(1))});
    ASSERT_FALSE(res.has_value());
    res = interval_intersection(TimeInterval{.from_=sys_days(1990y/month(1)/day(1)),.to_=sys_days(year(1991)/month(1)/day(1))},
                            TimeInterval{.from_=sys_days(year(1990)/month(1)/day(1)),.to_=sys_days(year(1991)/month(1)/day(1))});
    ASSERT_TRUE(res.has_value());
    {
        auto expected = TimeInterval{.from_=sys_days(1990y/month(1)/day(1)),.to_=sys_days(year(1991)/month(1)/day(1))};
        EXPECT_EQ(res.value(),expected);
    }
    
    res = interval_intersection(TimeInterval{.from_=sys_days(1990y/month(1)/day(1)),.to_=sys_days(year(1990)/month(10)/day(1))},
                            TimeInterval{.from_=sys_days(year(1990)/month(2)/day(1)),.to_=sys_days(year(1991)/month(1)/day(1))});
    ASSERT_TRUE(res.has_value());
    {
        auto expected = TimeInterval{.from_=sys_days(1990y/month(2)/day(1)),.to_=sys_days(year(1990)/month(10)/day(1))};
        EXPECT_EQ(res.value(),expected);
    }

    res = interval_intersection(TimeInterval{.from_=sys_days(1989y/month(1)/day(1)),.to_=sys_days(year(1991)/month(10)/day(1))},
                        TimeInterval{.from_=sys_days(year(1990)/month(2)/day(1)),.to_=sys_days(year(1991)/month(1)/day(1))});
    ASSERT_TRUE(res.has_value());
    {
        auto expected = TimeInterval{.from_=sys_days(1990y/month(2)/day(1)),.to_=sys_days(year(1991)/month(1)/day(1))};
        EXPECT_EQ(res.value(),expected);
    }

    res = interval_intersection(TimeInterval{.from_=sys_days(1990y/month(1)/day(1)),.to_=sys_days(year(1991)/month(10)/day(1))},
                    TimeInterval{.from_=sys_days(year(1989)/month(2)/day(1)),.to_=sys_days(year(1991)/month(1)/day(1))});
    ASSERT_TRUE(res.has_value());
    {
        auto expected = TimeInterval{.from_=sys_days(1990y/month(1)/day(1)),.to_=sys_days(year(1991)/month(1)/day(1))};
        EXPECT_EQ(res.value(),expected);
    }
}

int main(int argc,char* argv[]){
    testing::InitGoogleTest(&argc,argv);
    return RUN_ALL_TESTS();
}