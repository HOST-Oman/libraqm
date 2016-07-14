#########################################

# - Locate FreeType library
# This module defines
#  FREETYPE_LIBRARIES, the library to link against
#  FREETYPE_FOUND, if false, do not try to link to FREETYPE
#  FREETYPE_INCLUDE_DIRS, where to find headers.
#  This is the concatenation of the paths:
#  FREETYPE_INCLUDE_DIR_ft2build
#  FREETYPE_INCLUDE_DIR_freetype2
FIND_PACKAGE(Freetype REQUIRED)
IF (FREETYPE_FOUND)
	MESSAGE(STATUS "FreeType2 Library Found OK: ${FREETYPE_LIBRARIES}")
	#INCLUDE(CheckLibraryExists)
	#CHECK_LIBRARY_EXISTS(${FREETYPE_LIBRARY} FT_Get_First_Char "" HAVE_FREETYPE_FIRST)
	#CHECK_LIBRARY_EXISTS(${FREETYPE_LIBRARY} FT_Get_Next_Char "" HAVE_FREETYPE_NEXT)
ELSE (FREETYPE_FOUND)
	MESSAGE(FATAL_ERROR "No Freetype Found")
ENDIF(FREETYPE_FOUND)

##################################3

# - Find the native FriBiDI includes and library
#
# This module defines
#  FRIBIDI_INCLUDE_DIR, where to find fribidi.h, etc.
#  FRIBIDI_LIBRARIES, the libraries to link against to use FriBiDi.
#  FRIBIDI_FOUND, If false, do not try to use fribidi.
# also defined, but not for general use are
#  FRIBIDI_LIBRARY, where to find the FriBiDi library.

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

##################################33


# Find Harfbuzz include and library directories.
#
# After successful discovery, this will set for inclusion where needed:
# HARFBUZZ_INCLUDE_DIRS - containg the HarfBuzz headers
# HARFBUZZ_LIBRARIES - containg the HarfBuzz library



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

####################################

# add the binary tree directory to the search path for include files
INCLUDE_DIRECTORIES(
	${CMAKE_BINARY_DIR}	
	${CMAKE_CURRENT_BINARY_DIR} 
 	${CMAKE_CURRENT_SOURCE_DIR}
	${PROJECT_SOURCE_DIR}
	${HARFBUZZ_INCLUDE_DIRS}
	${FRIBIDI_INCLUDE_DIR}/fribidi	
	${FREETYPE_INCLUDE_DIR_freetype2}
)

#headers files
set(HEADERS
	reorder_runs.h
	raqm.h
)

#source files
set(SOURCES
	reorder_runs.c
	raqm.c	
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
	${HARFBUZZ_LIBRARIES}
)

############################################## GTK MAIL LISTING

#set(exec_prefix /usr/local/)
#find_package (GtkDoc REQUIRED)



#gtk_doc_add_module(
#      libraqm
#      SOURCE ${CMAKE_SOURCE_DIR}/src 
#      XML ${CMAKE_SOURCE_DIR}/docs/raqm-docs.xml
#)



###############################################

# INSTALLING PART

install(TARGETS raqm
     RUNTIME DESTINATION bin COMPONENT libraries
     LIBRARY DESTINATION lib COMPONENT libraries
     ARCHIVE DESTINATION lib/static COMPONENT libraries)
install (FILES raqm.h DESTINATION include)

