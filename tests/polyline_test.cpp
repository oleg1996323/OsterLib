#include <gtest/gtest.h>
#include "OsterLib/geometry/Polyline.h"
#include "OsterLib/types/coord.h"

using namespace geometry;

template<>
const auto& geometry::get<0,Coord>(const Coord& point){
    return point.lon_;
}

template<>
auto& geometry::get<0,Coord>(Coord& point){
    return point.lon_;
}

template<>
const auto& geometry::get<1,Coord>(const Coord& point){
    return point.lat_;
}

template<>
auto& geometry::get<1,Coord>(Coord& point){
    return point.lat_;
}

TEST(Polyline, DefaultEmpty) {
    geometry::Polyline<2,Euclid> pl;
    EXPECT_EQ(pl.number_vertices(), 0u);
    EXPECT_EQ(pl.number_segments(), 0u);
    EXPECT_EQ(pl.length(), 0.0);
}

TEST(Polyline, InitializerList) {
    geometry::Polyline<2,Euclid> pl{{0,0}, {3,0}, {3,4}};
    EXPECT_EQ(pl.number_vertices(), 3u);
    EXPECT_EQ(pl.number_segments(), 2u);
    EXPECT_NEAR(pl.length(), 7.0, 1e-9);   // 3 + 4
}

TEST(Polyline, RangeFromVector) {
    std::vector<geometry::BasicPoint<2,Euclid>> pts{{0,0}, {1,0}, {1,1}};
    geometry::Polyline<2,Euclid> pl(pts);
    EXPECT_EQ(pl.number_vertices(), 3u);
    EXPECT_NEAR(pl.length(), 2.0, 1e-9);
}

TEST(Polyline, RangeMoveFromVector) {
    std::vector<geometry::BasicPoint<2,Euclid>> pts{{0,0}, {1,0}, {1,1}};
    geometry::Polyline<2,Euclid> pl(std::move(pts));
    EXPECT_EQ(pl.number_vertices(), 3u);
    EXPECT_NEAR(pl.length(), 2.0, 1e-9);
}

TEST(Polyline, ThreeDimensional) {
    geometry::Polyline<3,Euclid> pl{{0,0,0}, {1,0,0}, {1,1,0}, {1,1,1}};
    EXPECT_EQ(pl.number_vertices(), 4u);
    EXPECT_NEAR(pl.length(), 3.0, 1e-9);
}

TEST(Polyline, CopyConstructor) {
    geometry::Polyline<2,Euclid> a{{0,0}, {3,0}, {3,4}};
    a.length();                              // заполнить кэш
    geometry::Polyline<2,Euclid> b(a);
    EXPECT_EQ(b.number_vertices(), 3u);
    EXPECT_NEAR(b.length(), 7.0, 1e-9);
    EXPECT_EQ(a.number_vertices(), 3u);      // a не изменён
}

TEST(Polyline, MoveConstructor) {
    geometry::Polyline<2,Euclid> a{{0,0}, {3,0}, {3,4}};
    a.length();
    geometry::Polyline<2,Euclid> b(std::move(a));
    EXPECT_EQ(b.number_vertices(), 3u);
    EXPECT_NEAR(b.length(), 7.0, 1e-9);
}

TEST(Polyline, CopyAssignment) {
    geometry::Polyline<2,Euclid> a{{0,0}, {1,0}};
    geometry::Polyline<2,Euclid> b{{5,5}};
    b = a;
    EXPECT_EQ(b.number_vertices(), 2u);
    EXPECT_NEAR(b.length(), 1.0, 1e-9);
}

TEST(Polyline, SelfAssignment) {
    geometry::Polyline<2,Euclid> a{{0,0}, {3,0}};
    a = a;
    EXPECT_EQ(a.number_vertices(), 2u);
    EXPECT_NEAR(a.length(), 3.0, 1e-9);
}

TEST(Polyline, AppendGrowsLength) {
    geometry::Polyline<2,Euclid> pl;
    pl.append(std::array<double,2>{0,0});
    EXPECT_EQ(pl.number_vertices(), 1u);
    pl.append(std::array<double,2>{3,0});
    EXPECT_EQ(pl.number_vertices(), 2u);
    EXPECT_NEAR(pl.length(), 3.0, 1e-9);
    pl.append(std::array<double,2>{3,4});
    EXPECT_EQ(pl.number_vertices(), 3u);
    EXPECT_NEAR(pl.length(), 7.0, 1e-9);
}

