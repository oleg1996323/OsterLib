#include <gtest/gtest.h>
#include "network/buffers/ring_buffer.h"
#include "network/buffers/vectorized_buffer.h"

TEST(Buffer,RingBufferSimple_test){
    network::RingBuffer<char> buffer(5);
    std::error_code err;
    for(int i=0;i<5;++i){
        buffer.push_back((char)i,err);
        ASSERT_EQ(err,std::error_code());
    }
    EXPECT_TRUE(buffer.full());
    auto first = buffer.data().first;
    auto second = buffer.data().second;
    auto expected = std::span<const char>("\0\1\2\3\4");
    ASSERT_EQ(first.size(),5);
    ASSERT_EQ(second.size(),0);
    for(int i = 0;i<first.size();++i)
        EXPECT_EQ(first[i],expected[i]);
    ASSERT_EQ(buffer.size(),5);
    buffer.commit_read(5);
    ASSERT_EQ(buffer.size(),0);
}

TEST(Buffer,RingBufferCheckErrors_test){
    network::RingBuffer<char> buffer(5);
    std::error_code err;
    for(int i=0;i<5;++i){
        buffer.push_back((char)i,err);
        ASSERT_EQ(err,std::error_code());
    }
    ASSERT_FALSE(buffer.push_back('\6',err));
    ASSERT_EQ(err,std::make_error_code(std::errc::no_buffer_space));
    ASSERT_TRUE(buffer.full());
    for(int i=0;i<3;++i)
        ASSERT_TRUE(buffer.pop_front());
    ASSERT_EQ(buffer.size(),2);
    auto first = buffer.data().first;
    auto second = buffer.data().second;
    auto expected_lhs = std::span<const char>("\3\4");
    ASSERT_EQ(first.size(),2);
    ASSERT_EQ(second.size(),0);
    for(int i = 0;i<first.size();++i)
        EXPECT_EQ(first[i],expected_lhs[i]);
    ASSERT_TRUE(buffer.push_back('\6',err));
    ASSERT_EQ(err,std::error_code());
    ASSERT_EQ(buffer.size(),3);
    first = buffer.data().first;
    second = buffer.data().second;
    auto expected_rhs = std::span<const char>("\6");
    for(int i = 0;i<first.size();++i)
        EXPECT_EQ(first[i],expected_lhs[i]);
    for(int i = 0;i<second.size();++i)
        EXPECT_EQ(second[i],expected_rhs[i]);
    buffer.commit_read(1);
    ASSERT_EQ(buffer.size(),2);
    buffer.commit_read(5);
    ASSERT_EQ(buffer.size(),0);
}

TEST(Buffer,VectorizedBufferSimple_test){
    network::VectorizedBuffer buffer;
    ASSERT_FALSE(buffer.has_to_write());
    auto sent = std::string("0123");
    ASSERT_EQ(sent.size(),4);
    auto vec = std::vector<char>(sent.begin(),sent.end());
    ASSERT_EQ(vec.size(),4);
    auto vec_ptr = vec.data();
    buffer.push_buffer(std::move(vec));
    ASSERT_TRUE(buffer.has_to_write());
    auto remain = buffer.remaining();
    ASSERT_NE(remain.first,nullptr);
    ASSERT_EQ(remain.first->iov_base,(void*)vec_ptr);
    ASSERT_EQ(remain.second,1);
    buffer.consume(1);
    ASSERT_TRUE(buffer.has_to_write());
    buffer.consume(100);
    ASSERT_FALSE(buffer.has_to_write());
    buffer.push_buffer(std::vector<char>());
    ASSERT_FALSE(buffer.has_to_write());
}

int main(int argc,char* argv[]){
    testing::InitGoogleTest(&argc,argv);
    return RUN_ALL_TESTS();
}