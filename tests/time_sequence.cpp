#include "OsterLib/types/time_interval.h"
#include <gtest/gtest.h>

TEST(TimeSequence,DateTimeTest){
    utc_tp first = std::chrono::sys_days(1991y/2/1) +std::chrono::hours(2)+std::chrono::minutes(1)+std::chrono::seconds(3);
    utc_tp second = std::chrono::sys_days(1996y/1/1) +std::chrono::hours(10)+std::chrono::minutes(12)+std::chrono::seconds(55);
    DateTimeDiff dt(first,second);
    ASSERT_EQ(dt.years_,4);
    ASSERT_EQ(dt.months_,11);
    ASSERT_EQ(dt.days_,0);
    ASSERT_EQ(dt.hours_,8);
    ASSERT_EQ(dt.minutes_,11);
    ASSERT_EQ(dt.seconds_,52);
}

TEST(TimeSequence,NumberLeapDaysTest){
    auto from=sys_days(year(1990)/month(1)/day(1));
    auto to=sys_days(year(2100)/month(1)/day(1));
    ASSERT_EQ(DateTimeDiff::number_leap_days(std::chrono::year_month_day(from),std::chrono::year_month_day(to)),27);
    from=sys_days(year(1990)/month(1)/day(1));
    to=sys_days(year(2200)/month(1)/day(1));
    ASSERT_EQ(DateTimeDiff::number_leap_days(std::chrono::year_month_day(from),std::chrono::year_month_day(to)),51);
}

TEST(TimeSequence,ComputeNumberOfIntervalsTest){
    std::error_code err;
    DateTimeDiff dtd = DateTimeDiff(err,std::chrono::months(6));
    ASSERT_EQ(TimeSequence::compute_number_of_intervals(sys_days(year(1990)/month(1)/day(1))+std::chrono::hours(6)+std::chrono::minutes(30)+std::chrono::seconds(50),
                                                sys_days(year(1991)/month(1)/day(1))+std::chrono::hours(6)+std::chrono::minutes(30)+std::chrono::seconds(50),dtd,err),2);
    dtd = DateTimeDiff(err,std::chrono::years(109));
    ASSERT_EQ(TimeSequence::compute_number_of_intervals(sys_days(year(2100)/month(1)/day(1)),
                                                sys_days(year(1991)/month(1)/day(1)),dtd,err),1);
    dtd = DateTimeDiff(err,std::chrono::days(5));
    ASSERT_EQ(TimeSequence::compute_number_of_intervals(sys_days(year(1992)/month(1)/day(1)),
                                                sys_days(year(1991)/month(1)/day(1)),dtd,err),73);

    dtd = DateTimeDiff(err,std::chrono::years(1),std::chrono::months(6));
    ASSERT_EQ(TimeSequence::compute_number_of_intervals(sys_days(year(1990)/month(1)/day(1)),
                                                sys_days(year(1994)/month(7)/day(1)),dtd,err),3);
    dtd = DateTimeDiff(err,std::chrono::months(1),std::chrono::days(0),
                        std::chrono::hours(0),std::chrono::minutes(19),std::chrono::seconds(45));
    ASSERT_EQ(TimeSequence::compute_number_of_intervals(sys_days(year(1990)/month(10)/day(31))+hours(23)+minutes(20)+seconds(30),
                                                sys_days(year(1991)/month(1)/day(1)),dtd,err),2);
    dtd = DateTimeDiff(err,std::chrono::years(5),std::chrono::months(5),std::chrono::days(15),
                        std::chrono::hours(23),std::chrono::minutes(19),std::chrono::seconds(45));

    // year_month_day tmp(sys_days((year(1990)/month(10)/day(31))+
    //                                             years(dtd.years_)*10+months(dtd.months_)*10)+days(dtd.days_)*10);
    auto from = sys_days(year(1990)/month(10)/day(31))+hours(23)+minutes(20)+seconds(30);
    year_month_day ymd_to(sys_days(floor<days>(from)));
    auto to = time_point_cast<seconds>(sys_days(ymd_to+
                years(dtd.years_)*10+months(dtd.months_)*10)+days(dtd.days_)*10);
    //std::cout<<to<<std::endl;
    to+=from-floor<days>(from);
    //std::cout<<to<<std::endl;
    to+=hours(dtd.hours_)*10+minutes(dtd.minutes_)*10+seconds(dtd.seconds_)*10;
    //std::cout<<to<<std::endl;
    ASSERT_EQ(TimeSequence::compute_number_of_intervals(from,
                                                to
                                                ,dtd,err),10);
}

