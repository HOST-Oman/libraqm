#.rst:
# FindGtkDoc
# ----------
#
# CMake macros to find and use the GtkDoc documentation system

# Output variables:
#
#   GTKDOC_FOUND            ... set to 1 if GtkDoc was foung
#
# If GTKDOC_FOUND == 1:
#
#   GTKDOC_SCAN_EXE         ... the location of the gtkdoc-scan executable
#   GTKDOC_SCANGOBJ_EXE     ... the location of the gtkdoc-scangobj executable
#   GTKDOC_MKDB_EXE         ... the location of the gtkdoc-mkdb executable
#   GTKDOC_MKHTML_EXE       ... the location of the gtkdoc-mkhtml executable
#   GTKDOC_FIXXREF_EXE      ... the location of the gtkdoc-fixxref executable

#=============================================================================
# Copyright 2009 Rich Wareham
# Copyright 2015 Lautsprecher Teufel GmbH
#
# Distributed under the OSI-approved BSD License (the "License");
# see accompanying file Copyright.txt for details.
#
# This software is distributed WITHOUT ANY WARRANTY; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the License for more information.
#=============================================================================
# (To distribute this file outside of CMake, substitute the full
#  License text for the above reference.)

include(CMakeParseArguments)
include(FindPackageHandleStandardArgs)

find_package(PkgConfig REQUIRED)

# The UseGtkDoc.cmake module requires at least 1.9, because it doesn't use
# the deprecated `gtkdoc-mktmpl` tool. We will check if the version satisfies
# the user's specified dependencies later (with the
# find_package_handle_standard_args() command).
pkg_check_modules(GtkDoc REQUIRED gtk-doc>=1.9)

find_program(GTKDOC_SCAN_EXE gtkdoc-scan PATH "${GLIB_PREFIX}/bin")
find_program(GTKDOC_SCANGOBJ_EXE gtkdoc-scangobj PATH "${GLIB_PREFIX}/bin")

get_filename_component(_this_dir ${CMAKE_CURRENT_LIST_FILE} PATH)
find_file(GTKDOC_SCANGOBJ_WRAPPER GtkDocScanGObjWrapper.cmake PATH ${_this_dir})

find_program(GTKDOC_MKDB_EXE gtkdoc-mkdb PATH "${GLIB_PREFIX}/bin")
find_program(GTKDOC_MKHTML_EXE gtkdoc-mkhtml PATH "${GLIB_PREFIX}/bin")
find_program(GTKDOC_FIXXREF_EXE gtkdoc-fixxref PATH "${GLIB_PREFIX}/bin")

find_package_handle_standard_args(GtkDoc
    REQUIRED_VARS GTKDOC_SCAN_EXE GTKDOC_SCANGOBJ_EXE GTKDOC_MKDB_EXE GTKDOC_MKHTML_EXE GTKDOC_FIXXREF_EXE
    VERSION_VAR GtkDoc_VERSION)