TEST(Polyline, AppendInvalidatesLengthCache) {
    geometry::Polyline<2,Euclid> pl{{0,0}, {3,0}};
    EXPECT_NEAR(pl.length(), 3.0, 1e-9);     // кэш = 3
    pl.append(std::array<double,2>{3,4});    // длина теперь 7
    EXPECT_NEAR(pl.length(), 7.0, 1e-9);     // ⚠️ должно быть 7, не 3
}

TEST(Polyline, AppendDuplicateAtBackDoesNotChangeLength) {
    geometry::Polyline<2,Euclid> pl{{0,0}, {3,0}};
    pl.append(std::array<double,2>{3,0});    // дубликат последней
    EXPECT_NEAR(pl.length(), 3.0, 1e-9);
}

TEST(Polyline, AppendFirstPointAgain_ClosedLoop) {
    // ⚠️ ВНИМАНИЕ: с текущим `&&` в append это НЕ инвалидирует кэш,
    // но должно (добавляется сегмент обратно к началу)
    geometry::Polyline<2,Euclid> pl{{0,0}, {3,0}, {3,4}};
    EXPECT_NEAR(pl.length(), 7.0, 1e-9);
    pl.append(std::array<double,2>{0,0});    // замкнули
    EXPECT_NEAR(pl.length(), 12.0, 1e-9);    // 7 + 5
}

TEST(Polyline, InsertAtBeginning) {
    geometry::Polyline<2,Euclid> pl{{3,0}, {3,4}};
    pl.insert(0, geometry::Point<2,Euclid>{{0,0}});
    EXPECT_EQ(pl.number_vertices(), 3u);
    EXPECT_NEAR(pl.length(), 7.0, 1e-9);     // 3 + 4
}

TEST(Polyline, InsertInMiddle) {
    geometry::Polyline<2,Euclid> pl{{0,0}, {3,4}};
    pl.insert(1, geometry::Point<2,Euclid>{{3,0}});
    EXPECT_EQ(pl.number_vertices(), 3u);
    EXPECT_NEAR(pl.length(), 7.0, 1e-9);     // 3 + 4
}

TEST(Polyline, InsertAtEnd) {
    geometry::Polyline<2,Euclid> pl{{0,0}, {3,0}};
    pl.insert(2, geometry::Point<2,Euclid>{{3,4}}); // >= size() → append
    EXPECT_EQ(pl.number_vertices(), 3u);
    EXPECT_NEAR(pl.length(), 7.0, 1e-9);
}

TEST(Polyline, InsertInvalidatesLengthCache) {
    geometry::Polyline<2,Euclid> pl{{0,0}, {3,4}};
    EXPECT_NEAR(pl.length(), 5.0, 1e-9);
    pl.insert(1, geometry::Point<2,Euclid>{{3,0}});
    EXPECT_NEAR(pl.length(), 7.0, 1e-9);     // ⚠️ должно быть 7
}

TEST(Polyline, EraseMiddle) {
    geometry::Polyline<2,Euclid> pl{{0,0}, {3,0}, {3,4}};
    EXPECT_NEAR(pl.length(), 7.0, 1e-9);
    pl.erase(1);
    EXPECT_EQ(pl.number_vertices(), 2u);
    EXPECT_NEAR(pl.length(), 5.0, 1e-9);     // 0,0 → 3,4 = 5
}

TEST(Polyline, EraseOutOfRangeUsesLast) {
    geometry::Polyline<2,Euclid> pl{{0,0}, {3,0}, {3,4}};
    pl.erase(999);                            // клампится к size-1
    EXPECT_EQ(pl.number_vertices(), 2u);
}

TEST(Polyline, EraseEmptyIsNoop) {
    geometry::Polyline<2,Euclid> pl;
    EXPECT_NO_THROW(pl.erase(0));
    EXPECT_EQ(pl.number_vertices(), 0u);
}

TEST(Polyline, EraseAllOneByOne) {
    geometry::Polyline<2,Euclid> pl{{0,0}, {1,0}, {2,0}};
    pl.erase(0);
    pl.erase(0);
    pl.erase(0);
    EXPECT_EQ(pl.number_vertices(), 0u);
    EXPECT_EQ(pl.length(), 0.0);
}

