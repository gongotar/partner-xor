// Copyright 2020-2021 Zuse Institute Berlin
//
//    Licensed under the Apache License, Version 2.0 (the "License");
//    you may not use this file except in compliance with the License.
//    You may obtain a copy of the License at
//
//        http://www.apache.org/licenses/LICENSE-2.0
//
//    Unless required by applicable law or agreed to in writing, software
//    distributed under the License is distributed on an "AS IS" BASIS,
//    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//    See the License for the specific language governing permissions and
//    limitations under the License.

#include <xor_cp_recover.h>

void memcpy_from_vars_test (unsigned char *buffer) {
    data_t *prot_data = protected_data;
    size_t offset = 0;
    unsigned char *tmp = (unsigned char *) malloc (size);
    memcpy_from_vars (&prot_data, &offset, tmp, 26);
    memcpy_from_vars (&prot_data, &offset, tmp+26, 28);
    memcpy_from_vars (&prot_data, &offset, tmp+54, 14);
    memcpy_from_vars (&prot_data, &offset, tmp+68, 17);
    memcpy_from_vars (&prot_data, &offset, tmp+85, 15);
    memcpy_from_vars (&prot_data, &offset, tmp+100, size-100);
    
    for (int i = 0; i<size; i++) {
        assert_eq_byte_id (buffer[i], tmp[i], i);
    }
    free (tmp);
}

void xor_cp_recover_multiple_chunk_single_var_test () {
    
    unsigned char *buffer = (unsigned char *) malloc (size);
    memset (buffer, (xrank+1), size);

    assert (COMB_Protect (buffer, size) == SUCCESS);
    
    // ###### Simple test memcpy_from_vars ######
    memcpy_from_vars_test (buffer);



}

void xor_cp_recover_single_chunk_single_var_test () {
    int i;
    unsigned char *buffer = (unsigned char *) malloc (size);
    memset (buffer, (xrank+1), size);

    assert (COMB_Protect (buffer, size) == SUCCESS);
    
    // ###### Simple test memcpy_from_vars ######
    memcpy_from_vars_test (buffer);

    // ###### Test the constructed XOR structure ######
    
    // generate the XOR structure
    xorstruct = (xorstruct_t *) malloc(sizeof(xorstruct_t)); 
    compute_xstruct(&xorstruct, totaldatasize);

    // manually create the expected XOR structure
    size_t expected_paritysize = (size_t) totaldatasize/(xranks-1);
    if (totaldatasize % (xranks-1))
        expected_paritysize ++;
    while (expected_paritysize % basesize)
        expected_paritysize ++;
    size_t expected_margin = (xranks-1)*expected_paritysize - totaldatasize;
    size_t expected_chunkdatasize = size;
    while (expected_chunkdatasize % expected_paritysize)
        expected_chunkdatasize ++;

    // check that the expectations match with the generted XOR structure
    assert_eq_long (xorstruct->xorparitysize, expected_paritysize);
    assert_eq_long (xorstruct->chunksize, xranks*expected_paritysize);
    assert_eq_long (xorstruct->chunkdatasize, expected_chunkdatasize);
    assert_eq_long (xorstruct->marginsize, expected_margin);
    assert_eq_long (xorstruct->chunkxoffset, xrank*expected_paritysize);
    assert (xorstruct->remaining_xorstruct == NULL);

    // ###### Test fill_xor_chunk function ######

    // initialize/create variables
    unsigned char *test_chunk_cp = (unsigned char *) malloc(xorstruct->chunksize);
    offset = 0;
    prot_data = protected_data;

    // fill the sample chunk
    fill_xor_chunk(test_chunk_cp, &prot_data, &offset, NULL, xorstruct);
    
    // manually create the expected XOR chunk and match with the generated chunk
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

    // ###### Test XOR checkpoint generation ######

    // initialize/create variables
    int recvcounts[xranks];
    size_t parity_elements = xorstruct->xorparitysize/basesize; 
    unsigned char *xorparity = (unsigned char *) malloc (xorstruct->xorparitysize);
    for (i = 0; i<xranks; i++) { 
        recvcounts[i] = parity_elements; 
    }

    // perform XOR checkpoint
    int rc = MPI_Reduce_scatter(test_chunk_cp, xorparity, recvcounts, MPI_LONG, xor_op, xcomm);

    assert (rc == MPI_SUCCESS);

    // ###### Test XOR recovery ######

    // generate XOR recovery for each rank and match the result 
    // to the computed XOR checkpoint of the previous test
    size_t chunk_elements = xorstruct->chunksize/basesize;

    for (int lostxrank = 0; lostxrank<xranks; lostxrank++) {

        // initialize/create the variables
        unsigned char *test_chunk_rec = (unsigned char *) malloc (xorstruct->chunksize);
        prot_data = protected_data;
        offset = 0;

        if (xrank == lostxrank) {   
            // lost rank, recover and compare to the actual checkpoint

            // initialize a temporary environment for the recovery
            unsigned char *recv_chunk = (unsigned char *) malloc (xorstruct->chunksize);
            unsigned char *tmpxor = (unsigned char *) malloc (xorstruct->xorparitysize);
            fill_xor_chunk(test_chunk_rec, NULL, &offset, NULL, xorstruct);

            // perform the recovery
            rc = MPI_Reduce(test_chunk_rec, recv_chunk, chunk_elements, MPI_LONG, xor_op, lostxrank, xcomm);

            // match the recovered data with the generated checkpoint 
            // of the previous test
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

            // ###### Test extract_xor_chunk ######
            
            // perform the function to extract data and XOR parity from the 
            // received chunk
            extract_xor_chunk(&prot_data, &offset, tmpxor, recv_chunk, xorstruct);

            // match the extracted data and XOR parity with the 
            // recovered extracted values
            for (i = 0; i<size; i++) {
                assert_eq_byte_id (buffer[i], (xrank+1), i);
            }
            for (i = 0; i<xorstruct->xorparitysize; i++) {
                assert_eq_byte_id (tmpxor[i], xorparity[i], i);
            }

            free (recv_chunk);
            free (tmpxor);

        }
        else {
            fill_xor_chunk(test_chunk_rec, &prot_data, &offset, xorparity, xorstruct);
            rc = MPI_Reduce(test_chunk_rec, NULL, chunk_elements, MPI_LONG, xor_op, lostxrank, xcomm);
        }
        free (test_chunk_rec);
        MPI_Barrier (xcomm);
    }

    free (test_chunk_cp);
    free (buffer);
}