# ::
#
# gtk_doc_add_module(doc_prefix sourcedir
#                    [XML xmlfile]
#                    [FIXXREFOPTS fixxrefoption1...]
#                    [IGNOREHEADERS header1...]
#                    [DEPENDS depend1...] )
#
# sourcedir must be the *full* path to the source directory.
#
# If omitted, sgmlfile defaults to the auto generated ${doc_prefix}/${doc_prefix}-docs.xml.
macro(gtk_doc_add_module _doc_prefix _doc_sourcedir)
    set(_one_value_args "XML")
    set(_multi_value_args "DEPENDS" "XML" "FIXXREFOPTS" "IGNOREHEADERS"
                          "CFLAGS" "LDFLAGS" "LDPATH" "SUFFIXES")
    cmake_parse_arguments("GTK_DOC" "" "${_one_value_args}" "${_multi_value_args}" ${ARGN})

    set(_depends ${GTK_DOC_DEPENDS})
    set(_xml ${GTK_DOC_XML})
    set(_fixxrefopts ${GTK_DOC_FIXXREFOPTS})
    set(_ignoreheaders ${GTK_DOC_IGNOREHEADERS})
    set(_extra_cflags ${GTK_DOC_CFLAGS})
    set(_extra_ldflags ${GTK_DOC_LDFLAGS})
    set(_extra_ldpath ${GTK_DOC_LDPATH})
    set(_suffixes ${GTK_DOC_SUFFIXES})

    list(LENGTH _xml_file _xml_file_length)

    if(_suffixes)
        set(_doc_source_suffixes "")
        foreach(_suffix ${_suffixes})
            if(_doc_source_suffixes)
                set(_doc_source_suffixes "${_doc_source_suffixes},${_suffix}")
            else(_doc_source_suffixes)
                set(_doc_source_suffixes "${_suffix}")
            endif(_doc_source_suffixes)
        endforeach(_suffix)
    else(_suffixes)
        set(_doc_source_suffixes "h")
    endif(_suffixes)

    # set(_do_all ALL)

    set(_opts_valid 1)
    if(NOT _xml_file_length LESS 2)
        message(SEND_ERROR "Must have at most one sgml file specified.")
        set(_opts_valid 0)
    endif(NOT _xml_file_length LESS 2)

    if(_opts_valid)
        # a directory to store output.
        set(_output_dir "${CMAKE_CURRENT_BINARY_DIR}/${_doc_prefix}")
        set(_output_dir_stamp "${_output_dir}/dir.stamp")

        # set default sgml file if not specified
        set(_default_xml_file "${_output_dir}/${_doc_prefix}-docs.xml")
        get_filename_component(_default_xml_file ${_default_xml_file} ABSOLUTE)

        # a directory to store html output.
        set(_output_html_dir "${_output_dir}/html")
        set(_output_html_dir_stamp "${_output_dir}/html_dir.stamp")

        # The output files
        set(_output_decl_list "${_output_dir}/${_doc_prefix}-decl-list.txt")
        set(_output_decl "${_output_dir}/${_doc_prefix}-decl.txt")
        set(_output_overrides "${_output_dir}/${_doc_prefix}-overrides.txt")
        set(_output_sections "${_output_dir}/${_doc_prefix}-sections.txt")
        set(_output_types "${_output_dir}/${_doc_prefix}.types")

        set(_output_signals "${_output_dir}/${_doc_prefix}.signals")

        set(_output_unused "${_output_dir}/${_doc_prefix}-unused.txt")
        set(_output_undeclared "${_output_dir}/${_doc_prefix}-undeclared.txt")
        set(_output_undocumented "${_output_dir}/${_doc_prefix}-undocumented.txt")

        set(_output_xml_dir "${_output_dir}/xml")
        set(_output_sgml_stamp "${_output_dir}/sgml.stamp")

        set(_output_html_stamp "${_output_dir}/html.stamp")

        # add a command to create output directory
        add_custom_command(
            OUTPUT "${_output_dir_stamp}" "${_output_dir}"
            COMMAND ${CMAKE_COMMAND} -E make_directory "${_output_dir}"
            COMMAND ${CMAKE_COMMAND} -E touch ${_output_dir_stamp}
            VERBATIM)

        set(_ignore_headers_opt "")
        if(_ignore_headers)
            set(_ignore_headers_opt "--ignore-headers=")
            foreach(_header ${_ignore_headers})
                set(_ignore_headers_opt "${_ignore_headers_opt}${_header} ")
            endforeach(_header ${_ignore_headers})
        endif(_ignore_headers)

        # add a command to scan the input
        add_custom_command(
            OUTPUT
                "${_output_decl_list}"
                "${_output_decl}"
                "${_output_decl}.bak"
                "${_output_overrides}"
                "${_output_sections}"
                "${_output_types}"
                "${_output_types}.bak"
            DEPENDS
                "${_output_dir}"
                ${_depends}
            COMMAND ${GTKDOC_SCAN_EXE}
                "--module=${_doc_prefix}"
                "${_ignore_headers_opt}"
                "--rebuild-sections"
                "--rebuild-types"
                "--source-dir=${_doc_sourcedir}"
            WORKING_DIRECTORY "${_output_dir}"
            VERBATIM)

        # add a command to scan the input via gtkdoc-scangobj
        # This is such a disgusting hack!
        add_custom_command(
            OUTPUT
                "${_output_signals}"
            DEPENDS
                "${_output_types}"
            COMMAND ${CMAKE_COMMAND}
                -D "GTKDOC_SCANGOBJ_EXE:STRING=${GTKDOC_SCANGOBJ_EXE}"
                -D "doc_prefix:STRING=${_doc_prefix}"
                -D "output_types:STRING=${_output_types}"
                -D "output_dir:STRING=${_output_dir}"
                -D "EXTRA_CFLAGS:STRING=${_extra_cflags}"
                -D "EXTRA_LDFLAGS:STRING=${_extra_ldflags}"
                -D "EXTRA_LDPATH:STRING=${_extra_ldpath}"
                -P ${GTKDOC_SCANGOBJ_WRAPPER}
            WORKING_DIRECTORY "${_output_dir}"
            VERBATIM)

        set(_copy_xml_if_needed "")
        if(_xml_file)
            get_filename_component(_xml_file ${_xml_file} ABSOLUTE)
            set(_copy_xml_if_needed
                COMMAND ${CMAKE_COMMAND} -E copy "${_xml_file}" "${_default_xml_file}")
        endif(_xml_file)

        set(_remove_xml_if_needed "")
        if(_xml_file)
            set(_remove_xml_if_needed
                COMMAND ${CMAKE_COMMAND} -E remove ${_default_xml_file})
        endif(_xml_file)

        # add a command to make the database
        add_custom_command(
            OUTPUT
                "${_output_sgml_stamp}"
                "${_default_xml_file}"
            DEPENDS
                "${_output_types}"
                "${_output_signals}"
                "${_output_sections}"
                "${_output_overrides}"
                ${_depends}
            ${_remove_xml_if_needed}
            COMMAND ${CMAKE_COMMAND} -E remove_directory ${_output_xml_dir}
            COMMAND ${GTKDOC_MKDB_EXE}
                "--module=${_doc_prefix}"
                "--source-dir=${_doc_sourcedir}"
                "--source-suffixes=${_doc_source_suffixes}"
                "--output-format=xml"
                "--main-sgml-file=${_default_xml_file}"
            ${_copy_xml_if_needed}
            WORKING_DIRECTORY "${_output_dir}"
            VERBATIM)

        # add a command to create html directory
        add_custom_command(
            OUTPUT "${_output_html_dir_stamp}" "${_output_html_dir}"
            COMMAND ${CMAKE_COMMAND} -E make_directory ${_output_html_dir}
            COMMAND ${CMAKE_COMMAND} -E touch ${_output_html_dir_stamp}
            VERBATIM)

        # add a command to output HTML
        add_custom_command(
            OUTPUT
                "${_output_html_stamp}"
            DEPENDS
                "${_output_html_dir_stamp}"
                "${_output_sgml_stamp}"
                "${_xml_file}"
                ${_depends}
            ${_copy_xml_if_needed}
            COMMAND
                cd "${_output_html_dir}" && ${GTKDOC_MKHTML_EXE}
                    "${_doc_prefix}"
                    "${_default_xml_file}"
            COMMAND
                cd "${_output_dir}" && ${GTKDOC_FIXXREF_EXE}
                    "--module=${_doc_prefix}"
                    "--module-dir=${_output_html_dir}"
                    ${_fixxref_opts}
                    ${_remove_xml_if_needed}
            VERBATIM)

        add_custom_target(doc-${_doc_prefix} ${_do_all}
            DEPENDS "${_output_html_stamp}")
    endif(_opts_valid)
endmacro(gtk_doc_add_module)
