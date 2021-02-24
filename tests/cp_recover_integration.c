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

#include <cp_recover_integration.h>

void init_variables () {
    var1_size = 4;
    var2_size = (size_t) ((size - 4) / 2);
    var3_size = (size_t) ((size - 4) / 2);

    var1 = (unsigned char *) malloc (var1_size);
    var2 = (unsigned char *) malloc (var2_size);
    var3 = (unsigned char *) malloc (var3_size);

    int data1 = ranks + rank;
    memcpy (var1, &data1, var1_size);
    memset (var2, (rank+2*ranks), var2_size);
    memset (var3, (rank+3*ranks), var3_size);

    
    assert (COMB_Protect (var3, var3_size) == SUCCESS);
    assert (COMB_Protect (var2, var2_size) == SUCCESS);
    assert (COMB_Protect (var1, var1_size) == SUCCESS);

    data_t *prot_data = protected_data;
    size_t offset;


    // ####### TEST memcpy_from_vars ########
    offset = 0;
    unsigned char *tmp_vars = (unsigned char *) malloc (size);

    memcpy_from_vars (&prot_data, &offset, tmp_vars, 26);
    memcpy_from_vars (&prot_data, &offset, tmp_vars+26, 28);
    memcpy_from_vars (&prot_data, &offset, tmp_vars+54, 14);
    memcpy_from_vars (&prot_data, &offset, tmp_vars+68, 17);
    memcpy_from_vars (&prot_data, &offset, tmp_vars+85, 15);
    memcpy_from_vars (&prot_data, &offset, tmp_vars+100, size-100);

    for (int i=0; i<size; i++) {
        if (i < var1_size) {
            assert_eq_byte_id (var1[i], tmp_vars[i], i);
        }
        else if (i < var1_size+var2_size) {
            assert_eq_byte_id ((var2[i-var1_size]), tmp_vars[i], i);
        }
        else {
            assert_eq_byte_id ((var3[i-var1_size-var2_size]), tmp_vars[i], i);
        }
    }
}

void partner_cp_recover_multiple_chunk_multiple_var_test () {
    init_variables ();
    
    // generate the XOR structure
    xorstruct = (xorstruct_t *) malloc(sizeof(xorstruct_t)); 
    compute_xstruct(&xorstruct, totaldatasize);

    // ########### TEST partner checkpoint ################

    cps_t *cp = (cps_t*) malloc(sizeof(cps_t));
    cp->version = 1;
    cp->state = NODATA;
    cp->xorstruct = xorstruct;
    cp->data = protected_data;
    cp->loaded = 0;
    cp->filesize = 0;

    free_cps_files();

    assert (xor_checkpoint (&cp) == SUCCESS); 
    cp->state = XORDATA;
    assert (partner_checkpoint (&cp) == SUCCESS);
    cp->state = FULLDATA;


    // ############ TEST Recovery for all ranks ###########
    for (int lostprank = 0; lostprank < pranks; lostprank++) {
        if (prank == lostprank) {
            cp->loaded = 0;
            memset (var1, '\0', var1_size);
            memset (var2, '\0', var2_size);
            memset (var3, '\0', var3_size);
            memset (cp->xorparity, '\0', cp->xorstruct->xorparitysize);
            free_cps_files();
            cp->filesize = 0;
            cp->state = NODATA;
        }
        int rescuer = (lostprank + 1) % pranks;
        if (prank == lostprank || prank == rescuer) {
            int partner = (prank == lostprank) ? rescuer:lostprank;
            assert (partner_recover (&cp, partner) == SUCCESS);
        }
        if (prank == lostprank) {
            cp->state = FULLDATA;
            int data1 = ranks + rank;
            assert_eq_int (*((int *)var1), data1);
            for (int i=0; i<size; i++) {
                if (i < var1_size) {
                }
                else if (i < var1_size+var2_size) {
                    assert_eq_byte_id ((var2[i-var1_size]), (rank+2*ranks), i);
                }
                else {
                    assert_eq_byte_id ((var3[i-var1_size-var2_size]), (rank+3*ranks), i);
                }
            }
        }
        MPI_Barrier (pcomm);
    }

}

