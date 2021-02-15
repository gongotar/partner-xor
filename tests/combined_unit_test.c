#include <assert.h> 
#include <mpi.h>
#include <combined.h>

char path[20] = "/local_ssd/bzcghola";

int main (int argc, char *argv[]) {

    if (argc < 2) {
      	printf("Usage: %s <config_file>\n", argv[0]);
      	exit(1);
    }
    int rank, ranks;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &ranks);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm comm = MPI_COMM_WORLD;

    assert(COMB_Init(comm, argv[1]) == SUCCESS);
    assert(COMB_Finalize() == SUCCESS);

    MPI_Finalize();

    return 0;
}