TEST(Polyline, LengthEmpty){ 
    EXPECT_EQ((geometry::Polyline<2,Euclid>().length()), 0.0);
}
TEST(Polyline, LengthOnePoint){
    EXPECT_EQ((geometry::Polyline<2,Euclid>{{1,2}}.length()), 0.0);
}
TEST(Polyline, LengthTwoPoints){ 
    EXPECT_NEAR((geometry::Polyline<2,Euclid>{{0,0},{3,4}}.length()), 5.0, 1e-9); 
}
TEST(Polyline, LengthCached) {
    geometry::Polyline<2,Euclid> pl{{0,0},{3,0}};
    double l1 = pl.length();
    double l2 = pl.length();                 // должен взять из кэша
    EXPECT_EQ(l1, l2);
}
TEST(Polyline, LengthClosedLoop) {
    geometry::Polyline<2,Euclid> pl{{0,0},{3,0},{3,4},{0,0}};
    EXPECT_NEAR(pl.length(), 12.0, 1e-9);    // 3+4+5
}
TEST(Polyline, LengthPrecision_LongDouble) {
    geometry::Polyline<2, Euclid, long double> pl{{0,0},{1,0},{1,1},{0,1},{0,0}};
    EXPECT_NEAR(static_cast<double>(pl.length()), 4.0, 1e-9);
}
TEST(Polyline, FreeLengthFunction) {
    geometry::Polyline<2,Euclid> pl{{0,0},{3,4}};
    EXPECT_NEAR(geometry::length(pl), 5.0, 1e-9);
}

TEST(Polyline, IntersectionsWithLine_None) {
    geometry::Polyline<2,Euclid> pl{{0,0},{1,0},{2,0}};
    geometry::BasicLine<2,Euclid> line({0,5},{2,5});
    auto is = geometry::intersections(pl, line);
    EXPECT_TRUE(is.empty());
}

TEST(Polyline, IntersectionsWithLine_One) {
    geometry::Polyline<2,Euclid> pl{{0,0},{2,0}};
    geometry::BasicLine<2,Euclid> line({1,-1},{1,1});    // вертикальная через x=1
    auto is = geometry::intersections(pl, line);
    ASSERT_EQ(is.size(), 1u);
    EXPECT_EQ(is[0].first_elem_id(), 0u);
    EXPECT_NEAR(is[0].point().template get<0>(), 1.0, 1e-9);
    EXPECT_NEAR(is[0].point().template get<1>(), 0.0, 1e-9);
}

TEST(Polyline, IntersectionsWithLine_TwoOnDifferentSegments) {
    geometry::Polyline<2,Euclid> pl{{0,0},{2,0},{2,2},{0,2}};
    geometry::BasicLine<2,Euclid> line({1,-1},{1,5});    // вертикальная x=1
    auto is = geometry::intersections(pl, line);
    ASSERT_EQ(is.size(), 2u);
    EXPECT_NEAR(is[0].point().template get<1>(), 0.0, 1e-9);
    EXPECT_NEAR(is[1].point().template get<1>(), 2.0, 1e-9);
}

TEST(Polyline, IntersectionsWithLine_AtVertex) {
    geometry::Polyline<2,Euclid> pl{{0,0},{1,1},{2,0}};
    geometry::BasicLine<2,Euclid> line({0,1},{2,1});    // горизонтальная y=1
    auto is = geometry::intersections(pl, line);
    ASSERT_EQ(is.size(), 1u);
    EXPECT_NEAR(is[0].point().template get<0>(), 1.0, 1e-9);
}

TEST(Polyline, IntersectionsPLxPL_None) {
    geometry::Polyline<2,Euclid> a{{0,0},{1,0}};
    geometry::Polyline<2,Euclid> b{{0,5},{1,5}};
    EXPECT_TRUE(geometry::intersections(a,b).empty());
}

TEST(Polyline, IntersectionsPLxPL_One) {
    geometry::Polyline<2,Euclid> a{{0,0},{2,0}};
    geometry::Polyline<2,Euclid> b{{1,-1},{1,1}};
    auto is = geometry::intersections(a,b);
    ASSERT_EQ(is.size(), 1u);
    EXPECT_NEAR(is[0].point().template get<0>(), 1.0, 1e-9);
}

TEST(Polyline, IntersectionsPLxPL_TwoSegments) {
    // зигзаг × горизонтальная
    geometry::Polyline<2,Euclid> a{{0,0},{1,2},{2,0},{3,2},{4,0}};
    geometry::Polyline<2,Euclid> b{{-1,1},{5,1}};
    auto is = geometry::intersections(a,b);
    // 4 пересечения (по одному на каждый отрезок "зигзага", кроме нулевого)
    EXPECT_EQ(is.size(), 4u);
}

TEST(Polyline, Equivalent_Same) {
    geometry::Polyline<2,Euclid> a{{0,0},{1,0},{1,1}};
    geometry::Polyline<2,Euclid> b{{0,0},{1,0},{1,1}};
    EXPECT_TRUE(geometry::equivalent(a,b));
}