TEST(TimeSequence,ComputeRawNumberOfIntervalsTest){
    std::error_code err;
    DateTimeDiff dtd = DateTimeDiff(err,std::chrono::months(6));
    ASSERT_EQ(TimeSequence::full_number_of_intervals(sys_days(year(1990)/month(1)/day(1))+std::chrono::hours(6)+std::chrono::minutes(30)+std::chrono::seconds(50),
                                                sys_days(year(1991)/month(1)/day(1))+std::chrono::hours(6)+std::chrono::minutes(30)+std::chrono::seconds(50),dtd,err),2);
    dtd = DateTimeDiff(err,std::chrono::years(109));
    ASSERT_EQ(TimeSequence::full_number_of_intervals(sys_days(year(2100)/month(1)/day(1)),
                                                sys_days(year(1991)/month(1)/day(1)),dtd,err),1);
    dtd = DateTimeDiff(err,std::chrono::days(5));
    ASSERT_EQ(TimeSequence::full_number_of_intervals(sys_days(year(1992)/month(1)/day(1)),
                                                sys_days(year(1991)/month(1)/day(1)),dtd,err),73);

    dtd = DateTimeDiff(err,std::chrono::years(1),std::chrono::months(6));
    ASSERT_EQ(TimeSequence::full_number_of_intervals(sys_days(year(1990)/month(1)/day(1)),
                                                sys_days(year(1994)/month(7)/day(1)),dtd,err),3);
    dtd = DateTimeDiff(err,std::chrono::months(1),std::chrono::days(0),
                        std::chrono::hours(0),std::chrono::minutes(19),std::chrono::seconds(45));
    ASSERT_EQ(TimeSequence::full_number_of_intervals(sys_days(year(1990)/month(10)/day(31))+hours(23)+minutes(20)+seconds(30),
                                                sys_days(year(1991)/month(1)/day(1)),dtd,err),2);
    dtd = DateTimeDiff(err,std::chrono::months(1),std::chrono::days(0));
    ASSERT_EQ(TimeSequence::full_number_of_intervals(sys_days(year(1990)/month(1)/day(1)),
                                                sys_days(year(1990)/month(1)/day(16))+hours(12),dtd,err),0);
    dtd = DateTimeDiff(err,std::chrono::years(10),std::chrono::months(10),std::chrono::days(10),
                        std::chrono::hours(10),std::chrono::minutes(10),std::chrono::seconds(10));
    auto from = sys_days(year(1990)/month(10)/day(31))+hours(23)+minutes(20)+seconds(30);
    year_month_day ymd_to(sys_days(floor<days>(from)));
    auto to = time_point_cast<seconds>(sys_days(ymd_to+
                years(dtd.years_)*10-years(5)+
                months(dtd.months_)*10-months(5))+
                hours(23)+minutes(20)+seconds(30)+
                days(dtd.days_)*10-days(5)+
                hours(dtd.hours_)*10-hours(5)+
                minutes(dtd.minutes_)*10-minutes(5)+
                seconds(dtd.seconds_)*10-seconds(5));
    //std::cout<<"to:"<<to<<std::endl;
    //std::cout<<DateTimeDiff::days_between_dates(year_month_day(floor<days>(from)),
                    //year_month_day(floor<days>(to)))<<std::endl;
    //std::cout<<DateTimeDiff::number_leap_days(year_month_day(floor<days>(from)),year_month_day(floor<days>(to)))<<std::endl;
    ASSERT_EQ(TimeSequence::full_number_of_intervals(from,
                                                to
                                                ,dtd,err),9);
}

