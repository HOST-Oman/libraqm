
execute_process(COMMAND ${TEST_PROG} ${testname} 
OUTPUT_FILE output.txt
RESULT_VARIABLE HAD_ERROR
ERROR_FILE error.txt
 )

configure_file(output.txt output.txt NEWLINE_STYLE UNIX)

if(HAD_ERROR)
    file (READ error.txt error)
    message(FATAL_ERROR "Test failed: ${error}")
endif()

execute_process(COMMAND ${CMAKE_COMMAND} -E compare_files
    output.txt ${SOURCEDIR}/${testname}
    RESULT_VARIABLE DIFFERENT)
if(DIFFERENT)
#     configure_file(output.txt ${SOURCEDIR}/expected.txt COPYONLY)
#     execute_process(COMMAND svn diff ${SOURCEDIR}/expected.txt)
    message(FATAL_ERROR "Test failed - files differ")
endif()