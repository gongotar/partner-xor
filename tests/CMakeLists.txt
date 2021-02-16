add_executable (combined_unit_test combined_unit_test.c)

include_directories(
    ${MPI_INCLUDE_PATH}
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(combined_unit_test 
    ${PROJECT_NAME} 
    ${MPI_C_LIBRARIES}
)

configure_file(${PROJECT_SOURCE_DIR}/tests/test_runner.sh test_runner.sh COPYONLY)
configure_file(${PROJECT_SOURCE_DIR}/tests/config.ini config.ini COPYONLY)

add_test(
    NAME unit_test_n8_2x4
    COMMAND test_runner.sh 8 2 4 ${PROJECT_SOURCE_DIR}/tests/config.ini 
)

