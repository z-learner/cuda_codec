cmake_minimum_required(VERSION 3.7)

project(decode_test)

add_executable(decode_test decode_by_file.cc)
target_link_libraries(decode_test NvCodec)