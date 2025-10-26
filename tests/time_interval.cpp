#include <gtest/gtest.h>
#include "types/time_interval.h"

using namespace std::chrono;

TEST(TimeIntervalTest,TimeIntervalInitTest){
    {
        TimeInterval default_ti;
        ASSERT_EQ(default_ti.from(),utc_tp());
        ASSERT_EQ(default_ti.to(),utc_tp());
        default_ti = TimeInterval(sys_days(1991y/1/1),sys_days(1991y/1/2));
        ASSERT_EQ(default_ti.from(),sys_days(1991y/1/1));
        ASSERT_EQ(default_ti.to(),sys_days(1991y/1/2));
    }
    {
        TimeInterval reversed_ti(sys_days(1991y/1/2),sys_days(1991y/1/1));
        ASSERT_EQ(reversed_ti.from(),sys_days(1991y/1/1));
        ASSERT_EQ(reversed_ti.to(),sys_days(1991y/1/2));
    }
    {
        TimeInterval same_ti(sys_days(1991y/1/1),sys_days(1991y/1/1));
        ASSERT_EQ(same_ti.from(),sys_days(1991y/1/1));
        ASSERT_EQ(same_ti.to(),sys_days(1991y/1/1));
    }
}

TEST(TimeIntervalTest,intervals_intersect_test){
    ASSERT_TRUE(intervals_intersect(TimeInterval(sys_days(1991y/1/1),sys_days(1991y/1/1)),
                                    TimeInterval(sys_days(1991y/1/1),sys_days(1991y/1/1))));
    ASSERT_TRUE(intervals_intersect(TimeInterval(sys_days(1991y/1/1),sys_days(1992y/1/1)),
                                    TimeInterval(sys_days(1991y/1/1),sys_days(1991y/2/1))));
    ASSERT_TRUE(intervals_intersect(TimeInterval(sys_days(1991y/1/1),sys_days(1992y/1/1)),
                                    TimeInterval(sys_days(1991y/2/1),sys_days(1991y/3/1))));
    ASSERT_TRUE(intervals_intersect(TimeInterval(sys_days(1991y/2/1),sys_days(1991y/3/1)),
                                    TimeInterval(sys_days(1991y/1/1),sys_days(1992y/1/1))));
    ASSERT_TRUE(intervals_intersect(TimeInterval(sys_days(1991y/1/1),sys_days(1991y/2/1)),
                                    TimeInterval(sys_days(1991y/1/1),sys_days(1992y/1/1))));
    ASSERT_TRUE(intervals_intersect(TimeInterval(sys_days(1990y/12/31),sys_days(1992y/3/1)),
                                    TimeInterval(sys_days(1991y/1/1),sys_days(1992y/1/1))));
    ASSERT_TRUE(intervals_intersect(TimeInterval(sys_days(1991y/1/1),sys_days(1992y/1/1)),
                                    TimeInterval(sys_days(1990y/12/31),sys_days(1992y/3/1))));
}

