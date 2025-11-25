#include <gtest/gtest.h>
#include "boost_functional/json.h"

TEST(JsonParse,JsonArrayParseTest){
    auto result = parse_json(std::string("[1,2,3]"));
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

int main(int argc,char* argv[]){
    testing::InitGoogleTest(&argc,argv);
    return RUN_ALL_TESTS();
}