TEST(Polyline, Equivalent_DifferentPointCount) {
    geometry::Polyline<2,Euclid> a{{0,0},{1,0}};
    geometry::Polyline<2,Euclid> b{{0,0},{1,0},{1,1}};
    EXPECT_FALSE(geometry::equivalent(a,b));
}

TEST(Polyline, Equivalent_DifferentGeometry) {
    geometry::Polyline<2,Euclid> a{{0,0},{1,0}};
    geometry::Polyline<2,Euclid> b{{0,0},{2,0}};
    EXPECT_FALSE(geometry::equivalent(a,b));
}

TEST(Polyline, Equivalent_CyclicShift) {
    // ⚠️ проверить, работает ли (в реализации ищется first.vertices().front() в second)
    geometry::Polyline<2,Euclid> a{{0,0},{1,0},{1,1},{0,1}};
    geometry::Polyline<2,Euclid> b{{1,0},{1,1},{0,1},{0,0}};
    EXPECT_TRUE(geometry::equivalent(a,b));  // возможно false — зависит от реализации
}

TEST(Polyline, CutByLine_Simple) {
    // вертикальная полилиния разрезается горизонтальной
    geometry::Polyline<2,Euclid> pl{{0,0},{0,10}};
    geometry::BasicLine<2,Euclid> line({-5,5},{5,5});
    auto result = pl.cut(line, true);
    // ожидается 2 части: [0,0]→[0,5] и [0,5]→[0,10]
    EXPECT_EQ(result.size(), 2u);
    print(result);
}

TEST(Polyline, CutByLine_NoIntersection) {
    geometry::Polyline<2,Euclid> pl{{0,0},{0,2}};
    geometry::BasicLine<2,Euclid> line({-5,5},{5,5});
    auto result = pl.cut(line, true);
    EXPECT_TRUE(result.empty());
}

TEST(Polyline, CutByPolyline_Cross) {
    geometry::Polyline<2,Euclid> a{{0,0},{10,0}};
    geometry::Polyline<2,Euclid> b{{5,-5},{5,5}};
    auto result = a.cut(b, true);
    EXPECT_EQ(result.size(), 2u);
}

TEST(Polyline, AllDuplicatePoints) {
    geometry::Polyline<2,Euclid> pl{{1,1},{1,1},{1,1}};
    EXPECT_NEAR(pl.length(), 0.0, 1e-9);
    EXPECT_EQ(pl.number_segments(), 2u);   // 3 точки — 2 «нулевых» сегмента
}

TEST(Polyline, ZeroLengthSegments) {
    geometry::Polyline<2,Euclid> pl{{0,0},{0,0},{1,0}};
    EXPECT_NEAR(pl.length(), 1.0, 1e-9);
}

TEST(Polyline, PrintDoesNotCrash) {
    geometry::Polyline<2,Euclid> pl{{0,0},{1,0},{1,1}};
    EXPECT_NO_THROW(geometry::print(pl));
    EXPECT_NO_THROW(geometry::print(geometry::Polyline<2,Euclid>{}));
}

TEST(Polyline, PrintVectorOfPolylines) {
    std::vector<geometry::Polyline<2,Euclid>> v{Polyline<2,Euclid>{{0,0},{1,0}}, Polyline<2,Euclid>{{2,2},{3,3}}};
    EXPECT_NO_THROW(geometry::print(v));
}

TEST(polyline,cut_test){
    Polyline<2,Euclid>(std::vector<Coord>());
    Polyline<2,Euclid> pl;
    pl.append(Coord{.lat_=0,.lon_=0});
    pl.append({1,1});
    pl.append(Coord{.lat_=4,.lon_=2});
    pl.append(Coord{.lat_=-2,.lon_=4});
    pl.append(Coord{.lat_=2,.lon_=6});
    pl.append(Coord{.lat_=4,.lon_=9});
    Polyline<2,Euclid> cut_pl;
    cut_pl.append(Coord{.lat_=-1,.lon_=1});
    cut_pl.append(Coord{.lat_=6,.lon_=2});
    cut_pl.append(Coord{.lat_=4,.lon_=2.5});
    cut_pl.append(Coord{.lat_=1,.lon_=2.5});
    cut_pl.append(Coord{.lat_=1,.lon_=6});
    cut_pl.append(Coord{.lat_=5,.lon_=9});
    cut_pl.append(Coord{.lat_=-2,.lon_=2});
    geometry::print(pl);
    BasicLine<2,Euclid> line(Point<Euclid,2>(6,2),Point<2,Euclid>(9,3));
    auto polylines = pl.cut(line,true);
    geometry::print(polylines);
}

int main(int argc, char* argv[]){
    testing::InitGoogleTest(&argc,argv);
    return RUN_ALL_TESTS();
}