TEST(TimeIntervalTest,interval_intersection_pos_test){

    auto beg_end = interval_intersection_pos(TimeInterval(sys_days(1990y/month(1)/day(1)),sys_days(year(1990)/month(1)/day(2))),
                            TimeSequence(sys_days(year(1990)/month(1)/day(3)),sys_days(year(1990)/month(1)/day(31)),days(1)));
    ASSERT_FALSE(beg_end.has_value());
    beg_end = interval_intersection_pos(TimeInterval(sys_days(1990y/month(1)/day(1)),sys_days(year(1990)/month(1)/day(1))+days(1)),
                            TimeSequence(sys_days(year(1990)/month(1)/day(1)),sys_days(year(1990)/month(1)/day(2)),days(1)));
    EXPECT_EQ(beg_end->first,0);
    EXPECT_EQ(beg_end->second,1);
    beg_end = interval_intersection_pos(TimeInterval(sys_days(1990y/month(1)/day(1)),sys_days(year(1990)/month(1)/day(1))+days(31)),
                            TimeSequence(sys_days(year(1990)/month(1)/day(1)),sys_days(year(1990)/month(1)/day(31)),days(1)));
    EXPECT_EQ(beg_end->first,0);
    EXPECT_EQ(beg_end->second,30);
    beg_end = interval_intersection_pos(TimeInterval(sys_days(1990y/month(1)/day(1)),sys_days(year(1990)/month(1)/day(1))+days(30)),
                            TimeSequence(sys_days(year(1990)/month(1)/day(1)),sys_days(year(1990)/month(1)/day(31)),days(1)));
    EXPECT_EQ(beg_end->first,0);
    EXPECT_EQ(beg_end->second,30);
    beg_end = interval_intersection_pos(TimeInterval(sys_days(1990y/month(1)/day(1)),sys_days(year(1990)/month(1)/day(1))+days(31)),
                            TimeSequence(sys_days(year(1990)/month(1)/day(1)),sys_days(year(1990)/month(1)/day(32)),days(1)));
    EXPECT_EQ(beg_end->first,0);
    EXPECT_EQ(beg_end->second,31);
    beg_end = interval_intersection_pos(TimeInterval(sys_days(1990y/month(1)/day(1)),sys_days(year(1990)/month(12)/day(25))),
                            TimeSequence(sys_days(year(1990)/month(1)/day(1)),sys_days(year(1991)/month(1)/day(1)),days(1)));
    EXPECT_EQ(beg_end->first,0);
    EXPECT_EQ(beg_end->second,358);
    beg_end = interval_intersection_pos(TimeInterval(sys_days(1990y/month(1)/day(1))+days(30),sys_days(year(1990)/month(12)/day(25))-days(30)),
                            TimeSequence(sys_days(year(1990)/month(1)/day(1)),sys_days(year(1991)/month(1)/day(1)),days(1)));
    EXPECT_EQ(beg_end->first,30);
    EXPECT_EQ(beg_end->second,328);
    beg_end = interval_intersection_pos(TimeInterval(sys_days(1990y/month(1)/day(1))-days(30),sys_days(year(1990)/month(12)/day(25))+days(30)),
                            TimeSequence(sys_days(year(1990)/month(1)/day(1)),sys_days(year(1991)/month(1)/day(1)),days(1)));
    EXPECT_EQ(beg_end->first,0);
    EXPECT_EQ(beg_end->second,(sys_days(year(1991)/month(1)/day(1))-sys_days(year(1990)/month(1)/day(1)))/days(1));
}

TEST(TimeIntervalTest,interval_instersection_test){
    auto res = interval_intersection(TimeInterval(sys_days(1990y/month(1)/day(1)),sys_days(year(1990)/month(1)/day(10))),
                            TimeInterval(sys_days(year(1990)/month(2)/day(1)),sys_days(year(1991)/month(1)/day(1))));
    ASSERT_FALSE(res.has_value());
    res = interval_intersection(TimeInterval(sys_days(1990y/month(1)/day(1)),sys_days(year(1991)/month(1)/day(1))),
                            TimeInterval(sys_days(year(1990)/month(1)/day(1)),sys_days(year(1991)/month(1)/day(1))));
    ASSERT_TRUE(res.has_value());
    {
        auto expected = TimeInterval(sys_days(1990y/month(1)/day(1)),sys_days(year(1991)/month(1)/day(1)));
        EXPECT_EQ(res.value(),expected);
    }
    
    res = interval_intersection(TimeInterval(sys_days(1990y/month(1)/day(1)),sys_days(year(1990)/month(10)/day(1))),
                            TimeInterval(sys_days(year(1990)/month(2)/day(1)),sys_days(year(1991)/month(1)/day(1))));
    ASSERT_TRUE(res.has_value());
    {
        auto expected = TimeInterval(sys_days(1990y/month(2)/day(1)),sys_days(year(1990)/month(10)/day(1)));
        EXPECT_EQ(res.value(),expected);
    }

    res = interval_intersection(TimeInterval(sys_days(1989y/month(1)/day(1)),sys_days(year(1991)/month(10)/day(1))),
                        TimeInterval(sys_days(year(1990)/month(2)/day(1)),sys_days(year(1991)/month(1)/day(1))));
    ASSERT_TRUE(res.has_value());
    {
        auto expected = TimeInterval(sys_days(1990y/month(2)/day(1)),sys_days(year(1991)/month(1)/day(1)));
        EXPECT_EQ(res.value(),expected);
    }

    res = interval_intersection(TimeInterval(sys_days(1990y/month(1)/day(1)),sys_days(year(1991)/month(10)/day(1))),
                    TimeInterval(sys_days(year(1989)/month(2)/day(1)),sys_days(year(1991)/month(1)/day(1))));
    ASSERT_TRUE(res.has_value());
    {
        auto expected = TimeInterval(sys_days(1990y/month(1)/day(1)),sys_days(year(1991)/month(1)/day(1)));
        EXPECT_EQ(res.value(),expected);
    }
}

int main(int argc,char* argv[]){
    testing::InitGoogleTest(&argc,argv);
    return RUN_ALL_TESTS();
}