# minimum required vertion of cmake
cmake_minimum_required (VERSION 3.2)

# project name
project (raqm)

set (VERSION 0.2)
include(CheckCCompilerFlag)
check_c_compiler_flag("-std=c99" C99_SUPPORTED)
if (C99_SUPPORTED)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99")
endif()


 if(NOT RAQM_INSTALL_LIB_DIR)
  set(RAQM_INSTALL_LIB_DIR "lib")
endif()

if(NOT RAQM_INSTALL_INCLUDE_DIR)
  set(RAQM_INSTALL_INCLUDE_DIR "include")
endif()

set(RAQM_SOURCE_DIR ${CMAKE_SOURCE_DIR})
#########################################

# - Locate FreeType library
# This module defines
#  FREETYPE_LIBRARIES, the library to link against
#  FREETYPE_FOUND, if false, do not try to link to FREETYPE
#  FREETYPE_INCLUDE_DIRS, where to find headers.
#  This is the concatenation of the paths:
#  FREETYPE_INCLUDE_DIR_ft2build
#  FREETYPE_INCLUDE_DIR_freetype2

if( WIN32 AND NOT(MSYS OR CYGWIN OR MINGW) )
	find_path(FREETYPE_INCLUDE_DIR ft2build.h
		HINTS ${PROJECT_SOURCE_DIR}/win/include/freetype2
	)
      find_library(FREETYPE_LIBRARIES 
			NAMES libfreetype-6 freetype libfreetype
			HINTS ${PROJECT_SOURCE_DIR}/win/lib
		)
      message("-- Found Freetype: ${FREETYPE_LIBRARIES} ")

else()
	find_package(Freetype REQUIRED)
	if (FREETYPE_FOUND)
		message(STATUS "FreeType2 Library Found OK: ${FREETYPE_LIBRARIES}")
	else (FREETYPE_FOUND)
		message(FATAL_ERROR "No Freetype Found")
	endif(FREETYPE_FOUND)
endif(WIN32 AND NOT(MSYS OR CYGWIN OR MINGW))

##################################3


# - Find the native FriBiDI includes and library
#
# This module defines
#  FRIBIDI_INCLUDE_DIR, where to find fribidi.h, etc.
#  FRIBIDI_LIBRARIES, the libraries to link against to use FriBiDi.
#  FRIBIDI_FOUND, If false, do not try to use fribidi.
# also defined, but not for general use are
#  FRIBIDI_LIBRARY, where to find the FriBiDi library.
set(FRIBIDI_NAMES fribidi libfribidi)

if (WIN32 AND NOT(MSYS OR CYGWIN OR MINGW))
	find_path( FRIBIDI_INCLUDE_DIR fribidi.h
		PATH ${PROJECT_SOURCE_DIR}/win/include/fribidi
	)
	find_library(FRIBIDI_LIBRARY 
		NAMES ${FRIBIDI_NAMES}
		HINTS ${PROJECT_SOURCE_DIR}/win/lib/
		)
	message("-- Found Fribidi: ${FRIBIDI_LIBRARY} ")
else()
	include(CheckFunctionExists)
	set(FRIBIDI_FOUND "NO")

	find_path(FRIBIDI_INCLUDE_DIR fribidi/fribidi.h
		/usr/local/include
		/usr/include
	)

	find_library(FRIBIDI_LIBRARY
		NAMES ${FRIBIDI_NAMES}
		PATHS /usr/lib /usr/local/lib
	)

	if (FRIBIDI_LIBRARY AND FRIBIDI_INCLUDE_DIR)
		set(CMAKE_REQUIRED_INCLUDES ${FRIBIDI_INCLUDE_DIR})
		set(CMAKE_REQUIRED_LIBRARIES ${FRIBIDI_LIBRARY})
		CHECK_FUNCTION_EXISTS(fribidi_utf8_to_unicode FOUND_fribidi_utf8_to_unicode)
		if(FOUND_fribidi_utf8_to_unicode)
			set(FRIBIDI_LIBRARIES ${FRIBIDI_LIBRARY})
			set(FRIBIDI_FOUND "YES")
		else(FOUND_fribidi_utf8_to_unicode)
			set(FRIBIDI_LIBRARIES "NOTFOUND")
			set(FRIBIDI_INCLUDE_DIR "NOTFOUND")
			set(FRIBIDI_FOUND "NO")
		endif(FOUND_fribidi_utf8_to_unicode)
	endif (FRIBIDI_LIBRARY AND FRIBIDI_INCLUDE_DIR)

	if (FRIBIDI_FOUND)
		if (NOT FRIBIDI_FIND_QUIETLY)
			message(STATUS "Found FriBiDi: ${FRIBIDI_LIBRARY}")
		endif (NOT FRIBIDI_FIND_QUIETLY)
		else (FRIBIDI_FOUND)
			if (FRIBIDI_FIND_REQUIRED)
				message(FATAL_ERROR "Could not find FriBiDi library")
			endif (FRIBIDI_FIND_REQUIRED)
	endif (FRIBIDI_FOUND)
