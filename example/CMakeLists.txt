add_executable (heat_dist heatdis_combined.c)

include_directories(
    ${MPI_INCLUDE_PATH}
    ${PROJECT_SOURCE_DIR}/include
)


target_link_libraries(heat_dist
    ${PROJECT_NAME} 
    ${MPI_C_LIBRARIES}
    m
)