TEST(TimeSequence,BoundByInterval_test){
    std::error_code err;
    using TimeInterval = __time_interval__<seconds>;
    DateTimeDiff dtd = DateTimeDiff(err,std::chrono::months(6));
    TimeSequence ts(sys_days(year(1990)/month(1)/day(1))+std::chrono::hours(6)+std::chrono::minutes(30)+std::chrono::seconds(50),
                        sys_days(year(1991)/month(1)/day(1))+std::chrono::hours(6)+std::chrono::minutes(30)+std::chrono::seconds(50),dtd,err);
    ASSERT_EQ(err,std::error_code());
    TimeInterval interval(sys_days(year(1990)/month(1)/day(1))+std::chrono::hours(6)+std::chrono::minutes(30)+std::chrono::seconds(50),
                        sys_days(year(1990)/month(12)/day(1)));
    TimeSequence bounded = ts.bound_by_interval(interval,err);
    ASSERT_EQ(err,std::error_code());
    ASSERT_EQ(bounded,TimeSequence(sys_days(year(1990)/month(1)/day(1))+std::chrono::hours(6)+std::chrono::minutes(30)+std::chrono::seconds(50),
                        sys_days(year(1990)/month(1)/day(1)+months(dtd.months_))+std::chrono::hours(6)+std::chrono::minutes(30)+std::chrono::seconds(50),1,err));
    ASSERT_EQ(err,std::error_code());
    dtd = DateTimeDiff(err,std::chrono::years(1));
    ts = TimeSequence(sys_days(year(1991)/month(1)/day(1)),
                        sys_days(year(2100)/month(1)/day(1)),dtd,err);
    ASSERT_EQ(err,std::error_code());
    interval = TimeInterval(sys_days(year(2000)/month(2)/day(30)),
                        sys_days(year(2020)/month(12)/day(1)));
    bounded = ts.bound_by_interval(interval,err);
    ASSERT_EQ(err,std::error_code());
    ASSERT_EQ(bounded,TimeSequence(sys_days(year(2001)/month(1)/day(1)),
                        sys_days(year(2020)/month(1)/day(1)),1,err));
    ASSERT_EQ(err,std::error_code());
    // dtd = DateTimeDiff(err,std::chrono::years(5),std::chrono::months(5),);
    // ts = TimeSequence(sys_days(year(1991)/month(1)/day(1)),
    //                     sys_days(year(2100)/month(1)/day(1)),dtd,err);
    // ASSERT_EQ(err,std::error_code());
    // interval = TimeInterval(sys_days(year(2000)/month(2)/day(30)),
    //                     sys_days(year(2020)/month(12)/day(1)));
    // bounded = ts.bound_by_interval(interval,err);
    // ASSERT_EQ(err,std::error_code());
    // ASSERT_EQ(bounded,TimeSequence(sys_days(year(2001)/month(1)/day(1)),
    //                     sys_days(year(2020)/month(1)/day(1)),1,err));
    // ASSERT_EQ(err,std::error_code());
}