endif()
##################################33


# Find Harfbuzz include and library directories.
#
# After successful discovery, this will set for inclusion where needed:
# HARFBUZZ_INCLUDE_DIRS - containg the HarfBuzz headers
# HARFBUZZ_LIBRARIES - containg the HarfBuzz library


if(WIN32 AND NOT(MSYS OR CYGWIN OR MINGW))
	find_path(HARFBUZZ_INCLUDE_DIRS hb.h
		PATH ${PROJECT_SOURCE_DIR}/win/include/harfbuzz
	)
	find_library(HARFBUZZ_LIBRARIES 
			NAMES libharfbuzz-0 libharfbuzz harfbuzz libharfbuzzd
			HINTS ${PROJECT_SOURCE_DIR}/win/lib/
		)
	message("-- Found Harfbuzz: ${HARFBUZZ_LIBRARIES} ")
else()
	include(FindPkgConfig)
	pkg_check_modules(PC_HARFBUZZ REQUIRED harfbuzz>=0.9.20)

	find_path(HARFBUZZ_INCLUDE_DIRS NAMES hb.h
		HINTS ${PC_HARFBUZZ_INCLUDE_DIRS} ${PC_HARFBUZZ_INCLUDEDIR} 
		)

	find_library(HARFBUZZ_LIBRARIES NAMES harfbuzz
		HINTS ${PC_HARFBUZZ_LIBRARY_DIRS} ${PC_HARFBUZZ_LIBDIR} 
		)
	if (PC_HARFBUZZ_FOUND)
		message(STATUS "Harfbuzz Library Found OK:  ${HARFBUZZ_LIBRARIES} ")
		INCLUDE(FindPackageHandleStandardArgs)
		FIND_PACKAGE_HANDLE_STANDARD_ARGS(HarfBuzz DEFAULT_MSG HARFBUZZ_INCLUDE_DIRS HARFBUZZ_LIBRARIES)
	ELSE ()
		message(FATAL_ERROR "No harfbuzz Found")
	ENDif()
endif(WIN32 AND NOT(MSYS OR CYGWIN OR MINGW))
####################################

if(WIN32 AND NOT(MSYS OR CYGWIN OR MINGW))
INCLUDE_DIRECTORIES(
	${FRIBIDI_INCLUDE_DIR}
	${FREETYPE_INCLUDE_DIR}
	${HARFBUZZ_INCLUDE_DIRS}
)
else()
INCLUDE_DIRECTORIES(
	${FRIBIDI_INCLUDE_DIR}/fribidi
	${FREETYPE_INCLUDE_DIR_freetype2}
	${HARFBUZZ_INCLUDE_DIRS}
)
endif()

#headers files
set(HEADERS
	${PROJECT_SOURCE_DIR}/src/raqm.h
)

#source files
set(SOURCES
	${PROJECT_SOURCE_DIR}/src/raqm.c
)


option(static_lib "Create static library" OFF)
if(static_lib)
#create static library
add_library( raqm STATIC
	${HEADERS}
	${SOURCES}
)
else()
#create shared library
add_library( raqm SHARED
	${HEADERS}
	${SOURCES}
)
endif()

