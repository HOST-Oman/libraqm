# project name
project (docs)
cmake_minimum_required (VERSION 3.2)

###########################

# configure a file to pass some of the CMake settings to other file
configure_file (
  "${CMAKE_CURRENT_SOURCE_DIR}/version.xml.in"
  "${CMAKE_CURRENT_BINARY_DIR}/version.xml"
  )

############################

INCLUDE(FindPkgConfig)
PKG_CHECK_MODULES(GLIB REQUIRED  glib-2.0)
FIND_PACKAGE(Freetype REQUIRED)
FIND_PATH(FRIBIDI_INCLUDE_DIR fribidi/fribidi.h
  /usr/local/include
  /usr/include
  )
pkg_check_modules(PC_HARFBUZZ REQUIRED harfbuzz>=0.9.20)
FIND_PATH(RAQM_INCLUDE_DIR
  /usr/local/include
  /usr/include
)

FIND_LIBRARY(RAQM_LIBRARY
  NAMES raqm
  PATHS /usr/lib /usr/local/lib
)

##############################################3 

set(exec_prefix /usr/local/)
find_package(GtkDoc)
	if(NOT GTKDOC_FOUND)
		message(FATAL_ERROR "gtkdoc was not found, API documentation can not be built.")
	endif()

gtk_doc_add_module(libraqm
		SOURCE ${CMAKE_SOURCE_DIR}/src
		#IGNOREHEADERS reorder_runs.h raqm.h
		SUFFIXES h c
		LDFLAGS
			-L${CMAKE_BINARY_DIR}/src
			-lraqm
		LDPATH
			"${CMAKE_BINARY_DIR}/src"
		#OUTPUT_DIR
		#	"${CMAKE_CURRENT_SOURCE_DIR}/api"
		DEPENDS raqm
)

#set_directory_properties (PROPERTIES CLEAN_NO_CUSTOM ${CMAKE_CURRENT_SOURCE_DIR}/api)
 #set_directory_properties (PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES "${CMAKE_CURRENT_SOURCE_DIR}/api/html;${CMAKE_CURRENT_SOURCE_DIR}/api/xml;${CMAKE_CURRENT_SOURCE_DIR}/api/tmpl")



############################################

#Install configure file
#install (FILES "${PROJECT_BINARY_DIR}/version.xml" DESTINATION ${path})
#install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/grx/html DESTINATION ${CMAKE_INSTALL_DOCDIR})
#INSTALL(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/html DESTINATION share/gtk-doc/html/libhif)

