cmake_minimum_required(VERSION 3.22)
project(DataStructureProject CXX)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# 添加子模块目录
add_subdirectory(record_reader)

# 生成顶层可执行文件
add_executable(main main.cpp)

# 链接 record_reader 静态库
target_link_libraries(main PRIVATE record_reader)
