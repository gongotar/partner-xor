add_library(${PROJECT_NAME} SHARED 
    combined.c 
    common.c
    checkpoint_utils.c
    naming_utils.c
    xor_struct.c
    recovery_utils.c
    extra.c
    mpi_utils.c
    ini.c
)

install(TARGETS ${PROJECT_NAME} DESTINATION lib)
install(FILES ${PROJECT_SOURCE_DIR}/include/combined.h DESTINATION include)
