cmake_minimum_required(VERSION 3.28)
project(surma LANGUAGES CXX)

set(CMAKE_CXX_EXTENSIONS OFF)
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
find_package(PkgConfig REQUIRED)
pkg_check_modules(XDP REQUIRED libxdp)
pkg_check_modules(BPF REQUIRED libbpf)

add_library(surma_lib STATIC
	src/Pipeline.cpp
	src/capture/Umem.cpp
	src/capture/Socket.cpp
	src/capture/XdpProgram.cpp
	src/capture/EpollFd.cpp
	src/capture/RxLoop.cpp
	src/processing/ProcessingThread.cpp
	src/processing/PacketParser.cpp
)
target_include_directories(surma_lib PRIVATE ${XDP_INCLUDE_DIRS})
target_include_directories(surma_lib PUBLIC src/)

target_link_libraries(surma_lib PRIVATE
	spdlog::spdlog
	${XDP_LIBRARIES}
	${BPF_LIBRARIES}
)
target_compile_options(surma_lib PRIVATE
	$<$<COMPILE_LANGUAGE:CXX>:-Wall -Wextra -Werror>
	$<$<COMPILE_LANGUAGE:CUDA>:-Xcompiler=-Wall,-Wextra,-Werror>
)

add_executable(surma src/main.cpp)
target_link_libraries(surma PRIVATE
	surma_lib
	spdlog::spdlog
)

set(SURMA_SANITIZER "" CACHE STRING
	"Sanitizer to enable: address,undefined | thread | memory")

if (SURMA_SANITIZER)
	message(STATUS "Sanitizer enabled: ${SURMA_SANITIZER}")
	target_compile_options(surma_lib PRIVATE
		-fsanitize=${SURMA_SANITIZER}
		-fno-omit-frame-pointer
		-g
	)
	target_link_options(surma PRIVATE
		-fsanitize=${SURMA_SANITIZER}
	)
endif()


enable_testing()
add_subdirectory(tests)
