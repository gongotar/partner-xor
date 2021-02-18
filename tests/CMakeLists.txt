add_executable (combined_unit_test 
    combined_unit_test.c
    xor_cp_recover.c)

include_directories(
    ${MPI_INCLUDE_PATH}
    ${PROJECT_SOURCE_DIR}/tests
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(combined_unit_test 
    ${PROJECT_NAME} 
    ${MPI_C_LIBRARIES}
)

configure_file(${PROJECT_SOURCE_DIR}/tests/test_runner.sh test_runner.sh COPYONLY)
configure_file(${PROJECT_SOURCE_DIR}/tests/config.ini config.ini COPYONLY)

#add_test(
#    NAME single_chunk_single_var_xor_checkpoint_recover_nano_sizes
#    COMMAND test_runner.sh 100 1000 270 ${PROJECT_SOURCE_DIR}/tests/config.ini 
#)
#
#add_test(
#    NAME single_chunk_single_var_xor_checkpoint_recover_micro_sizes
#    COMMAND test_runner.sh 1000 100000 18000 ${PROJECT_SOURCE_DIR}/tests/config.ini 
#)
#
#add_test(
#    NAME single_chunk_single_var_xor_checkpoint_recover_small_sizes
#    COMMAND test_runner.sh 100000 1600000 250000 ${PROJECT_SOURCE_DIR}/tests/config.ini 
#)

add_test(
    NAME multiple_chunk_single_var_xor_checkpoint_recover_medium_sizes
    COMMAND test_runner.sh 160100 20000000 5000000 ${PROJECT_SOURCE_DIR}/tests/config.ini 
)



