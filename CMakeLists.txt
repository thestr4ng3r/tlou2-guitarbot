
cmake_minimum_required(VERSION 3.2)

project(guitarbot)

set(CHIAKI_ENABLE_TESTS OFF)
set(CHIAKI_ENABLE_CLI OFF)
set(CHIAKI_ENABLE_GUI OFF)
set(CHIAKI_ENABLE_ANDROID OFF)
set(CHIAKI_ENABLE_SWITCH OFF)
set(CHIAKI_ENABLE_SETSU OFF)
set(CHIAKI_LIB_ENABLE_OPUS OFF)
set(CHIAKI_LIB_ENABLE_MBEDTLS OFF)
set(CHIAKI_LIB_OPENSSL_EXTERNAL_PROJECT OFF)
add_subdirectory(chiaki)

add_executable(guitarbot main.c)
target_link_libraries(guitarbot chiaki-lib)
