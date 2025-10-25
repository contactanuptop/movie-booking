cmake_minimum_required(VERSION 3.16)
project(booking_cpp LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(booking src/BookingService.cpp)
target_include_directories(booking PUBLIC include)

add_executable(booking_cli src/main.cpp)
target_link_libraries(booking_cli PRIVATE booking)
set_target_properties(booking_cli PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

enable_testing()
add_executable(booking_tests tests/test_booking.cpp)
target_include_directories(booking_tests PRIVATE third_party include)
target_link_libraries(booking_tests PRIVATE booking)
add_test(NAME booking_unit COMMAND booking_tests)
