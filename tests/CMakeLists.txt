find_package(Catch2 REQUIRED)

add_executable(surma_tests test_main.cpp)

option(SURMA_COVERAGE "Enable coverage instrumentation" OFF)

if(SURMA_COVERAGE)
	target_compile_options(surma_tests PRIVATE --coverage -O0 -fno-inline)
	target_link_options(surma_tests PRIVATE --coverage)
endif()
target_link_libraries(surma_tests PRIVATE Catch2::Catch2WithMain)

include(CTest)
include(Catch)
catch_discover_tests(surma_tests)
