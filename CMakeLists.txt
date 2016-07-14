# minimum required vertion of cmake
cmake_minimum_required (VERSION 3.2)

# project name
project (raqm)

##############################3

#enable compiler warning
add_definitions ("-Wall  -Wno-portability ")

#enable C99 mode
add_definitions(-std=gnu99)

#check GLIB module if exist
INCLUDE(FindPkgConfig)
PKG_CHECK_MODULES(GLIB REQUIRED  glib-2.0)


#set the values of raqam.pc file
set (prefix /usr/local)
set (exec_prefix ................................)
set (libdir /lib)
set (includedir /include)
set (PACKAGE_NAME "Raqm")
set (PACKAGE_VERSION 0.2.0)
set (AX_PACKAGE_REQUIRES "freetype2 >= 12.0.6")  
set (AX_PACKAGE_REQUIRES_PRIVATE "harfbuzz fribidi")
#set (Cflags: -I${includedir} )
#set (Libs: -L${libdir} -lraqm)


# configure a file to pass some of the CMake settings to other file
configure_file (
  "${PROJECT_SOURCE_DIR}/raqm.pc.in"
  "${PROJECT_BINARY_DIR}/raqm.pc"
  )


#include subdirectories 
include_directories ( 
	${PROJECT_SOURCE_DIR}
	${PROJECT_SOURCE_DIR}/docs
	#${PROJECT_SOURCE_DIR}/tests
	${PROJECT_SOURCE_DIR}/src
	#${PROJECT_SOURCE_DIR}/m4
)

add_subdirectory (docs)
#add_subdirectory (tests)
add_subdirectory (src)
#add_subdirectory (m4)



#TESTING
option(unit_test "apply unit test" OFF)

if(unit_test)
# add_custom_target(check ${CMAKE_CTEST_COMMAND} -V)
 include_directories(${PROJECT_SOURCE_DIR}/tests)
 add_subdirectory (tests)
endif()

#################################################3

#Install configure file
#install (FILES "${PROJECT_BINARY_DIR}/rarm.pc" DESTINATION ${path})


#Install our READMEs etc.
#INSTALL(FILES
#	AUTHORS
#	COPYING
#	NEWS
#	README
#	DESTINATION ${path}
#)





# Add "tags" target and make my_project depending on this target.
#set_source_files_properties(tags PROPERTIES GENERATED true)
#add_custom_command(tags
#    COMMAND gtkdocize --copy || exit 1
#    COMMAND autoreconf --force --install --verbose || exit $?
#    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
#add_dependencies(libraqm tags)



#delete some files
#set(cmake_generated ${CMAKE_BINARY_DIR}/CMakeCache.txt
#                   ${CMAKE_BINARY_DIR}/cmake_install.cmake  
#                  ${CMAKE_BINARY_DIR}/Makefile
#                 ${CMAKE_BINARY_DIR}/CMakeFiles
#)
#
#foreach(file ${cmake_generated})
#
# if (EXISTS ${file})
#     file(REMOVE_RECURSE ${file})
#  endif()
#
#endforeach(file)
