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

#include <cp_rec_system.h>

void create_variables () {
    var1_size = 4;
    var2_size = (size_t) ((size - 4) / 2);
    var3_size = (size_t) ((size - 4) / 2);

    var1 = (unsigned char *) malloc (var1_size);
    var2 = (unsigned char *) malloc (var2_size);
    var3 = (unsigned char *) malloc (var3_size);
}

void init_variables () {
    int data1 = ranks + rank;
    memcpy (var1, &data1, var1_size);
    memset (var2, (rank+2*ranks), var2_size);
    memset (var3, (rank+3*ranks), var3_size);
}

void api_prot_cp (MPI_Comm comm, char *config) {

    // ####### Initialize Test ########
    assert (COMB_Init (comm, config) == SUCCESS);
    
    // create variables
    create_variables ();

    // protect variabless
    assert (COMB_Protect (var3, var3_size) == SUCCESS);
    assert (COMB_Protect (var2, var2_size) == SUCCESS);
    assert (COMB_Protect (var1, var1_size) == SUCCESS);

    // fill the variables
    init_variables ();

    // create the checkpoint
    assert (COMB_Checkpoint() == SUCCESS);
    assert (COMB_Checkpoint() == SUCCESS);

    int clearing_xrank = size%xranks;

    // ###### Test Finalize ######
    assert (COMB_Finalize((xrank == clearing_xrank)) == SUCCESS);
}

void api_prot_rec (MPI_Comm comm, char *config) {

    // ####### Initialize Test ########
    assert (COMB_Init (comm, config) == SUCCESS);

    // create variables
    create_variables ();

    // protect variabless
    assert (COMB_Protect (var3, var3_size) == SUCCESS);
    assert (COMB_Protect (var2, var2_size) == SUCCESS);
    assert (COMB_Protect (var1, var1_size) == SUCCESS);

    // recover the job
    int restart;
    assert (COMB_Recover(&restart) == SUCCESS);

    // test the recovered values
    assert_eq_int_id (restart, 2, rank);
    int data1 = ranks + rank;
    assert_eq_int (*((int *)var1), data1);
    
    // ###### Test Finalize ######
    assert (COMB_Finalize(1) == SUCCESS);

}
