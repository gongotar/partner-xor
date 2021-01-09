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

#include <mpi_utils.h>

// MPI Operation
void compute_xor_op(unsigned long *in, unsigned long *inout, int *len, MPI_Datatype *dptr) {
    register long i;
    register int ln = *len;
    register unsigned long *rin = in, *rinout = inout;
    for (i = 0; i<ln; i++) {
        *rinout++ ^= *rin++;
    }
}

void intersect_list_op(int *in, int *inout, int *len, MPI_Datatype *dptr) {
    int hashset[cpcount+1]; // a hash containing all possible versions
    make_hashset(hashset, cpcount+1, in, *len);

    int i, key;
    for (i=0; i<*len; i++) {
        if (inout[i] == 0) continue;
        key = inout[i] % (cpcount+1);
        if (hashset[key] != inout[i]) {
            inout[i] = 0;
        }
    }
}
