cmake_minimum_required(VERSION 3.2)

######################################

#check GLIB module if exist
INCLUDE(FindPkgConfig)
PKG_CHECK_MODULES(GLIB REQUIRED  glib-2.0)

######################################

FIND_PACKAGE(Freetype REQUIRED)

#FIND_PATH(FRIBIDI_INCLUDE_DIR fribidi/fribidi.h
 # /usr/local/include
  #/usr/include
  #)
#pkg_check_modules(PC_HARFBUZZ REQUIRED harfbuzz>=0.9.20)


include_directories(
	#${HARFBUZZ_INCLUDE_DIRS}
	#${FRIBIDI_INCLUDE_DIR}/fribidi	
	${GLIB_INCLUDE_DIRS}	
	${FREETYPE_INCLUDE_DIR_freetype2}
	${CMAKE_CURRENT_SOURCE_DIR}/fonts/sha1sum
	${CMAKE_CURRENT_SOURCE_DIR}
 )

add_subdirectory (fonts)

#enable C99 mode
add_definitions(-std=c99)

#set the variable TEST_SRCS to the paths of the test files 
set(TEST_SRCS 
     ${CMAKE_CURRENT_SOURCE_DIR}/features-arabic.test 
     ${CMAKE_CURRENT_SOURCE_DIR}/features-kerning.test 
     ${CMAKE_CURRENT_SOURCE_DIR}/features-ligature.test 
     ${CMAKE_CURRENT_SOURCE_DIR}/scripts-backward.test 
     ${CMAKE_CURRENT_SOURCE_DIR}/scripts-backward-ltr.test 
     ${CMAKE_CURRENT_SOURCE_DIR}/scripts-backward-rtl.test 
     ${CMAKE_CURRENT_SOURCE_DIR}/scripts-forward.test 
     ${CMAKE_CURRENT_SOURCE_DIR}/scripts-forward-ltr.test 
     ${CMAKE_CURRENT_SOURCE_DIR}/scripts-forward-rtl.test 
     ${CMAKE_CURRENT_SOURCE_DIR}/multi-fonts.test 
     ${CMAKE_CURRENT_SOURCE_DIR}/multi-fonts2.test 
     ${CMAKE_CURRENT_SOURCE_DIR}/xyoffset.test 
     ${CMAKE_CURRENT_SOURCE_DIR}/test1.test 
     ${CMAKE_CURRENT_SOURCE_DIR}/test2.test 
     ${CMAKE_CURRENT_SOURCE_DIR}/test3.test 
     ${CMAKE_CURRENT_SOURCE_DIR}/test4.test 
     ${CMAKE_CURRENT_SOURCE_DIR}/test5.test 
     ${CMAKE_CURRENT_SOURCE_DIR}/test1_LTR.test 
     ${CMAKE_CURRENT_SOURCE_DIR}/test2_LTR.test 
     ${CMAKE_CURRENT_SOURCE_DIR}/test3_LTR.test 
     ${CMAKE_CURRENT_SOURCE_DIR}/test4_LTR.test 
     ${CMAKE_CURRENT_SOURCE_DIR}/test5_LTR.test 
     ${CMAKE_CURRENT_SOURCE_DIR}/test1_RTL.test 
     ${CMAKE_CURRENT_SOURCE_DIR}/test2_RTL.test 
     ${CMAKE_CURRENT_SOURCE_DIR}/test3_RTL.test 
     ${CMAKE_CURRENT_SOURCE_DIR}/test4_RTL.test 
     ${CMAKE_CURRENT_SOURCE_DIR}/test5_RTL.test 
     ${CMAKE_CURRENT_SOURCE_DIR}/cursor_position1.test 
     ${CMAKE_CURRENT_SOURCE_DIR}/cursor_position2.test 
     ${CMAKE_CURRENT_SOURCE_DIR}/cursor_position3.test 
     ${CMAKE_CURRENT_SOURCE_DIR}/cursor_position4.test 
     ${CMAKE_CURRENT_SOURCE_DIR}/cursor_position_GB3.test 
     ${CMAKE_CURRENT_SOURCE_DIR}/cursor_position_GB4.test 
     ${CMAKE_CURRENT_SOURCE_DIR}/cursor_position_GB5.test 
     ${CMAKE_CURRENT_SOURCE_DIR}/cursor_position_GB8a.test 
     ${CMAKE_CURRENT_SOURCE_DIR}/cursor_position_GB9.test 
     ${CMAKE_CURRENT_SOURCE_DIR}/cursor_position_GB9a.test
)

#find the path of RAQM_INCLUDE_DIR
FIND_PATH(RAQM_INCLUDE_DIR
  /usr/local/include
  /usr/include
)

#find RAQM_LIBRARY
FIND_LIBRARY(RAQM_LIBRARY
  NAMES raqm
  PATHS /usr/lib /usr/local/lib
)


add_executable(raqm-test raqm-test.c)

#link the executable file with libraries
target_link_libraries(raqm-test 
	${RAQM_LIBRARY} 
	${FREETYPE_LIBRARIES}
	${GLIB_LIBRARIES}	
)


#set the variable SHELL_EXECUTABLE to certain path
set(SHELL_EXECUTABLE ${CMAKE_CURRENT_SOURCE_DIR}/runtest)


 add_custom_target(check ALL
                   COMMAND  sh ${SHELL_EXECUTABLE} ${TEST_SRCS}
                   DEPENDS  raqm-test
)



######################3



