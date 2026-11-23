#include "OsterLib/geometry/Polygon.h"
#include "OsterLib/geometry/Point.h"
#include <gtest/gtest.h>

using namespace geometry;

TEST(OsterLib_polygon,cut_test_counter){
    {
        Line line = {
            Point<Euclid,2>{7,21},
            Point<Euclid,2>{7,-1}
        };
        {           
            {
                Polygon<2,Euclid> pg;
                pg.append({0,0});
                pg.append({0,10});
                pg.append({3,10});
                pg.append({3,5});
                pg.append({5.5,5});
                pg.append({5.5,10});
                pg.append({5,10});
                pg.append({5,6});
                pg.append({4,6});
                pg.append({4,11});
                pg.append({7,11});
                pg.append({9,10});
                pg.append({9,6.5});
                pg.append({10,6.5});
                pg.append({10,8});
                pg.append({10.5,8});
                pg.append({10.5,6});
                pg.append({8,6});
                pg.append({8,9});
                pg.append({5.75,9});
                pg.append({5.75,8});
                pg.append({6.75,8});
                pg.append({6.75,6.75});
                pg.append({6.5,6.75});
                pg.append({6.5,7.5});
                pg.append({6,7.5});
                pg.append({6,6.25});
                pg.append({6.25,6.25});
                pg.append({6.25,7.25});
                pg.append({6.375,7.25});
                pg.append({6.375,6.5});
                pg.append({7,6.5});
                pg.append({7,3});
                pg.append({8,3});
                pg.append({12,7});
                pg.append({12,10});
                pg.append({16,10});
                pg.append({16,5});
                pg.append({18,5});
                pg.append({18,10});
                pg.append({20,10});
                pg.append({20,3});
                pg.append({14,3});
                pg.append({14,8});
                pg.append({13,8});
                pg.append({13,0});
                pg.append({0,0});
                ASSERT_TRUE(pg.is_valid());
                //print(pg);
                std::cout<<std::endl;
                {
                    std::cout<<"test #1"<<std::endl;
                    auto res = pg.clip(line, true);
                    ASSERT_FALSE(res.empty());
                    for(const Polygon<2,Euclid>& polyg:res)
                        print(polyg);
                    std::vector<Polygon<2,Euclid>> etalon = {
                        [](){
                            Polygon<2,Euclid> result;
                            result.append({9,7});
                            result.append({9,10});
                            result.append({7,11});
                            result.append({4,11});
                            result.append({4,7});
                            result.append({5,7});
                            result.append({5,10});
                            result.append({5.5,10});
                            result.append({5.5,7});
                            result.append({6,7});
                            result.append({6,7.5});
                            result.append({6.5,7.5});
                            result.append({6.5,7});
                            result.append({6.75,7});
                            result.append({6.75,8});
                            result.append({5.75,8});
                            result.append({5.75,9});
                            result.append({8,9});
                            result.append({8,7});
                            result.append({9,7});
                            return result;
                        }(),
                        [](){
                            Polygon<2,Euclid> result;
                            result.append({6.375,7});
                            result.append({6.375,7.25});
                            result.append({6.25,7.25});
                            result.append({6.25,7});
                            result.append({6.375,7});
                            return result;
                        }(),
                        [](){
                            Polygon<2,Euclid> result;
                            result.append({10.5,7});
                            result.append({10.5,8});
                            result.append({10,8});
                            result.append({10,7});
                            result.append({10.5,7});
                            return result;
                        }(),
                        [](){
                            Polygon<2,Euclid> result;
                            result.append({16,7});
                            result.append({16,10});
                            result.append({12,10});
                            result.append({12,7});
                            result.append({13,7});
                            result.append({13,8});
                            result.append({14,8});
                            result.append({14,7});
                            result.append({16,7});
                            return result;
                        }(),
                        [](){
                            Polygon<2,Euclid> result;
                            result.append({20,7});
                            result.append({20,10});
                            result.append({18,10});
                            result.append({18,7});
                            result.append({20,7});
                            return result;
                        }(),
                        [](){
                            Polygon<2,Euclid> result;
                            result.append({3,7});
                            result.append({3,10});
                            result.append({0,10});
                            result.append({0,7});
                            result.append({3,7});
                            return result;
                        }()
                    };
                    int counter = 0;
                    for(auto& new_pg:res){
                        if(auto found = std::find_if(etalon.begin(),etalon.end(),
                            [&new_pg](const Polygon<2,Euclid>& pg){
                            return pg.equivalent(new_pg,false);
                        });found==etalon.end()){
                            std::cout<<"polygon №: "<<counter<<std::endl;
                            //assert(bg::is_valid(new_pg));
                            print(new_pg);
                            EXPECT_TRUE(false);
                        }
                        else EXPECT_TRUE(true);
                        ++counter;
                    }
                }
                {
                    std::cout<<"test #2"<<std::endl;
                    auto res = pg.clip(line, false);
                    ASSERT_FALSE(res.empty());
                    for(const Polygon<2,Euclid>& polyg:res)
                        print(polyg);
                    std::vector<Polygon<2,Euclid>> etalon = {
                        [](){
                            Polygon<2,Euclid> result;
                            result.append({20,7});
                            result.append({20,3});
                            result.append({14,3});
                            result.append({14,7});
                            result.append({16,7});
                            result.append({16,5});
                            result.append({18,5});
                            result.append({18,7});
                            result.append({20,7});
                            return result;
                        }(),
                        [](){
                            Polygon<2,Euclid> result;
                            result.append({10.5,7});
                            result.append({10.5,6});
                            result.append({8,6});
                            result.append({8,7});
                            result.append({9,7});
                            result.append({9,6.5});
                            result.append({10,6.5});
                            result.append({10,7});
                            result.append({10.5,7});
                            return result;
                        }(),
                        [](){
                            Polygon<2,Euclid> result;
                            result.append({6.75,6.75});
                            result.append({6.5,6.75});
                            result.append({6.5,7});
                            result.append({6.75,7});
                            result.append({6.75,6.75});
                            return result;
                        }(),
                        [](){
                            Polygon<2,Euclid> result;
                            result.append({5,6});
                            result.append({4,6});
                            result.append({4,7});
                            result.append({5,7});
                            result.append({5,6});
                            return result;
                        }(),
                        [](){
                            Polygon<2,Euclid> result;
                            result.append({0,0});
                            result.append({0,7});
                            result.append({3,7});
                            result.append({3,5});
                            result.append({5.5,5});
                            result.append({5.5,7});
                            result.append({6,7});
                            result.append({6,6.25});
                            result.append({6.25,6.25});
                            result.append({6.25,7});
                            result.append({6.375,7});
                            result.append({6.375,6.5});
                            result.append({7,6.5});
                            result.append({7,3});
                            result.append({8,3});
                            result.append({12,7});
                            result.append({13,7});
                            result.append({13,0});
                            result.append({0,0});
                            return result;
                        }()
                    };
                    int counter = 0;
                    for(auto& new_pg:res){
                        if(auto found = std::find_if(etalon.begin(),etalon.end(),
                            [&new_pg](const Polygon<2,Euclid>& pg){
                            return pg.equivalent(new_pg,false);
                        });found==etalon.end()){
                            std::cout<<"polygon №: "<<counter<<std::endl;
                            print(new_pg);
                            EXPECT_TRUE(false);
                        }
                        else EXPECT_TRUE(true);
                        ++counter;
                    }
                }
            }
            //////////////////////////////////////////////////////////////////////////////////
            //////////////////////////////////////////////////////////////////////////////////
            /////////////CLOCKWISE////////////////////////////////////////////////////////////
            //////////////////////////////////////////////////////////////////////////////////
            //////////////////////////////////////////////////////////////////////////////////
            {
                Polygon<2,Euclid> pg;
                pg.append({0,0});
                pg.append({0,10});
                pg.append({3,10});
                pg.append({3,5});
                pg.append({5.5,5});
                pg.append({5.5,10});
                pg.append({5,10});
                pg.append({5,6});
                pg.append({4,6});
                pg.append({4,11});
                pg.append({7,11});
                pg.append({9,10});
                pg.append({9,6.5});
                pg.append({10,6.5});
                pg.append({10,8});
                pg.append({10.5,8});
                pg.append({10.5,6});
                pg.append({8,6});
                pg.append({8,9});
                pg.append({5.75,9});
                pg.append({5.75,8});
                pg.append({6.75,8});
                pg.append({6.75,6.75});
                pg.append({6.5,6.75});
                pg.append({6.5,7.5});
                pg.append({6,7.5});
                pg.append({6,6.25});
                pg.append({6.25,6.25});
                pg.append({6.25,7.25});
                pg.append({6.375,7.25});
                pg.append({6.375,6.5});
                pg.append({7,6.5});
                pg.append({7,3});
                pg.append({8,3});
                pg.append({12,7});
                pg.append({12,10});
                pg.append({16,10});
                pg.append({16,5});
                pg.append({18,5});
                pg.append({18,10});
                pg.append({20,10});
                pg.append({20,3});
                pg.append({14,3});
                pg.append({14,8});
                pg.append({13,8});
                pg.append({13,0});
                pg.append({0,0});
                ASSERT_TRUE(pg.is_valid());
                //print(pg);
                {
                    std::cout<<"test #3"<<std::endl;
                    auto res = pg.clip(line,true);
                    ASSERT_FALSE(res.empty());
                    for(const Polygon<2,Euclid>& polyg:res)
                        print(polyg);
                    std::vector<Polygon<2,Euclid>> etalon = {
                        [](){
                            Polygon<2,Euclid> result;
                            result.append({9,7});
                            result.append({9,10});
                            result.append({7,11});
                            result.append({4,11});
                            result.append({4,7});
                            result.append({5,7});
                            result.append({5,10});
                            result.append({5.5,10});
                            result.append({5.5,7});
                            result.append({6,7});
                            result.append({6,7.5});
                            result.append({6.5,7.5});
                            result.append({6.5,7});
                            result.append({6.75,7});
                            result.append({6.75,8});
                            result.append({5.75,8});
                            result.append({5.75,9});
                            result.append({8,9});
                            result.append({8,7});
                            result.append({9,7});
                            return result;
                        }(),
                        [](){
                            Polygon<2,Euclid> result;
                            result.append({6.375,7});
                            result.append({6.375,7.25});
                            result.append({6.25,7.25});
                            result.append({6.25,7});
                            result.append({6.375,7});
                            return result;
                        }(),
                        [](){
                            Polygon<2,Euclid> result;
                            result.append({10.5,7});
                            result.append({10.5,8});
                            result.append({10,8});
                            result.append({10,7});
                            result.append({10.5,7});
                            return result;
                        }(),
                        [](){
                            Polygon<2,Euclid> result;
                            result.append({16,7});
                            result.append({16,10});
                            result.append({12,10});
                            result.append({12,7});
                            result.append({13,7});
                            result.append({13,8});
                            result.append({14,8});
                            result.append({14,7});
                            result.append({16,7});
                            return result;
                        }(),
                        [](){
                            Polygon<2,Euclid> result;
                            result.append({20,7});
                            result.append({20,10});
                            result.append({18,10});
                            result.append({18,7});
                            result.append({20,7});
                            return result;
                        }(),
                        [](){
                            Polygon<2,Euclid> result;
                            result.append({3,7});
                            result.append({3,10});
                            result.append({0,10});
                            result.append({0,7});
                            result.append({3,7});
                            return result;
                        }()
                    };
                    int counter = 0;
                    for(auto& new_pg:res){
                        if(auto found = std::find_if(etalon.begin(),etalon.end(),
                            [&new_pg](const Polygon<2,Euclid>& pg){
                            return pg.equivalent(new_pg,false);
                        });found==etalon.end()){
                            std::cout<<"polygon №: "<<counter<<std::endl;
                            print(new_pg);
                            EXPECT_TRUE(false);
                        }
                        else EXPECT_TRUE(true);
                        ++counter;
                    }
                }
                {
                    std::cout<<"test #4"<<std::endl;
                    auto res = pg.clip(line, false);
                    ASSERT_FALSE(res.empty());
                    for(const Polygon<2,Euclid>& polyg:res)
                        print(polyg);
                    std::vector<Polygon<2,Euclid>> etalon = {
                        [](){
                            Polygon<2,Euclid> result;
                            result.append({20,7});
                            result.append({20,3});
                            result.append({14,3});
                            result.append({14,7});
                            result.append({16,7});
                            result.append({16,5});
                            result.append({18,5});
                            result.append({18,7});
                            result.append({20,7});
                            return result;
                        }(),
                        [](){
                            Polygon<2,Euclid> result;
                            result.append({10.5,7});
                            result.append({10.5,6});
                            result.append({8,6});
                            result.append({8,7});
                            result.append({9,7});
                            result.append({9,6.5});
                            result.append({10,6.5});
                            result.append({10,7});
                            result.append({10.5,7});
                            return result;
                        }(),
                        [](){
                            Polygon<2,Euclid> result;
                            result.append({6.75,6.75});
                            result.append({6.5,6.75});
                            result.append({6.5,7});
                            result.append({6.75,7});
                            result.append({6.75,6.75});
                            return result;
                        }(),
                        [](){
                            Polygon<2,Euclid> result;
                            result.append({5,7});
                            result.append({5,6});
                            result.append({4,6});
                            result.append({4,7});
                            result.append({5,7});
                            return result;
                        }(),
                        [](){
                            Polygon<2,Euclid> result;
                            result.append({0,0});
                            result.append({0,7});
                            result.append({3,7});
                            result.append({3,5});
                            result.append({5.5,5});
                            result.append({5.5,7});
                            result.append({6,7});
                            result.append({6,6.25});
                            result.append({6.25,6.25});
                            result.append({6.25,7});
                            result.append({6.375,7});
                            result.append({6.375,6.5});
                            result.append({7,6.5});
                            result.append({7,3});
                            result.append({8,3});
                            result.append({12,7});
                            result.append({13,7});
                            result.append({13,0});
                            result.append({0,0});
                            return result;
                        }()
                    };
                    int counter = 0;
                    for(auto& new_pg:res){
                        if(auto found = std::find_if(etalon.begin(),etalon.end(),
                            [&new_pg](const Polygon<2,Euclid>& pg){
                            return pg.equivalent(new_pg,false);
                        });found==etalon.end()){
                            std::cout<<"polygon №: "<<counter<<std::endl;
                            print(new_pg);
                            EXPECT_TRUE(false);
                        }
                        else EXPECT_TRUE(true);
                        ++counter;
                    }
                }
            }
        }
    }
    {
        {
            Line<2,Euclid> polyline = {
                {0, 0},
                {55, 55}
            };
            Polygon<2,Euclid> pg;
            pg.append({0,0});
            pg.append({0,2});
            pg.append({2,2});
            pg.append({2,0});
            pg.append({0,0});
            std::cout<<"test #5"<<std::endl;
            auto res = pg.clip(polyline, false);
            for(const Polygon<2,Euclid>& polyg:res)
                print(polyg);
            std::vector<Polygon<2,Euclid>> etalon = {
                [](){
                    Polygon<2,Euclid> result;
                    result.append({2,2});
                    result.append({0,2});
                    result.append({0,0});
                    result.append({2,2});
                    return result;
                }()
            };
            int counter = 0;
            for(auto& new_pg:res){
                if(auto found = std::find_if(etalon.begin(),etalon.end(),
                    [&new_pg](const Polygon<2,Euclid>& pg){
                    return pg.equivalent(new_pg,false);
                });found==etalon.end()){
                    std::cout<<"polygon №: "<<counter<<std::endl;
                    print(new_pg);
                    EXPECT_TRUE(false);
                }
                else EXPECT_TRUE(true);
                ++counter;
            }
        }
    }
}

