#include <gtest/gtest.h>
#include "OsterLib/types/time_interval.h"

using namespace std::chrono;

TEST(TimeIntervalTest,TimeIntervalInitTest){
    {
        __time_interval__ default_ti;
        ASSERT_EQ(default_ti.from(),utc_tp());
        ASSERT_EQ(default_ti.to(),utc_tp());
        default_ti = __time_interval__(sys_days(1991y/1/1),sys_days(1991y/1/2));
        ASSERT_EQ(default_ti.from(),sys_days(1991y/1/1));
        ASSERT_EQ(default_ti.to(),sys_days(1991y/1/2));
    }
    {
        __time_interval__ reversed_ti(sys_days(1991y/1/2),sys_days(1991y/1/1));
        ASSERT_EQ(reversed_ti.from(),sys_days(1991y/1/1));
        ASSERT_EQ(reversed_ti.to(),sys_days(1991y/1/2));
    }
    {
        __time_interval__ same_ti(sys_days(1991y/1/1),sys_days(1991y/1/1));
        ASSERT_EQ(same_ti.from(),sys_days(1991y/1/1));
        ASSERT_EQ(same_ti.to(),sys_days(1991y/1/1));
    }
}

TEST(TimeIntervalTest,intervals_intersect_test){
    ASSERT_TRUE(intervals_intersect(__time_interval__(sys_days(1991y/1/1),sys_days(1991y/1/1)),
                                    __time_interval__(sys_days(1991y/1/1),sys_days(1991y/1/1))));
    ASSERT_TRUE(intervals_intersect(__time_interval__(sys_days(1991y/1/1),sys_days(1992y/1/1)),
                                    __time_interval__(sys_days(1991y/1/1),sys_days(1991y/2/1))));
    ASSERT_TRUE(intervals_intersect(__time_interval__(sys_days(1991y/1/1),sys_days(1992y/1/1)),
                                    __time_interval__(sys_days(1991y/2/1),sys_days(1991y/3/1))));
    ASSERT_TRUE(intervals_intersect(__time_interval__(sys_days(1991y/2/1),sys_days(1991y/3/1)),
                                    __time_interval__(sys_days(1991y/1/1),sys_days(1992y/1/1))));
    ASSERT_TRUE(intervals_intersect(__time_interval__(sys_days(1991y/1/1),sys_days(1991y/2/1)),
                                    __time_interval__(sys_days(1991y/1/1),sys_days(1992y/1/1))));
    ASSERT_TRUE(intervals_intersect(__time_interval__(sys_days(1990y/12/31),sys_days(1992y/3/1)),
                                    __time_interval__(sys_days(1991y/1/1),sys_days(1992y/1/1))));
    ASSERT_TRUE(intervals_intersect(__time_interval__(sys_days(1991y/1/1),sys_days(1992y/1/1)),
                                    __time_interval__(sys_days(1990y/12/31),sys_days(1992y/3/1))));
}

