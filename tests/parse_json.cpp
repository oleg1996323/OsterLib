#include <gtest/gtest.h>
#include "boost_functional/json.h"
#include "types/time_interval.h"

TEST(JsonParse,JsonArrayParseTest){
    {
        auto result = parse_json_from_buffer(std::string("[1,2,3]"));
        ASSERT_TRUE(result.has_value());
        ASSERT_TRUE(result->is_array());
        ASSERT_EQ(result->as_array().size(),3);
        std::vector<uint64_t> check_vals{1,2,3};
        int count = 0;
        boost::json::error err;
        for(auto& val:result->as_array()){
            ASSERT_TRUE(val.is_number());
            ASSERT_EQ(check_vals.at(count++),val.to_number<uint64_t>());
        }
    }
}

TEST(JsonParse, JsonTimePointParseTest){
    boost::json::value result = to_json<std::chrono::seconds>(utc_tp(sys_days(1990y/1/1)+hours(12)));
    auto time_res = from_json<utc_tp>(result);
    ASSERT_TRUE(time_res.has_value());
    ASSERT_EQ(*time_res,sys_days(1990y/1/1)+hours(12));
}

TEST(JsonParse, JsonDurationParseTest){
    boost::json::value result = to_json<hours>(days(500)+hours(12)+minutes(59));
    auto diff_res = from_json<utc_diff>(result);
    ASSERT_TRUE(diff_res.has_value());
    ASSERT_EQ(*diff_res,days(500)+hours(12));
}

int main(int argc,char* argv[]){
    testing::InitGoogleTest(&argc,argv);
    return RUN_ALL_TESTS();
}