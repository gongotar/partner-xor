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

add_test(
    NAME unit_test_2x4 
    COMMAND mpirun -n 8 combined_unit_test ${PROJECT_SOURCE_DIR}/example/config.ini
)

