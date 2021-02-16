#include <assert.h> 
#include <mpi.h>
#include <combined.h>
#include <xor_struct.h>
#include <mpi_utils.h>
#include <stdlib.h>

#define assert_eq_long(v1, v2) do { \
    if (v1 != v2) { \
        fprintf (stderr, "%s:%d: %ld != %ld\n", \
                __FILE__, __LINE__, (long)v1, (long)v2); \
    } \
    assert (v1 == v2); \
    } while(0)

#define assert_eq_long_id(v1, v2, id) do { \
    if (v1 != v2) { \
        fprintf (stderr, "%s:%d: %ld != %ld, at %d\n", \
                __FILE__, __LINE__, (long)v1, (long)v2, id); \
    } \
    assert (v1 == v2); \
    } while(0)

#define assert_eq_byte(v1, v2) do { \
    if (v1 != v2) { \
        fprintf (stderr, "%s:%d: %u != %u\n", \
                __FILE__, __LINE__, (uint8_t)v1, (uint8_t)v2); \
    } \
    assert (v1 == v2); \
    } while(0)

#define assert_eq_byte_id(v1, v2, id) do { \
    if (v1 != v2) { \
        fprintf (stderr, "%s:%d: %u != %u, at %d\n", \
                __FILE__, __LINE__, (uint8_t)v1, (uint8_t)v2, id); \
    } \
    assert (v1 == v2); \
    } while(0)

int main (int argc, char *argv[]) {

    if (argc < 2) {
      	printf("Usage: %s <config_file>\n", argv[0]);
      	exit(1);
    }
    int rank, ranks, i;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &ranks);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm comm = MPI_COMM_WORLD;

    // ####### Initialize Test ########
    assert (COMB_Init (comm, argv[1]) == SUCCESS);

    size_t size = 100*sizeof(char);
    unsigned char *buffer = (unsigned char *) malloc (size);
    memset (buffer, (xrank+1), size);

    assert (COMB_Protect (buffer, size) == SUCCESS);

    // ###### Test memcpy_from_vars ######
    data_t *prot_data = protected_data;
    size_t offset = 0;
    unsigned char *tmp = (unsigned char *) malloc (size);
    memcpy_from_vars (&prot_data, &offset, tmp, 26);
    memcpy_from_vars (&prot_data, &offset, tmp+26, 28);
    memcpy_from_vars (&prot_data, &offset, tmp+54, 14);
    memcpy_from_vars (&prot_data, &offset, tmp+68, 17);
    memcpy_from_vars (&prot_data, &offset, tmp+85, 15);
    for (i = 0; i<size; i++) {
        assert_eq_byte_id (buffer[i], tmp[i], i);
    }
    free (tmp);

    xorstruct = (xorstruct_t *) malloc(sizeof(xorstruct_t)); 

    compute_xstruct(&xorstruct, totaldatasize);

    assert_eq_long (xorstruct->xorparitysize, 40);
    assert_eq_long (xorstruct->chunksize, 160);
    assert_eq_long (xorstruct->chunkdatasize, 120);
    assert_eq_long_id (xorstruct->marginsize, 20, rank);
    assert_eq_long (xorstruct->chunkxoffset, xrank*40);
    assert (xorstruct->remaining_xorstruct == NULL);

    int recvcounts[xranks];
    uint8_t exp = 0;
    size_t chunk_elements = xorstruct->xorparitysize/basesize; 
    for (i = 0; i<xranks; i++) { 
        recvcounts[i] = chunk_elements; 
        exp = exp ^ (i+1);
    }

    unsigned char *test_chunk_cp = (unsigned char *) malloc(xorstruct->chunksize);
    offset = 0;
    fill_xor_chunk(test_chunk_cp, &prot_data, &offset, NULL, xorstruct);
    
    size_t offset_xor_seg_start = xrank*xorstruct->xorparitysize;
    size_t offset_xor_seg_end = xrank*xorstruct->xorparitysize + xorstruct->xorparitysize;
    size_t offset_meaningfull_data_end = xorstruct->chunksize-xorstruct->marginsize;

    for (i = 0; i<xorstruct->chunksize; i++) {
        if (i < offset_xor_seg_start && i < size)
            assert_eq_byte_id (test_chunk_cp[i], (xrank+1), i); 
        else if (offset_xor_seg_start <= i && i < offset_xor_seg_end)
            assert_eq_byte_id (test_chunk_cp[i], 0, i); 
        else if (offset_xor_seg_end <= i && i < offset_meaningfull_data_end)
            assert_eq_byte_id (test_chunk_cp[i], (xrank+1), i);
        else
            assert_eq_byte_id (test_chunk_cp[i], 0, i); 
    }

    unsigned char *xorparity = (unsigned char *) malloc (xorstruct->xorparitysize);

    int rc = MPI_Reduce_scatter(test_chunk_cp, xorparity, recvcounts, MPI_LONG, xor_op, xcomm);
    assert (rc == MPI_SUCCESS);

    //for (int lostxrank = 0; lostxrank<xranks; lostxrank++) {
    int lostxrank = 0;
        unsigned char *test_chunk_rec = (unsigned char *) malloc (xorstruct->chunksize);
        prot_data = protected_data;
        offset = 0;

        if (xrank == lostxrank) {
            unsigned char *recv_chunk = (unsigned char *) malloc (xorstruct->chunksize);
            unsigned char *tmpxor = (unsigned char *) malloc (xorstruct->xorparitysize);

            fill_xor_chunk(test_chunk_rec, NULL, &offset, NULL, xorstruct);
            rc = MPI_Reduce(test_chunk_rec, recv_chunk, chunk_elements, MPI_LONG, xor_op, lostxrank, xcomm);
            printf("achieved\n");
            for (i = 0; i<xorstruct->chunksize; i++) {
                printf("%u", recv_chunk[i]);
            }
            printf ("\nerwartet\n");
            for (i = 0; i<xorstruct->chunksize; i++) {
                printf("%u", test_chunk_cp[i]);
            }
            printf("\n");
            for (i = 0; i<xorstruct->chunksize; i++) {
                if (i < offset_xor_seg_start && i < size)
                    assert_eq_byte_id (recv_chunk[i], test_chunk_cp[i], i); 
                else if (offset_xor_seg_start <= i && i < offset_xor_seg_end)
                    assert_eq_byte_id (recv_chunk[i], xorparity[i-offset_xor_seg_start], i); 
                else if (offset_xor_seg_end <= i && i < offset_meaningfull_data_end)
                    assert_eq_byte_id (recv_chunk[i], test_chunk_cp[i], i);
                else
                    assert_eq_byte_id (recv_chunk[i], 0, i); 
            }

            //extract_xor_chunk(&prot_data, &offset, tmpxor, recv_chunk, xorstruct);
            free (recv_chunk);
            free (tmpxor);

        }
        else {
            fill_xor_chunk(test_chunk_rec, &prot_data, &offset, xorparity, xorstruct);
            rc = MPI_Reduce(test_chunk_rec, NULL, chunk_elements, MPI_LONG, xor_op, lostxrank, xcomm);
        }
        free (test_chunk_rec);
    //}

    free (test_chunk_cp);

    
    assert (COMB_Finalize() == SUCCESS);

    MPI_Finalize();
    free (buffer);

    return 0;
}
