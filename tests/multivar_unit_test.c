#include <multivar_xor_cp_recover.h>

int main (int argc, char *argv[]) {

    if (argc < 3) {
      	printf("Usage: %s <mem_in_bytes> <config_file>\n", argv[0]);
      	exit(1);
    }
    if (sscanf(argv[1], "%ld", &size) != 1) {
        printf("Wrong memory size! See usage\n");
	      exit(3);
    }

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &ranks);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm comm = MPI_COMM_WORLD;

    // ####### Initialize Test ########
    assert (COMB_Init (comm, argv[2]) == SUCCESS);
    
    xor_cp_recover_multiple_chunk_multiple_var_test ();
    
    // ###### Test Finalize ######
    assert (COMB_Finalize() == SUCCESS);

    MPI_Finalize();

    return 0;
}
