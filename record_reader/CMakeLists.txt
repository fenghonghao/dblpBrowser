# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# 收集所有的源文件
aux_source_directory(src SOURCES)

# 生成静态库
add_library(record_reader STATIC ${SOURCES})

target_include_directories(record_reader PUBLIC includes)
