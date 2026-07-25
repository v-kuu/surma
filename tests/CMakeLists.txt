find_package(Catch2 REQUIRED)

add_executable(surma_tests test_main.cpp)
target_link_libraries(surma_tests PRIVATE Catch2::Catch2WithMain)

include(CTest)
include(Catch)
catch_discover_tests(surma_tests)
