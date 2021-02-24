add_executable (unit_test
    unit_test.c
    xor_units.c)

add_executable (integration_test 
    integration_test.c
    cp_recover_integration.c)

add_executable (system_test 
    system_test.c
    cp_rec_system.c)

include_directories(
    ${MPI_INCLUDE_PATH}
    ${PROJECT_SOURCE_DIR}/tests
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(unit_test
    ${PROJECT_NAME} 
    ${MPI_C_LIBRARIES}
)

target_link_libraries(integration_test
    ${PROJECT_NAME} 
    ${MPI_C_LIBRARIES}
)

target_link_libraries(system_test
    ${PROJECT_NAME} 
    ${MPI_C_LIBRARIES}
)

configure_file(${PROJECT_SOURCE_DIR}/tests/test_runner.sh test_runner.sh COPYONLY)
configure_file(${PROJECT_SOURCE_DIR}/tests/config.ini config.ini COPYONLY)

add_test(
    NAME unit_test_single_chunk_single_var_xor_nano_sizes
    COMMAND test_runner.sh 100 1000 270 ${PROJECT_SOURCE_DIR}/tests/config.ini 0
)

add_test(
    NAME unit_test_single_chunk_single_var_xor_micro_sizes
    COMMAND test_runner.sh 1000 100000 18000 ${PROJECT_SOURCE_DIR}/tests/config.ini 0
)

add_test(
    NAME unit_test_single_chunk_single_var_xor_small_sizes
    COMMAND test_runner.sh 100000 1600000 250000 ${PROJECT_SOURCE_DIR}/tests/config.ini 0
)

add_test(
    NAME unit_test_multiple_chunk_single_var_xor_medium_sizes
    COMMAND test_runner.sh 1601000 20000000 5000000 ${PROJECT_SOURCE_DIR}/tests/config.ini 0
)

add_test(
    NAME unit_test_multiple_chunk_single_var_xor_large_sizes
    COMMAND test_runner.sh 100000000 300000000 70000000 ${PROJECT_SOURCE_DIR}/tests/config.ini 0
)

add_test(
    NAME integration_test_single_chunk_multiple_var_xor_nano_sizes
    COMMAND test_runner.sh 100 1000 270 ${PROJECT_SOURCE_DIR}/tests/config.ini 1
)

add_test(
    NAME integration_test_single_chunk_multiple_var_xor_micro_sizes
    COMMAND test_runner.sh 1000 100000 18000 ${PROJECT_SOURCE_DIR}/tests/config.ini 1
)

add_test(
    NAME integration_test_single_chunk_multiple_var_xor_small_sizes
    COMMAND test_runner.sh 100000 1600000 250000 ${PROJECT_SOURCE_DIR}/tests/config.ini 1
)

add_test(
    NAME integration_test_multiple_chunk_multiple_var_xor_medium_sizes
    COMMAND test_runner.sh 1601000 20000000 5000000 ${PROJECT_SOURCE_DIR}/tests/config.ini 1
)

add_test(
    NAME integration_test_multiple_chunk_multiple_var_xor_large_sizes
    COMMAND test_runner.sh 100000000 300000000 70000000 ${PROJECT_SOURCE_DIR}/tests/config.ini 1
)

add_test(
    NAME integration_test_single_chunk_multiple_var_partner_nano_sizes
    COMMAND test_runner.sh 100 1000 270 ${PROJECT_SOURCE_DIR}/tests/config.ini 2
)

add_test(
    NAME integration_test_single_chunk_multiple_var_partner_micro_sizes
    COMMAND test_runner.sh 1000 100000 18000 ${PROJECT_SOURCE_DIR}/tests/config.ini 2
)

add_test(
    NAME integration_test_single_chunk_multiple_var_partner_small_sizes
    COMMAND test_runner.sh 100000 1600000 250000 ${PROJECT_SOURCE_DIR}/tests/config.ini 2
)

add_test(
    NAME integration_test_multiple_chunk_multiple_var_partner_medium_sizes
    COMMAND test_runner.sh 1601000 20000000 5000000 ${PROJECT_SOURCE_DIR}/tests/config.ini 2
)

add_test(
    NAME system_test_simple_cp_recover_nano_size 
    COMMAND test_runner.sh 100 1000 270 ${PROJECT_SOURCE_DIR}/tests/config.ini 3
)

add_test(
    NAME system_test_simple_cp_recover_micro_size 
    COMMAND test_runner.sh 1000 100000 18000 ${PROJECT_SOURCE_DIR}/tests/config.ini 3
)

add_test(
    NAME system_test_simple_cp_recover_small_size 
    COMMAND test_runner.sh 100000 1600000 250000 ${PROJECT_SOURCE_DIR}/tests/config.ini 3
)

add_test(
    NAME system_test_simple_cp_recover_medium_size 
    COMMAND test_runner.sh 1601000 20000000 5000000 ${PROJECT_SOURCE_DIR}/tests/config.ini 3
)