TEST(TimeIntervalTest,interval_intersection_pos_test){
    std::error_code err = std::error_code();
    auto ts = TimeSequence(sys_days(year(1990)/month(1)/day(3)),sys_days(year(1990)/month(1)/day(31)),err,days(1));
    ASSERT_EQ(err,std::error_code());
    auto beg_end = interval_intersection_pos(__time_interval__(sys_days(1990y/month(1)/day(1)),sys_days(year(1990)/month(1)/day(2))),
                            ts,err);
    ASSERT_FALSE(beg_end.has_value());
    ts = TimeSequence(sys_days(year(1990)/month(1)/day(1)),sys_days(year(1990)/month(1)/day(2)),err,days(1));
    ASSERT_EQ(err,std::error_code());
    beg_end = interval_intersection_pos(__time_interval__(sys_days(1990y/month(1)/day(1)),sys_days(year(1990)/month(1)/day(1))+days(1)),
                            ts,err);
    EXPECT_EQ(beg_end->first,0);
    EXPECT_EQ(beg_end->second,1);
    ts = TimeSequence(sys_days(year(1990)/month(1)/day(1)),sys_days(year(1990)/month(1)/day(31)),err,days(1));
    ASSERT_EQ(err,std::error_code());
    beg_end = interval_intersection_pos(__time_interval__(sys_days(1990y/month(1)/day(1)),sys_days(year(1990)/month(1)/day(1))+days(31)),
                            ts,err);
    EXPECT_EQ(beg_end->first,0);
    EXPECT_EQ(beg_end->second,30);
    ts = TimeSequence(sys_days(year(1990)/month(1)/day(1)),sys_days(year(1990)/month(1)/day(31)),err,days(1));
    ASSERT_EQ(err,std::error_code());
    beg_end = interval_intersection_pos(__time_interval__(sys_days(1990y/month(1)/day(1)),sys_days(year(1990)/month(1)/day(1))+days(30)),
                            ts,err);
    EXPECT_EQ(beg_end->first,0);
    EXPECT_EQ(beg_end->second,30);
    ts = TimeSequence(sys_days(year(1990)/month(1)/day(1)),sys_days(year(1990)/month(1)/day(32)),err,days(1));
    ASSERT_EQ(err,std::error_code());
    beg_end = interval_intersection_pos(__time_interval__(sys_days(1990y/month(1)/day(1)),sys_days(year(1990)/month(1)/day(1))+days(31)),
                            ts,err);
    EXPECT_EQ(beg_end->first,0);
    EXPECT_EQ(beg_end->second,31);
    ts  = TimeSequence(sys_days(year(1990)/month(1)/day(1)),sys_days(year(1991)/month(1)/day(1)),err,days(1));
    ASSERT_EQ(err,std::error_code());
    ts = TimeSequence(sys_days(year(1990)/month(1)/day(1)),sys_days(year(1991)/month(1)/day(1)),err,days(1));
    ASSERT_EQ(err,std::error_code());
    beg_end = interval_intersection_pos(__time_interval__(sys_days(1990y/month(1)/day(1)),sys_days(year(1990)/month(12)/day(25))),
                            ts,err);
    EXPECT_EQ(beg_end->first,0);
    EXPECT_EQ(beg_end->second,358);
    ts = TimeSequence(sys_days(year(1990)/month(1)/day(1)),sys_days(year(1991)/month(1)/day(1)),err,days(1));
    ASSERT_EQ(err,std::error_code());
    beg_end = interval_intersection_pos(__time_interval__(sys_days(1990y/month(1)/day(1))+days(30),sys_days(year(1990)/month(12)/day(25))-days(30)),
                            ts,err);
    EXPECT_EQ(beg_end->first,30);
    EXPECT_EQ(beg_end->second,328);
    ts = TimeSequence(sys_days(year(1990)/month(1)/day(1)),sys_days(year(1991)/month(1)/day(1)),err,days(1));
    ASSERT_EQ(err,std::error_code());
    beg_end = interval_intersection_pos(__time_interval__(sys_days(1990y/month(1)/day(1))-days(30),sys_days(year(1990)/month(12)/day(25))+days(30)),
                            ts,err);
    EXPECT_EQ(beg_end->first,0);
    EXPECT_EQ(beg_end->second,(sys_days(year(1991)/month(1)/day(1))-sys_days(year(1990)/month(1)/day(1)))/days(1));
    ts = TimeSequence(sys_days(year(1990)/month(1)/day(1)),sys_days(year(1990)/month(1)/day(31)),err,days(1));
    beg_end = interval_intersection_pos(__time_interval__(sys_days(1990y/month(1)/day(1)),sys_days(year(1990)/month(1)/day(2))),
                            ts,err);
    EXPECT_EQ(beg_end->first,0);
    EXPECT_EQ(beg_end->second,1);
}