TEST(TimeSequence,ConstructionTest){
    {
        utc_tp cur_time = sys_days(year(1990)/month(1)/day(1));
        TimeSequence test1 = TimeSequence(cur_time);
        EXPECT_EQ(test1.time_duration(),DateTimeDiff());
        EXPECT_EQ(test1.get_interval(),(__time_interval__(cur_time,cur_time)));
    }
    {
        __time_interval__ interval =__time_interval__(sys_days(year(1990)/month(1)/day(1)),sys_days(year(1991)/month(1)/day(1)));
        std::error_code err = std::error_code();
        TimeSequence test1(interval);
        EXPECT_EQ(test1.time_duration(),DateTimeDiff(interval.from(),interval.to()));
        EXPECT_EQ(test1.get_interval(),interval);
    }
    {
        auto from=sys_days(year(1990)/month(10)/day(31))+hours(23)+minutes(20)+seconds(30);
        auto to=sys_days(year(1991)/month(1)/day(1));
        std::error_code err;
        TimeSequence test1(from,to,2,err);
        EXPECT_EQ(test1.time_duration(),TimeSequence(from,to,err,
                    std::chrono::months(1),std::chrono::days(0),std::chrono::hours(0),std::chrono::minutes(19),std::chrono::seconds(45)).time_duration());
        EXPECT_EQ(test1.get_interval(),(__time_interval__(from,to)));
        from = sys_days(year(1990)/month(9)/day(19))+hours(23)+minutes(20)+seconds(30);
        to=sys_days(year(1991)/month(1)/day(1));
    }
    {
        auto from=sys_days(year(1990)/month(1)/day(1));
        auto to=sys_days(year(1990)/month(1)/day(2));
        std::error_code err;
        TimeSequence test(from,to,err,std::chrono::days(1));
        EXPECT_EQ(test.time_duration(),DateTimeDiff(from,to,1,err));
        to = sys_days(year(1991)/month(1)/day(1));
        test = TimeSequence(from,to,2,err);
        ASSERT_EQ(test.time_duration(),DateTimeDiff(err,std::chrono::months(6)));
    }
}
TEST(TimeSequence,DateTimeDiffExtremalTest){
    {
        auto from=sys_days(year(1990)/month(1)/day(1));
        utc_tp_t<std::chrono::seconds> to=sys_days(year(2100)/month(1)/day(1));
        std::error_code err;
        TimeSequence test(from,to,err,std::chrono::years(1));
        ASSERT_EQ(err,std::error_code());
        err = std::error_code();
        EXPECT_EQ(test.time_duration(),DateTimeDiff(from,to,110,err));
        ASSERT_EQ(err,std::error_code());
        from=sys_days(year(1990)/month(1)/day(1));
        to=sys_days(year(2100)/month(1)/day(1))+std::chrono::seconds(110+60*110+3600*110);
        
        test = TimeSequence(from,to,err,std::chrono::years(1),std::chrono::months(0),
                                    std::chrono::days(0),std::chrono::hours(1),
                                std::chrono::minutes(1),std::chrono::seconds(1));
        ASSERT_EQ(err,std::error_code());
        err = std::error_code();
        EXPECT_EQ(test.time_duration(),DateTimeDiff(from,to,110,err));
        ASSERT_EQ(err,std::error_code());
    }
}
TEST(TimeSequence,MakeFromRangeTest){
    {
        std::list<utc_tp> time_series;
        for(int i=0;i<11;++i)
            time_series.emplace_back(sys_days(1990y/1/1)+days(i));
        std::error_code err = std::error_code();
        auto time_seq = TimeSequence::make_from_range(time_series,err);
        EXPECT_EQ(time_seq.first.time_duration(),DateTimeDiff(err,std::chrono::days(1)));
        auto interval = __time_interval__(sys_days(1990y/1/1),sys_days(1990y/1/11));
        EXPECT_EQ(time_seq.first.get_interval().from(),interval.from());
        EXPECT_EQ(time_seq.first.get_interval().to(),interval.to());
        EXPECT_EQ(time_seq.second,time_series.end());
    }

    {
        std::list<utc_tp> time_series;
        std::error_code err = std::error_code();
        for(int i=0;i<11;++i)
            time_series.emplace_back(sys_days(1990y/1/1)+days(i));
        time_series.emplace_back(sys_days(1991y/1/1));
        auto time_seq = TimeSequence::make_from_range(time_series,err);
        EXPECT_EQ(time_seq.first.time_duration(),DateTimeDiff(err,std::chrono::days(1)));
        auto interval = __time_interval__(sys_days(1990y/1/1),sys_days(1990y/1/11));
        EXPECT_EQ(time_seq.first.get_interval().from(),interval.from());
        EXPECT_EQ(time_seq.first.get_interval().to(),interval.to());
        EXPECT_EQ(time_seq.second,std::prev(time_series.end()));
    }
    {
        std::list<utc_tp> time_series;
        std::error_code err = std::error_code();
        time_series.emplace_back(sys_days(1991y/1/1));
        time_series.emplace_back(sys_days(1990y/1/1));
        auto ts = TimeSequence::make_from_range(time_series,err);
        ASSERT_EQ(err,std::error_code());
        EXPECT_EQ(ts.first.time_duration(),DateTimeDiff());
        EXPECT_EQ(ts.second,++time_series.begin());
    }
    {
        std::list<utc_tp> time_series;
        std::error_code err = std::error_code();
        time_series.emplace_back(sys_days(1990y/1/1));
        time_series.emplace_back(sys_days(1990y/1/1));
        time_series.emplace_back(sys_days(1991y/1/1));
        auto time_seq = TimeSequence::make_from_range(time_series,err);
        EXPECT_EQ(time_seq.first.time_duration(),DateTimeDiff());
        auto interval = __time_interval__(sys_days(1990y/1/1),sys_days(1990y/1/1));
        EXPECT_EQ(time_seq.first.get_interval().from(),interval.from());
        EXPECT_EQ(time_seq.first.get_interval().to(),interval.to());
        EXPECT_EQ(time_seq.second,++time_series.begin());
    }
}