# link the libraries to the executable
target_link_libraries (raqm
	${FREETYPE_LIBRARIES}
	${FRIBIDI_LIBRARY}
	${GLIB_LIBRARY}
	${HARFBUZZ_LIBRARIES}
)

#enable compiler warning for GNU C
if(CMAKE_COMPILER_IS_GNUCC)
add_definitions (-Wall  -Wno-portability)
endif()

#enable compiler warning for MSVC
if (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
  set(CMAKE_C_FLAGS "/W4 /WX /wd4068")
endif()

###############################################
#UNIT TESTING

option (WANT_DEBUG "enable debugging" OFF)
if (WANT_DEBUG)
    set(CMAKE_BUILD_TYPE Debug)
    set (RAQM_DEBUG YES)
    message( STATUS "debugging mode are enabled ...")
endif()

#TESTING not supported yet
option(UNIT_TEST "apply unit test" OFF)

if(UNIT_TEST)
    # add_custom_target(check ${CMAKE_CTEST_COMMAND} -V)
    enable_testing()
    set (RAQM_TESTING ON)
    include_directories(${PROJECT_SOURCE_DIR}/tests)
    add_subdirectory (tests)
    message( STATUS "RAQM_TESTING mode are enabled ...")
endif()

if (UNIT_TEST OR WANT_DEBUG)
configure_file(
	${CMAKE_SOURCE_DIR}/config.h.cmake
	${CMAKE_SOURCE_DIR}/src/config.h
)
endif (UNIT_TEST OR WANT_DEBUG)

###############################################
# DOCUMENTATION
 option(enable-gtk-doc "build the documentation" OFF)
 if (enable-gtk-doc)
     INCLUDE(FindPkgConfig)
     find_package(GtkDoc 1.25 REQUIRED)

     if(GTKDOC_FOUND)
     	message(STATUS "gtkdoc found, API documentation will be built.")
     	#configure_file(${CMAKE_SOURCE_DIR}/docs/version.xml.in version.xml @ONLY)
     	configure_file(${CMAKE_SOURCE_DIR}/docs/raqm-docs.cmake.xml.in
			raqm-docs.xml @ONLY)
	gtk_doc_add_module(raqm
	    XML
		${CMAKE_CURRENT_BINARY_DIR}/raqm-docs.xml
	    SOURCE
		${CMAKE_CURRENT_SOURCE_DIR}
		${CMAKE_SOURCE_DIR}/src
	    SUFFIXES
		"h"
		"c"
	    LIBRARIES
		raqm
	    LIBRARY_DIRS
		${FREETYPE_LIBRARIES}
		${FRIBIDI_LIBRARY}
		${HARFBUZZ_LIBRARIES}
		${GLIB_LIBRARIES}
		)


	add_custom_target (docs ALL DEPENDS doc-raqm)

	include(GNUInstallDirs)
	install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/raqm/html
	DESTINATION ${CMAKE_INSTALL_DOCDIR})
     else()
           message(STATUS "gtkdoc was not found, API documentation can not be built.")
     endif()
else()
	message (STATUS "build without documentation")
endif()
  
###############################################

# INSTALLING PART


#-----------------------------------------------------------------------------
# pkgconfig support
# enabled by default on Unix, disabled by default on other platforms
if(UNIX)
  option(BUILD_PKGCONFIG_FILES "Build and install pkg-config files" ON)
else()
  option(BUILD_PKGCONFIG_FILES "Build and install pkg-config files" OFF)
endif()

if(BUILD_PKGCONFIG_FILES)
  configure_file(raqm.pc.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/raqm.pc @ONLY)
  install( FILES ${CMAKE_CURRENT_BINARY_DIR}/raqm.pc DESTINATION ${RAQM_INSTALL_LIB_DIR}/pkgconfig )
endif()



# Install library
install(TARGETS raqm
  LIBRARY DESTINATION ${RAQM_INSTALL_LIB_DIR} COMPONENT Libraries
  ARCHIVE DESTINATION ${RAQM_INSTALL_LIB_DIR} COMPONENT Libraries
)

# Install includes files
install(FILES ${CMAKE_SOURCE_DIR}/src/raqm.h
  DESTINATION ${RAQM_INSTALL_INCLUDE_DIR} COMPONENT Headers
)

