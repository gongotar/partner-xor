#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include <combined.h>
#include "heatdis.h"

/*
    This sample application is based on the heat distribution code
    originally developed within the FTI project: github.com/leobago/fti
*/

#define CP_INTERVAL    30

void initData(int nbLines, int M, int rank, double *h) {
    int i, j;
    for (i = 0; i < nbLines; i++) {
        for (j = 0; j < M; j++) {
            h[(i*M)+j] = 0;
        }
    }
    if (rank == 0) {
        for (j = (M*0.1); j < (M*0.9); j++) {
            h[j] = 100;
        }
    }
}

double doWork(int numprocs, int rank, int M, int nbLines, double *g, double *h) {
    int i,j;
    MPI_Request req1[2], req2[2];
    MPI_Status status1[2], status2[2];
    double localerror;
    localerror = 0;
    for(i = 0; i < nbLines; i++) {
        for(j = 0; j < M; j++) {
            h[(i*M)+j] = g[(i*M)+j];
        }
    }
    if (rank > 0) {
        MPI_Isend(g+M, M, MPI_DOUBLE, rank-1, WORKTAG, MPI_COMM_WORLD, &req1[0]);
        MPI_Irecv(h,   M, MPI_DOUBLE, rank-1, WORKTAG, MPI_COMM_WORLD, &req1[1]);
    }
    if (rank < numprocs - 1) {
        MPI_Isend(g+((nbLines-2)*M), M, MPI_DOUBLE, rank+1, WORKTAG, MPI_COMM_WORLD, &req2[0]);
        MPI_Irecv(h+((nbLines-1)*M), M, MPI_DOUBLE, rank+1, WORKTAG, MPI_COMM_WORLD, &req2[1]);
    }
    if (rank > 0) {
        MPI_Waitall(2,req1,status1);
    }
    if (rank < numprocs - 1) {
        MPI_Waitall(2,req2,status2);
    }
    for(i = 1; i < (nbLines-1); i++) {
        for(j = 0; j < M; j++) {
            g[(i*M)+j] = 0.25*(h[((i-1)*M)+j]+h[((i+1)*M)+j]+h[(i*M)+j-1]+h[(i*M)+j+1]);
            if(localerror < fabs(g[(i*M)+j] - h[(i*M)+j])) {
                localerror = fabs(g[(i*M)+j] - h[(i*M)+j]);
            }
        }
    }
    if (rank == (numprocs-1)) {
        for(j = 0; j < M; j++) {
            g[((nbLines-1)*M)+j] = g[((nbLines-2)*M)+j];
        }
    }
    return localerror;
}

int main(int argc, char *argv[]) {
    int rank, nbProcs, nbLines, M, arg, cp_count = 0;
    double wtime, memSize, localerror, globalerror = 1;
    double st, dur = 0, totaldur;

    setbuf(stdout, NULL);
    if (argc < 3) {
      	printf("Usage: %s <mem_in_mb> <config_file>\n", argv[0]);
      	exit(1);
    }


    if (sscanf(argv[1], "%d", &arg) != 1) {
        printf("Wrong memory size! See usage\n");
	      exit(3);
    }

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &nbProcs);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm comm = MPI_COMM_WORLD;

    assert(COMB_Init(comm, argv[2]) == COMB_SUCCESS);


    M = (int)sqrt((double)(arg * 1024.0 * 1024.0 * nbProcs) / (2 * sizeof(double))); // two matrices needed
    nbLines = (M / nbProcs) + 3;

    double *h = (double *) malloc(sizeof(double *) * M * nbLines);
    double *g = (double *) malloc(sizeof(double *) * M * nbLines);
    int step; 

    COMB_Protect (h, sizeof(double *) * M * nbLines);
    COMB_Protect (g, sizeof(double *) * M * nbLines);
    COMB_Protect (&step, sizeof(int));

    // if there is a recovery, load it into the memory
    int restart;
    COMB_Recover(&restart);
    if (restart == 0) {
        step = 0;
        initData(nbLines, M, rank, g);
    }
    else {
        printf("restart found for rank %d version %d step %d\n", rank, restart, step);
    }

    memSize = M * nbLines * 2 * sizeof(double) / (1024 * 1024);

    if (rank == 0)
	       printf("Local data size is %d x %d = %f MB (%d).\n", M, nbLines, memSize, arg);
    if (rank == 0)
	       printf("Target precision : %f \n", PRECISION);
    if (rank == 0)
	       printf("Maximum number of iterations : %d \n", ITER_TIMES);

    wtime = MPI_Wtime();

    while(step < ITER_TIMES) {
        if ((step % CP_INTERVAL) == 0) {
            st = MPI_Wtime();    
            assert(COMB_Checkpoint() == COMB_SUCCESS);
            dur += (MPI_Wtime() - st);
            cp_count ++;
        }
        localerror = doWork(nbProcs, rank, M, nbLines, g, h);
        if ((step % REDUCE) == 0)
	         MPI_Allreduce(&localerror, &globalerror, 1, MPI_DOUBLE, MPI_MAX, MPI_COMM_WORLD);
        if (((step % ITER_OUT) == 0) && (rank == 0))
	         printf("Step : %d, error = %f\n", step, globalerror);
        if (globalerror < PRECISION)
	         break;
	      step ++;
    }
    
    MPI_Reduce(&dur, &totaldur, 1, MPI_DOUBLE, MPI_SUM, 0, comm);
    if (rank == 0) {
        printf("Avarage checkpointing duration %f\n", totaldur/(nbProcs*cp_count));
	    printf("Execution finished in %lf seconds.\n", MPI_Wtime() - wtime);
    }

    COMB_Finalize(1);
    MPI_Finalize();
    free(g);
    free(h);
    
    return 0;
}