TEST(TimeSequence,PushTimeTest){
    TimeSequence seq(sys_days(1991y/1/1));
    std::error_code err = std::error_code();
    EXPECT_EQ(seq.get_interval(),__time_interval__(sys_days(1991y/1/1),sys_days(1991y/1/1)));
    EXPECT_EQ(seq.time_duration(),DateTimeDiff());
    seq.push_time(sys_days(1990y/1/1),err);
    EXPECT_EQ(seq.get_interval(),__time_interval__(sys_days(1990y/1/1),sys_days(1991y/1/1)));
    EXPECT_EQ(seq.time_duration(),DateTimeDiff(err,std::chrono::years(1)));
    EXPECT_FALSE(seq.push_time(sys_days(1991y/1/1),err));
    EXPECT_FALSE(seq.push_time(sys_days(1990y/1/1),err));
    EXPECT_FALSE(seq.push_time(sys_days(1990y/1/2),err));
    EXPECT_FALSE(seq.push_time(sys_days(1989y/12/31),err));
    EXPECT_FALSE(seq.push_time(sys_days(1991y/1/2),err));
    EXPECT_FALSE(seq.push_time(sys_days(1990y/12/31),err));
    EXPECT_TRUE(seq.push_time(sys_days(1989y/1/1),err));
}

TEST(TimeSequence, ExtendableTest){
    TimeSequence seq(__time_interval__(sys_days(1990y/12/30),sys_days(1990y/12/31)));
    std::vector<utc_tp> seq_rng = [](){
        std::vector<utc_tp> res;
        for(int i=1;i<10;++i){
            /* //std::cout<<"emplaced "<< */res.emplace_back(sys_days(1991y/1/i))/* <<std::endl */;
        }
        return res;}();
    std::error_code err = std::error_code();
    seq = TimeSequence(sys_days(1991y/01/10),sys_days(1991y/01/20),10,err);
    ASSERT_TRUE(err==std::error_code());
    {
        auto tmp = TimeSequence::make_from_range(seq_rng,err).first;
        ASSERT_TRUE(err==std::error_code());
        ASSERT_TRUE(seq.extendable_by(TimeSequence::make_from_range(seq_rng,err).first));
    }
    ASSERT_TRUE(TimeSequence::make_from_range(seq_rng,err).first.extendable_by(seq));
    ASSERT_FALSE(seq.extendable_by(TimeSequence(__time_interval__(sys_days(1989y/12/31),sys_days(1990y/1/1)))));
    ASSERT_FALSE(seq.extendable_by(TimeSequence(__time_interval__(sys_days(1990y/1/2),sys_days(1990y/1/10)))));
    ASSERT_FALSE(seq.extendable_by(TimeSequence(__time_interval__(sys_days(1990y/1/2),sys_days(2010y/1/10)))));
    ASSERT_FALSE(seq.extendable_by(TimeSequence(__time_interval__(sys_days(1990y/1/2)+hours(1),sys_days(2010y/1/10)))));
    ASSERT_FALSE(seq.extendable_by(TimeSequence(__time_interval__(sys_days(1990y/1/2)+hours(1),sys_days(2010y/1/10)+hours(1)))));
}

TEST(TimeSequence,ExtendByIntervalTest){
    TimeSequence seq(__time_interval__(sys_days(1990y/1/1),sys_days(1991y/1/1)));
    auto discret = seq.time_duration();
    std::error_code err = std::error_code();
    EXPECT_TRUE(seq.extend_by_interval(__time_interval__(sys_days(1989y/1/1),sys_days(1992y/1/1)),err));
    EXPECT_EQ(err,std::error_code());
    err = std::error_code();
    EXPECT_FALSE(seq.extend_by_interval(__time_interval__(sys_days(1988y/1/2),sys_days(1988y/12/31)),err));
    EXPECT_EQ(discret,seq.time_duration());
}

TEST(TimeSequence,YearMonthDiffTest){

}

int main(int argc,char* argv[]){
    testing::InitGoogleTest(&argc,argv);
    return RUN_ALL_TESTS();
}