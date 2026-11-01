cmake_minimum_required(VERSION 3.16)
project(tests)
enable_testing()

add_executable(polygon_test polygon_test.cpp ${CMAKE_CURRENT_SOURCE_DIR}/../src/geometry/Polygon.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/geometry/algorithms.cpp)
add_executable(line_test line_test.cpp ${CMAKE_CURRENT_SOURCE_DIR}/../src/geometry/Line.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/geometry/algorithms.cpp)
add_executable(polyline_test polyline_test.cpp ${CMAKE_CURRENT_SOURCE_DIR}/../src/geometry/Polyline.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/geometry/algorithms.cpp)
target_include_directories(polyline_test PUBLIC ${COMMON_DIRS})

add_executable(matrix_test matrix_test.cpp)
add_executable(vector_test vector_test.cpp)

add_executable(bytes_test bytes.cpp)
target_link_libraries(bytes_test GTest::gtest)

add_executable(serialization_test serialization.cpp)
target_link_libraries(serialization_test GTest::gtest)
target_include_directories(serialization_test PRIVATE ${Boost_INCLUDE_DIRS})

add_executable(time_interval_test time_interval.cpp)
target_link_libraries(time_interval_test GTest::gtest types)

add_executable(time_sequence_test time_sequence.cpp)
target_link_libraries(time_sequence_test GTest::gtest types)

add_executable(parse_json_test parse_json.cpp)
target_link_libraries(parse_json_test GTest::gtest types network)

add_executable(lexical_cast_test lexical_cast.cpp)
target_link_libraries(lexical_cast_test GTest::gtest types)

add_executable(network_test network_test.cpp)
target_link_libraries(network_test network GTest::gtest)

add_executable(buffers_test buffers_test.cpp)
target_link_libraries(buffers_test network GTest::gtest)

add_executable(ctx_error_test contexted_error_test.cpp)
target_link_libraries(ctx_error_test GTest::gtest)

gtest_discover_tests(
    bytes_test
    serialization_test
    time_interval_test
    time_sequence_test
    parse_json_test
    lexical_cast_test
    network_test
    ctx_error_test
    buffers_test 
    polygon_test 
    line_test 
    polyline_test
    vector_test
    matrix_test WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