void xor_cp_recover_multiple_chunk_multiple_var_test () {
    
    init_variables ();

    // ###### Test the constructed XOR structure ######

    // generate the XOR structure
    xorstruct = (xorstruct_t *) malloc(sizeof(xorstruct_t)); 
    compute_xstruct(&xorstruct, totaldatasize);
    size_t maxchunkb = MAX_CHUNK_ELEMENTS*basesize;
    if (totaldatasize > maxchunkb) {
        // manually create the expected XOR structure
        // main chunk
        size_t expected_segmentsize = (size_t) maxchunkb/(xranks-1);
        while (expected_segmentsize % basesize)
            expected_segmentsize --;
        size_t expected_chunkdatasize = (xranks-1) * expected_segmentsize; // since all segments must be filled
        size_t expected_paritysize = ((size_t) (size/expected_chunkdatasize))*expected_segmentsize;
        size_t expected_margin = 0;
        // rest chunk
        size_t expected_realsize_rest = totaldatasize % expected_chunkdatasize;
        if (expected_realsize_rest > 0) {
            size_t expected_paritysize_rest = (size_t) expected_realsize_rest / (xranks-1);
            if (expected_realsize_rest % (xranks-1))
                expected_paritysize_rest ++;
            while (expected_paritysize_rest % basesize)
                expected_paritysize_rest ++;
            size_t expected_chunkdatasize_rest = expected_realsize_rest;
            while (expected_chunkdatasize_rest % expected_paritysize_rest)
                expected_chunkdatasize_rest ++;
            size_t expected_margin_rest = (xranks-1)*expected_paritysize_rest - expected_realsize_rest;
            expected_paritysize += expected_paritysize_rest;
            
            xorstruct_t *rxorstruct = xorstruct->remaining_xorstruct;

            assert_eq_long (rxorstruct->xorparitysize, expected_paritysize_rest);
            assert_eq_long (rxorstruct->chunksize, xranks*expected_paritysize_rest);
            assert_eq_long (rxorstruct->chunkdatasize, expected_chunkdatasize_rest);
            assert_eq_long (rxorstruct->marginsize, expected_margin_rest);
            assert_eq_long (rxorstruct->chunkxoffset, xrank*expected_paritysize_rest);
            assert_eq_long (rxorstruct->chunkxparitysize, expected_paritysize_rest);
        }
        else {
            assert (xorstruct->remaining_xorstruct == NULL);
        }

        // check that the expectations match with the generted XOR structure
        assert_eq_long (xorstruct->chunksize, xranks*expected_segmentsize);
        assert_eq_long (xorstruct->chunkdatasize, expected_chunkdatasize);
        assert_eq_long (xorstruct->marginsize, expected_margin);
        assert_eq_long (xorstruct->chunkxoffset, xrank*expected_segmentsize);
        assert_eq_long (xorstruct->xorparitysize, expected_paritysize);
        assert_eq_long (xorstruct->chunkxparitysize, expected_segmentsize);
    }
    else {
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
        assert_eq_long (xorstruct->chunkxparitysize, expected_paritysize);
        assert (xorstruct->remaining_xorstruct == NULL);
    }

    // ########### TEST XOR checkpoint ################

    cps_t *cp = (cps_t*) malloc(sizeof(cps_t));
    cp->version = 0;
    cp->state = NODATA;
    cp->xorstruct = xorstruct;
    cp->data = protected_data;

    assert (xor_checkpoint(&cp) == SUCCESS); 

    // ############ TEST Recovery for all ranks ###########
    for (int lostxrank = 0; lostxrank < xranks; lostxrank++) {
        if (xrank == lostxrank) {
            memset (var1, '\0', var1_size);
            memset (var2, '\0', var2_size);
            memset (var3, '\0', var3_size);
        }
        assert (xor_recover(&cp, lostxrank) == SUCCESS);
        if (xrank == lostxrank) {
            int data1 = ranks + rank;
            assert_eq_int (*var1, data1);
            for (int i=0; i<size; i++) {
                if (i < var1_size) {
                }
                else if (i < var1_size+var2_size) {
                    assert_eq_byte_id ((var2[i-var1_size]), (rank+2*ranks), i);
                }
                else {
                    assert_eq_byte_id ((var3[i-var1_size-var2_size]), (rank+3*ranks), i);
                }
            }
        }
    }


}