TEST(polygon,is_valid_test){
    {
        Polygon<2,Euclid> pg;
        pg.append({0, 0});  // A
        pg.append({0, 2});  // B
        pg.append({2, 0});  // C
        pg.append({2, 2});  // D
        pg.append({0, 0});  // A — close сработал, ring замкнут
        pg.append({0, 0});  // A — close сработал ещё раз, получили [A,B,C,D,A,A]
        ASSERT_TRUE(!pg.is_valid());     // ← упадёт
    }
    {
        Polygon<2,Euclid> pg;
        pg.append({0, 0});  // A
        pg.append({0, 2});  // B
        pg.append({2, 2});  // C
        pg.append({2, 2});  // C
        pg.append({2, 0});  // D
        pg.append({1, 0});  // E
        pg.append({0.5, 0});  // E
        pg.append({0, 0});  // A — close сработал, ring замкнут
        pg.append({0, 0});  // A — close сработал ещё раз, получили [A,B,C,D,A,A]
        ASSERT_TRUE(pg.is_valid());     // ← упадёт
    }
    {
        Polygon<2,Euclid> pg;
        pg.append({0, 0});  // A
        pg.append({0, 2});  // B
        pg.append({2, 0});  // C
        pg.append({2, 2});  // C
        pg.append({0, 0});  // A
        ASSERT_TRUE(!pg.is_valid());
    }
}

int main(int argc,char* argv[]){
    testing::InitGoogleTest(&argc,argv);
    return RUN_ALL_TESTS();
}
