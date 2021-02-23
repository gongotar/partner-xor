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

#include <xor_units.h>

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

    if (size <= MAX_CHUNK_ELEMENTS*basesize) {
        xor_cp_recover_single_chunk_single_var_test ();
    }
    else{
        xor_cp_recover_multiple_chunk_single_var_test ();
    }
    
    // ###### Test Finalize ######
    assert (COMB_Finalize(1) == SUCCESS);

    MPI_Finalize();

    return 0;
}
