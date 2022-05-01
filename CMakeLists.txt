cmake_minimum_required(VERSION 3.12)
project(nsutils
		VERSION 0.2.1
		DESCRIPTION "Linux namespace utilities"
		HOMEPAGE_URL "https://github.com/rd235/nsutils"
		LANGUAGES C)

include(GNUInstallDirs)
add_definitions(-D_GNU_SOURCE)
# string(LENGTH "${CMAKE_SOURCE_DIR}/" SOURCE_PATH_SIZE)
# add_definitions("-DSOURCE_PATH_SIZE=${SOURCE_PATH_SIZE}")
# set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_FORTIFY_SOURCE=2 -O2 -pedantic -Wall -Wextra")

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

find_library(TESTCAP cap)
if(NOT TESTCAP)
  message(FATAL_ERROR "libcap not found")
endif()

add_executable(nshold nshold.c catargv.c prefix.c)
target_link_libraries(nshold cap)
install(TARGETS nshold
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

add_executable(nslist nslist.c catargv.c memogetusername.c prefix.c printflen.c)
install(TARGETS nslist
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

add_executable(nsrelease nsrelease.c nssearch.c catargv.c prefix.c)
install(TARGETS nsrelease
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

add_executable(netnsjoin netnsjoin.c nssearch.c catargv.c prefix.c)
target_link_libraries(netnsjoin cap)
install(TARGETS netnsjoin
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

install(CODE 
		"execute_process(COMMAND setcap cap_sys_ptrace,cap_sys_admin+p ${CMAKE_INSTALL_FULL_BINDIR}/netnsjoin)"
		COMMAND_ECHO
		)

add_subdirectory(symlinks)
add_subdirectory(man)

add_custom_target(uninstall
  "${CMAKE_COMMAND}" -P "${PROJECT_SOURCE_DIR}/Uninstall.cmake")
