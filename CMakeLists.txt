cmake_minimum_required(VERSION 3.10)
project(mnist_cpp)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -g")

# 查找依赖库
# find_package(Eigen3 REQUIRED)
set(EIGEN3_INCLUDE_DIR "/usr/local/include/Eigen" CACHE PATH "Path to Eigen headers")
find_package(ZLIB REQUIRED)
find_package(CURL REQUIRED)

# 包含头文件目录
include_directories(
    ${EIGEN3_INCLUDE_DIR}
    ${CURL_INCLUDE_DIRS}
    ${ZLIB_INCLUDE_DIRS}
    include
)

# 包含头文件目录
include_directories(include)

# 收集所有源文件
file(GLOB SOURCES "src/*.cpp")

# 创建可执行文件
add_executable(mnist_cpp ${SOURCES})