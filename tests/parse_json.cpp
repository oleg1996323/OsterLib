#include <gtest/gtest.h>
#include "OsterLib/boost_functional/json.h"
#include "OsterLib/types/time_interval.h"
#include "OsterLib/types/coord.h"

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

TEST(JsonConversion, JsonTimePointTest){
    {
        boost::json::value result = to_json(utc_tp(sys_days(1990y/1/1)+hours(12)));
        auto time_res = from_json<utc_tp>(result);
        ASSERT_TRUE(time_res.has_value());
        ASSERT_EQ(*time_res,sys_days(1990y/1/1)+hours(12));
    }
    {
        boost::json::value result = to_json<std::chrono::seconds>(utc_tp(sys_days(1990y/1/1)+hours(12)));
        auto time_res = from_json<utc_tp>(result);
        ASSERT_TRUE(time_res.has_value());
        ASSERT_EQ(*time_res,sys_days(1990y/1/1)+hours(12));
    }
}

TEST(JsonConversion, JsonDurationTest){
    std::error_code err;
    boost::json::value result = to_json(DateTimeDiff(err,days(500),hours(12),minutes(59)));
    auto diff_res = from_json<DateTimeDiff>(result);
    ASSERT_TRUE(diff_res.has_value());
    ASSERT_EQ(*diff_res,DateTimeDiff(err,days(500),hours(12),minutes(59)));
}

TEST(JsonConversion, JsonCoordTest){
    boost::json::value result = to_json(Coord{.lat_=45.5,.lon_=51.2});
    auto coord = from_json<Coord>(result);
    ASSERT_TRUE(coord.has_value());
    ASSERT_EQ(*coord,(Coord{.lat_=45.5,.lon_=51.2}));
}

TEST(JsonConversion, JsonCoordWithBoundsTest){
    {
        boost::json::value result = to_json(Coord{.lat_=90.,.lon_=180.});
        auto coord = from_json<Coord>(result);
        ASSERT_TRUE(coord.has_value());
        ASSERT_EQ(*coord,(Coord{.lat_=90,.lon_=180}));
    }
    {
        boost::json::value result = to_json(Coord{.lat_=91,.lon_=180});
        auto coord = from_json<Coord>(result);
        ASSERT_FALSE(coord.has_value());
    }
    {
        boost::json::value result = to_json(Coord{.lat_=90,.lon_=181});
        auto coord = from_json<Coord>(result);
        ASSERT_FALSE(coord.has_value());
    }
}

TEST(JsonConversion, JsonArrayTest){
    {
        std::vector<int> arr{1,2,3,4,5,6};
        auto json_arr = to_json(arr);
        auto restored_arr = from_json<std::vector<int>>(json_arr);
        ASSERT_TRUE(restored_arr.has_value());
        ASSERT_EQ(*restored_arr,arr);
    }
    {
        std::vector<utc_tp> arr{system_clock::now(),sys_days(1990y/1/1),sys_days(2000y/12/1)};
        auto json_arr = to_json(arr);
        auto restored_arr = from_json<std::vector<utc_tp>>(json_arr);
        ASSERT_TRUE(restored_arr.has_value());
        ASSERT_EQ(*restored_arr,arr);
    }
}

TEST(JsonConversion, JsonMapTest){

}

TEST(JsonConversion, JsonSetTest){

}
#include "OsterLib/network/clientsettings.h"
TEST(JsonConversion, JsonClientSettingsTest){
    network::client::Settings set;
    set.protocol_=network::Protocol::ETHERNET;
    auto val = to_json(set);
    std::ofstream first("first.json",std::ios::out|std::ios::trunc);
    first<<val<<std::endl;
    auto resolved = from_json<network::client::Settings>(val);
    ASSERT_TRUE(resolved.has_value());
    auto& resolved_ref = resolved.value();
    std::ofstream second("second.json",std::ios::out|std::ios::trunc);
    second<<to_json(resolved_ref)<<std::endl;
    ASSERT_EQ(resolved.value(),set);
}
#include "OsterLib/network/serversettings.h"
TEST(JsonConversion, JsonServerSettingsTest){
    network::server::Settings set;
    set.protocol_=network::Protocol::ETHERNET;
    auto val = to_json(set);
    auto resolved = from_json<network::server::Settings>(val);
    ASSERT_TRUE(resolved.has_value());
    ASSERT_EQ(resolved.value(),set);
}

int main(int argc,char* argv[]){
    testing::InitGoogleTest(&argc,argv);
    return RUN_ALL_TESTS();
}