TEST(TimeIntervalTest,interval_instersection_test){
    auto res = interval_intersection(__time_interval__(sys_days(1990y/month(1)/day(1)),sys_days(year(1990)/month(1)/day(10))),
                            __time_interval__(sys_days(year(1990)/month(2)/day(1)),sys_days(year(1991)/month(1)/day(1))));
    ASSERT_FALSE(res.has_value());
    res = interval_intersection(__time_interval__(sys_days(1990y/month(1)/day(1)),sys_days(year(1991)/month(1)/day(1))),
                            __time_interval__(sys_days(year(1990)/month(1)/day(1)),sys_days(year(1991)/month(1)/day(1))));
    ASSERT_TRUE(res.has_value());
    {
        auto expected = __time_interval__(sys_days(1990y/month(1)/day(1)),sys_days(year(1991)/month(1)/day(1)));
        EXPECT_EQ(res.value(),expected);
    }
    
    res = interval_intersection(__time_interval__(sys_days(1990y/month(1)/day(1)),sys_days(year(1990)/month(10)/day(1))),
                            __time_interval__(sys_days(year(1990)/month(2)/day(1)),sys_days(year(1991)/month(1)/day(1))));
    ASSERT_TRUE(res.has_value());
    {
        auto expected = __time_interval__(sys_days(1990y/month(2)/day(1)),sys_days(year(1990)/month(10)/day(1)));
        EXPECT_EQ(res.value(),expected);
    }

    res = interval_intersection(__time_interval__(sys_days(1989y/month(1)/day(1)),sys_days(year(1991)/month(10)/day(1))),
                        __time_interval__(sys_days(year(1990)/month(2)/day(1)),sys_days(year(1991)/month(1)/day(1))));
    ASSERT_TRUE(res.has_value());
    {
        auto expected = __time_interval__(sys_days(1990y/month(2)/day(1)),sys_days(year(1991)/month(1)/day(1)));
        EXPECT_EQ(res.value(),expected);
    }

    res = interval_intersection(__time_interval__(sys_days(1990y/month(1)/day(1)),sys_days(year(1991)/month(10)/day(1))),
                    __time_interval__(sys_days(year(1989)/month(2)/day(1)),sys_days(year(1991)/month(1)/day(1))));
    ASSERT_TRUE(res.has_value());
    {
        auto expected = __time_interval__(sys_days(1990y/month(1)/day(1)),sys_days(year(1991)/month(1)/day(1)));
        EXPECT_EQ(res.value(),expected);
    }
}

// TEST(TimeIntervalTest,bounds_search){
//     using TimeInterval = __time_interval__<std::chrono::seconds>;
//     std::set<TimeInterval> set;
//     auto beg_check = set.insert(TimeInterval(sys_days(1991y/1/1),sys_days(2000y/12/1))).first;
//     set.insert(TimeInterval(sys_days(1995y/1/1),sys_days(1998y/5/1)));
//     set.insert(TimeInterval(sys_days(1980y/1/1),sys_days(2010y/11/1)));
//     set.insert(TimeInterval(sys_days(1996y/1/1),sys_days(1998y/3/1)));
//     set.insert(TimeInterval(sys_days(2005y/1/1),sys_days(2010y/12/1)));
//     set.insert(TimeInterval(sys_days(1999y/1/1),sys_days(2008y/7/1)));
//     ASSERT_EQ(set.size(),6);
//     TimeInterval to_search(sys_days(1992y/1/1),sys_days(2001y/1/1));
//     auto beg = set.lower_bound(to_search);
//     auto end = set.upper_bound(to_search);
//     ASSERT_NE(beg,end);
//     EXPECT_EQ(beg_check,beg);
// }

TEST(DateTimeDiffTest,operators_test){
    utc_tp_t<std::chrono::seconds> time;
    DateTimeDiff diff;
    utc_tp_t<std::chrono::seconds> result = diff+time;
    result = time+diff;
    utc_tp_t<std::chrono::days> result_d = std::chrono::floor<std::chrono::days>(diff+time);
}

int main(int argc,char* argv[]){
    testing::InitGoogleTest(&argc,argv);
    return RUN_ALL_TESTS();
}