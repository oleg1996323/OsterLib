#include "types/time_interval.h"
#include <gtest/gtest.h>

TEST(TimeSequence,ConstructionTest){
    {
        utc_tp cur_time = sys_days(year(1990)/month(1)/day(1));
        TimeSequence test1(cur_time);
        EXPECT_EQ(test1.discret(),utc_diff());
        EXPECT_EQ(test1.get_interval(),(TimeInterval(cur_time,cur_time)));
    }
    {
        TimeInterval interval =TimeInterval(sys_days(year(1990)/month(1)/day(1)),sys_days(year(1991)/month(1)/day(1)));
        TimeSequence test1(interval);
        EXPECT_EQ(test1.discret(),interval.to()-interval.from());
        EXPECT_EQ(test1.get_interval(),interval);
    }
    {
        auto from=sys_days(year(1990)/month(1)/day(1));
        auto to=sys_days(year(1991)/month(1)/day(1));
        TimeSequence test1(from,to,(to-from)/2);
        EXPECT_EQ(test1.discret(),(to-from)/2);
        EXPECT_EQ(test1.get_interval(),(TimeInterval(from,to)));
    }
}

TEST(TimeSequence,MakeFromRangeTest){
    {
        std::list<utc_tp> time_series;
        for(int i=0;i<11;++i)
            time_series.emplace_back(sys_days(1990y/1/1)+days(i));
        auto time_seq = TimeSequence::make_from_range(time_series);
        EXPECT_EQ(time_seq.first.discret(),days(1));
        auto interval = TimeInterval(sys_days(1990y/1/1),sys_days(1990y/1/11));
        EXPECT_EQ(time_seq.first.get_interval().from(),interval.from());
        EXPECT_EQ(time_seq.first.get_interval().to(),interval.to());
        EXPECT_EQ(time_seq.second,time_series.end());
    }

    {
        std::list<utc_tp> time_series;
        for(int i=0;i<11;++i)
            time_series.emplace_back(sys_days(1990y/1/1)+days(i));
        time_series.emplace_back(sys_days(1991y/1/1));
        auto time_seq = TimeSequence::make_from_range(time_series);
        EXPECT_EQ(time_seq.first.discret(),days(1));
        auto interval = TimeInterval(sys_days(1990y/1/1),sys_days(1990y/1/11));
        EXPECT_EQ(time_seq.first.get_interval().from(),interval.from());
        EXPECT_EQ(time_seq.first.get_interval().to(),interval.to());
        EXPECT_EQ(time_seq.second,std::prev(time_series.end()));
    }
    {
        std::list<utc_tp> time_series;
        time_series.emplace_back(sys_days(1991y/1/1));
        time_series.emplace_back(sys_days(1990y/1/1));
        EXPECT_THROW(TimeSequence::make_from_range(time_series),std::invalid_argument);
    }
    {
        std::list<utc_tp> time_series;
        time_series.emplace_back(sys_days(1990y/1/1));
        time_series.emplace_back(sys_days(1990y/1/1));
        time_series.emplace_back(sys_days(1991y/1/1));
        auto time_seq = TimeSequence::make_from_range(time_series);
        EXPECT_EQ(time_seq.first.discret(),utc_diff::zero());
        auto interval = TimeInterval(sys_days(1990y/1/1),sys_days(1990y/1/1));
        EXPECT_EQ(time_seq.first.get_interval().from(),interval.from());
        EXPECT_EQ(time_seq.first.get_interval().to(),interval.to());
        EXPECT_EQ(time_seq.second,std::next(time_series.begin(),2));
    }
}

TEST(TimeSequence,PushTimeTest){
    TimeSequence seq(sys_days(1991y/1/1));
    EXPECT_EQ(seq.get_interval(),TimeInterval(sys_days(1991y/1/1),sys_days(1991y/1/1)));
    EXPECT_EQ(seq.discret(),utc_diff::zero());
    seq.push_time(sys_days(1990y/1/1));
    EXPECT_EQ(seq.get_interval(),TimeInterval(sys_days(1990y/1/1),sys_days(1991y/1/1)));
    EXPECT_EQ(seq.discret(),sys_days(1991y/1/1)-sys_days(1990y/1/1));
    EXPECT_FALSE(seq.push_time(sys_days(1991y/1/1)));
    EXPECT_FALSE(seq.push_time(sys_days(1990y/1/1)));
    EXPECT_FALSE(seq.push_time(sys_days(1990y/1/2)));
    EXPECT_FALSE(seq.push_time(sys_days(1989y/12/31)));
    EXPECT_FALSE(seq.push_time(sys_days(1991y/1/2)));
    EXPECT_FALSE(seq.push_time(sys_days(1990y/12/31)));
    EXPECT_TRUE(seq.push_time(sys_days(1989y/1/1)));
}

TEST(TimeSequence, ExtendableTest){
    TimeSequence seq(TimeInterval(sys_days(1990y/1/1),sys_days(1990y/1/2)));
    std::vector<utc_tp> seq_rng = [](){
        std::vector<utc_tp> res;
        for(int i=0;i<10;++i)
            res.push_back(sys_days(1991y/1/i));
        return res;}();
    ASSERT_TRUE(seq.extendable(TimeSequence::make_from_range(seq_rng).first));
    ASSERT_TRUE(seq.extendable(TimeSequence(TimeInterval(sys_days(1989y/12/31),sys_days(1990y/1/1)))));
    ASSERT_TRUE(seq.extendable(TimeSequence(TimeInterval(sys_days(1990y/1/2),sys_days(1990y/1/10)))));
    ASSERT_TRUE(seq.extendable(TimeSequence(TimeInterval(sys_days(1990y/1/2),sys_days(2010y/1/10)))));
    ASSERT_FALSE(seq.extendable(TimeSequence(TimeInterval(sys_days(1990y/1/2)+hours(1),sys_days(2010y/1/10)))));
    ASSERT_FALSE(seq.extendable(TimeSequence(TimeInterval(sys_days(1990y/1/2)+hours(1),sys_days(2010y/1/10)+hours(1)))));
}

TEST(TimeSequence,ExtendByIntervalTest){
    TimeSequence seq(TimeInterval(sys_days(1990y/1/1),sys_days(1991y/1/1)));
    auto discret = seq.discret();
    EXPECT_TRUE(seq.extend_by_interval(TimeInterval(sys_days(1989y/1/1),sys_days(1992y/1/1))));
    EXPECT_FALSE(seq.extend_by_interval(TimeInterval(sys_days(1988y/1/2),sys_days(1988y/12/31))));
    EXPECT_EQ(discret,seq.discret());
}

int main(int argc,char* argv[]){
    testing::InitGoogleTest(&argc,argv);
    return RUN_ALL_TESTS();
}