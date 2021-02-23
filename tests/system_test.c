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

int main (int argc, char *argv[]) {

    int method;

    if (argc < 4) {
      	printf("Usage: %s <method> <mem_in_bytes> <config_file>\n", argv[0]);
      	exit(1);
    }
    if (sscanf(argv[1], "%d", &method) != 1) {
        printf("Wrong method! See usage\n");
	    exit(3);
    }
    if (sscanf(argv[2], "%ld", &size) != 1) {
        printf("Wrong memory size! See usage\n");
	    exit(3);
    }

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &ranks);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm comm = MPI_COMM_WORLD;

    if (method == 0) {
        api_prot_cp (comm, argv[3]);
    }
    else if (method == 1) {
        api_prot_rec (comm, argv[3]);
    }
    else {
        printf("Wrong method! See usage\n");
	    exit(3);
    }

    MPI_Finalize();

    return 0;
}
