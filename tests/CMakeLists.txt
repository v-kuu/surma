find_package(Catch2 REQUIRED)

add_executable(surma_tests
	test_surma.cpp
)

target_link_libraries(surma_tests PRIVATE
	surma_lib
	Catch2::Catch2WithMain
)

option(SURMA_COVERAGE "Enable coverage instrumentation" OFF)

if(SURMA_COVERAGE)
	target_compile_options(surma_tests PRIVATE --coverage -O0 -fno-inline)
	target_link_options(surma_tests PRIVATE --coverage)
endif()

if(SURMA_SANITIZER)
	target_compile_options(surma_tests PRIVATE
		-fsanitize=${SURMA_SANITIZER}
		-fno-omit-frame-pointer
	)
	target_link_options(surma_tests PRIVATE
		-fsanitize=${SURMA_SANITIZER}
	)
endif()

include(CTest)
include(Catch)
catch_discover_tests(surma_tests)
