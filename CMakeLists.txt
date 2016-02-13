project(libwap C)

cmake_minimum_required(VERSION 3.2)

set(CMAKE_C_FLAGS "-Wall")

# nowaways built in libc, normally
add_definitions(-DHAVE_ICONV)

add_executable(wabread
	cencode.c cencode.h libwab.c libwab.h pstwabids.c pstwabids.h tools.c tools.h uerr.c uerr.h wabread.c
)
