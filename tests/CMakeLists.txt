cmake_minimum_required(VERSION 3.2)

file(GLOB_RECURSE fonts  fonts/sha1sum/*)

if (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
set(BUILD_SHARED_LIBS OFF)
#     set(CMAKE_EXE_LINKER_FLAGS -static)
    set(CMAKE_FIND_LIBRARY_SUFFIXES .lib)
    set(CMAKE_EXE_LINK_DYNAMIC_C_FLAGS)       # remove -Wl,-Bdynamic
    set(CMAKE_EXE_LINK_DYNAMIC_CXX_FLAGS)
    set(CMAKE_SHARED_LIBRARY_C_FLAGS)         # remove -fPIC
    set(CMAKE_SHARED_LIBRARY_CXX_FLAGS)
    set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS)    # remove -rdynamic
    set(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS) 
endif()

add_executable(raqm-test raqm-test.c )

include_directories(
	${FREETYPE_INCLUDE_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}
 )

target_include_directories(raqm-test PUBLIC
  $<BUILD_INTERFACE:${RAQM_SOURCE_DIR}/src>
  $<INSTALL_INTERFACE:include/raqm> 
  )
#link the executable file with libraries
target_link_libraries(raqm-test 
	raqm 
	${FREETYPE_LIBRARIES}
	${FRIBIDI_LIBRARY}
	${HARFBUZZ_LIBRARIES}	
)

if (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
  set(CMAKE_C_FLAGS "/W4 /WX- /wd4068 /wd4996")
  target_compile_options(raqm-test INTERFACE  ${CMAKE_C_FLAGS})
endif()

file(COPY ${fonts} DESTINATION fonts/sha1sum) 

file(GLOB_RECURSE tests_list *.test)
foreach(test ${tests_list})
  get_filename_component (testname ${test} NAME_WE)
  get_filename_component (testpath ${test} ABSOLUTE)
  add_test(NAME ${testname}
    COMMAND ${CMAKE_COMMAND}
    -DTEST_PROG=$<TARGET_FILE:raqm-test>
    -DSOURCEDIR=${CMAKE_CURRENT_SOURCE_DIR}
    -Dtestname=${testname}
    -Dtestpath=${testpath}
    -P ${CMAKE_CURRENT_SOURCE_DIR}/runtest.cmake
    )
endforeach()
