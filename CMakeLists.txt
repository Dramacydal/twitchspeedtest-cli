cmake_minimum_required(VERSION 3.18)

project(twitchspeedtest-cli)

set(CMAKE_CXX_STANDARD 20)

if (NOT WIN32)
    find_package(CURL REQUIRED)
endif()

include_directories(deps)

add_compile_definitions(NO_CRYPTO)

add_executable(twitchspeedtest-cli
        main.cpp
        twitchspeedtestcli.cpp
        deps/librtmp/amf.c
        deps/librtmp/cencode.c
        deps/librtmp/log.c
        deps/librtmp/md5.c
        deps/librtmp/parseurl.c
        deps/librtmp/rtmp.c
        deps/util/platform.cpp
)

if (NOT WIN32)
    target_link_libraries(twitchspeedtest-cli ${CURL_LIBRARIES})
endif ()
