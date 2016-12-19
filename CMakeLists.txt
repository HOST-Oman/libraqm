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


#TESTING not supported yet
# option(unit_test "apply unit test" OFF)
# 
# if(unit_test)
# # add_custom_target(check ${CMAKE_CTEST_COMMAND} -V)
#  include_directories(${PROJECT_SOURCE_DIR}/tests)
#  add_subdirectory (tests)
# endif()


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
	FIND_PATH(FREETYPE_INCLUDE_DIR ft2build.h
		PATH
		${PROJECT_SOURCE_DIR}/win/include/freetype2
	)
  
	option(32bit "Compile the library for WIN32.. make the option OFF to compile it for WIN64" ON)
  
	if(32bit)
		FIND_LIBRARY(FREETYPE_LIBRARIES libfreetype-6
			PATH
			${PROJECT_SOURCE_DIR}/win/lib/windows32
		)
	else(32bit)
		FIND_LIBRARY(FREETYPE_LIBRARIES libfreetype-6
			PATH
			${PROJECT_SOURCE_DIR}/win/lib/windows64
		)
	endif(32bit)
		message("-- Found Freetype: ${FREETYPE_LIBRARIES} ")

else()
	FIND_PACKAGE(Freetype REQUIRED)
	IF (FREETYPE_FOUND)
		MESSAGE(STATUS "FreeType2 Library Found OK: ${FREETYPE_LIBRARIES}")
		#INCLUDE(CheckLibraryExists)
		#CHECK_LIBRARY_EXISTS(${FREETYPE_LIBRARY} FT_Get_First_Char "" HAVE_FREETYPE_FIRST)
		#CHECK_LIBRARY_EXISTS(${FREETYPE_LIBRARY} FT_Get_Next_Char "" HAVE_FREETYPE_NEXT)
	ELSE (FREETYPE_FOUND)
		MESSAGE(FATAL_ERROR "No Freetype Found")
	ENDIF(FREETYPE_FOUND)
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
if (WIN32 AND NOT(MSYS OR CYGWIN OR MINGW))
	FIND_PATH( FRIBIDI_INCLUDE_DIR fribidi.h
		PATH
		${PROJECT_SOURCE_DIR}/win/include/fribidi
	)

	if(32bit)
		FIND_LIBRARY(FRIBIDI_LIBRARY libfribidi
		PATH
		${PROJECT_SOURCE_DIR}/win/lib/windows32
		)
	else()
		FIND_LIBRARY(FRIBIDI_LIBRARY libfribidi
		PATH
		${PROJECT_SOURCE_DIR}/win/lib/windows64
		)
	endif(32bit)

	message("-- Found Fribidi: ${FRIBIDI_LIBRARY} ")

else()
	include(CheckFunctionExists)
	SET(FRIBIDI_FOUND "NO")

	FIND_PATH(FRIBIDI_INCLUDE_DIR fribidi/fribidi.h
		/usr/local/include
		/usr/include
	)

	SET(FRIBIDI_NAMES ${FRIBIDI_NAMES} fribidi libfribidi)
	FIND_LIBRARY(FRIBIDI_LIBRARY
		NAMES ${FRIBIDI_NAMES}
		PATHS /usr/lib /usr/local/lib
	)

	IF (FRIBIDI_LIBRARY AND FRIBIDI_INCLUDE_DIR)
		SET(CMAKE_REQUIRED_INCLUDES ${FRIBIDI_INCLUDE_DIR})
		SET(CMAKE_REQUIRED_LIBRARIES ${FRIBIDI_LIBRARY})
		CHECK_FUNCTION_EXISTS(fribidi_utf8_to_unicode FOUND_fribidi_utf8_to_unicode)
		IF(FOUND_fribidi_utf8_to_unicode)
			SET(FRIBIDI_LIBRARIES ${FRIBIDI_LIBRARY})
			SET(FRIBIDI_FOUND "YES")
		ELSE(FOUND_fribidi_utf8_to_unicode)
			SET(FRIBIDI_LIBRARIES "NOTFOUND")
			SET(FRIBIDI_INCLUDE_DIR "NOTFOUND")
			SET(FRIBIDI_FOUND "NO")
		ENDIF(FOUND_fribidi_utf8_to_unicode)
	ENDIF (FRIBIDI_LIBRARY AND FRIBIDI_INCLUDE_DIR)

	IF (FRIBIDI_FOUND)
		IF (NOT FRIBIDI_FIND_QUIETLY)
			MESSAGE(STATUS "Found FriBiDi: ${FRIBIDI_LIBRARY}")
		ENDIF (NOT FRIBIDI_FIND_QUIETLY)
		ELSE (FRIBIDI_FOUND)
			IF (FRIBIDI_FIND_REQUIRED)
				MESSAGE(FATAL_ERROR "Could not find FriBiDi library")
			ENDIF (FRIBIDI_FIND_REQUIRED)
	ENDIF (FRIBIDI_FOUND)
endif()
##################################33


# Find Harfbuzz include and library directories.
#
# After successful discovery, this will set for inclusion where needed:
# HARFBUZZ_INCLUDE_DIRS - containg the HarfBuzz headers
# HARFBUZZ_LIBRARIES - containg the HarfBuzz library


if(WIN32 AND NOT(MSYS OR CYGWIN OR MINGW))
	FIND_PATH(HARFBUZZ_INCLUDE_DIRS hb.h
		PATH
		${PROJECT_SOURCE_DIR}/win/include/harfbuzz
	)
	
	if(32bit)
		FIND_LIBRARY(HARFBUZZ_LIBRARIES libharfbuzz-0
			PATH
			${PROJECT_SOURCE_DIR}/win/lib/windows32
		)
	else(32bit)
		FIND_LIBRARY(HARFBUZZ_LIBRARIES libharfbuzz-0
			PATH
			${PROJECT_SOURCE_DIR}/win/lib/windows64
		)
	endif(32bit)
  
	message("-- Found Harfbuzz: ${HARFBUZZ_LIBRARIES} ")
 
else()
	INCLUDE(FindPkgConfig)
	pkg_check_modules(PC_HARFBUZZ REQUIRED harfbuzz>=0.9.20)


	FIND_PATH(HARFBUZZ_INCLUDE_DIRS NAMES hb.h
		HINTS ${PC_HARFBUZZ_INCLUDE_DIRS} ${PC_HARFBUZZ_INCLUDEDIR} )

	FIND_LIBRARY(HARFBUZZ_LIBRARIES NAMES harfbuzz
		HINTS ${PC_HARFBUZZ_LIBRARY_DIRS} ${PC_HARFBUZZ_LIBDIR} )

	IF (PC_HARFBUZZ_FOUND)
		MESSAGE(STATUS "Harfbuzz Library Found OK:  ${HARFBUZZ_LIBRARIES} ")
		INCLUDE(FindPackageHandleStandardArgs)
		FIND_PACKAGE_HANDLE_STANDARD_ARGS(HarfBuzz DEFAULT_MSG HARFBUZZ_INCLUDE_DIRS HARFBUZZ_LIBRARIES)
	ELSE ()
		MESSAGE(FATAL_ERROR "No harfbuzz Found")
	ENDIF()
endif(WIN32 AND NOT(MSYS OR CYGWIN OR MINGW))
####################################

if(WIN32 AND NOT(MSYS OR CYGWIN OR MINGW))
INCLUDE_DIRECTORIES(
	${FRIBIDI_INCLUDE_DIR}
	${FREETYPE_INCLUDE_DIR}
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
