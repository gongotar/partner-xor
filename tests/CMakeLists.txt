add_executable (singlevar_unit_test 
    singlevar_unit_test.c
    singlevar_xor_cp_recover.c)

add_executable (multivar_unit_test 
    multivar_unit_test.c
    multivar_xor_cp_recover.c)

include_directories(
    ${MPI_INCLUDE_PATH}
    ${PROJECT_SOURCE_DIR}/tests
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(singlevar_unit_test
    ${PROJECT_NAME} 
    ${MPI_C_LIBRARIES}
)

target_link_libraries(multivar_unit_test
    ${PROJECT_NAME} 
    ${MPI_C_LIBRARIES}
)

configure_file(${PROJECT_SOURCE_DIR}/tests/test_runner.sh test_runner.sh COPYONLY)
configure_file(${PROJECT_SOURCE_DIR}/tests/config.ini config.ini COPYONLY)

#add_test(
#    NAME single_chunk_single_var_xor_checkpoint_recover_nano_sizes
#    COMMAND test_runner.sh 100 1000 270 ${PROJECT_SOURCE_DIR}/tests/config.ini 0
#)
#
#add_test(
#    NAME single_chunk_single_var_xor_checkpoint_recover_micro_sizes
#    COMMAND test_runner.sh 1000 100000 18000 ${PROJECT_SOURCE_DIR}/tests/config.ini 0
#)
#
#add_test(
#    NAME single_chunk_single_var_xor_checkpoint_recover_small_sizes
#    COMMAND test_runner.sh 100000 1600000 250000 ${PROJECT_SOURCE_DIR}/tests/config.ini 0
#)
#
#add_test(
#    NAME multiple_chunk_single_var_xor_checkpoint_recover_medium_sizes
#    COMMAND test_runner.sh 1601000 20000000 5000000 ${PROJECT_SOURCE_DIR}/tests/config.ini 0
#)
#
#add_test(
#    NAME multiple_chunk_single_var_xor_checkpoint_recover_large_sizes
#    COMMAND test_runner.sh 100000000 300000000 70000000 ${PROJECT_SOURCE_DIR}/tests/config.ini 0
#)

add_test(
    NAME multiple_chunk_multiple_var_xor_checkpoint_recover
    COMMAND test_runner.sh 100 100 1 ${PROJECT_SOURCE_DIR}/tests/config.ini 1
)


