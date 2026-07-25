cmake_minimum_required(VERSION 3.28)
project(surma LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(CheckLanguage)
check_language(CUDA)
if(CMAKE_CUDA_COMPILER)
	enable_language(CUDA)
	set(CMAKE_CUDA_STANDARD 20)
	set(CMAKE_CUDA_STANDARD_REQUIRED ON)
	message(STATUS "CUDA enabled: ${CMAKE_CUDA_COMPILER_VERSION}")
else()
	message(STATUS "CUDA not found, GPU support disabled")
endif()

find_package(spdlog REQUIRED)

add_executable(surma src/main.cpp)
target_link_libraries(surma PRIVATE spdlog::spdlog)
target_compile_options(surma PRIVATE
	$<$<COMPILE_LANGUAGE:CXX>:-Wall -Wextra -Werror>
	$<$<COMPILE_LANGUAGE:CUDA>:-Xcompiler=-Wall,-Wextra-Werror>
)

enable_testing()
add_subdirectory(tests)
