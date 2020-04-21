cmake_minimum_required(VERSION 3.12)
project(decimal_for_cpp)

set(CMAKE_CXX_STANDARD 11)

if(DECIMAL_FOR_CPP_BUILD_TEST)
    find_package(Boost 1.57.0 COMPONENTS system filesystem REQUIRED)
    add_executable(decimal_for_cpp_tests tests/runner.cpp tests/decimalTest.ipp)
    target_include_directories(decimal_for_cpp_tests PUBLIC ${Boost_INCLUDE_DIRS} include)
    target_link_libraries(decimal_for_cpp_tests ${Boost_LIBRARIES})
endif()