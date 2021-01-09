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

#include <extra.h>


// API test functions
int generate_data(float* data, unsigned long count, int rank, int version) {
    register long i;
    register float isq;
    register float* tmp = data;
    for (i = 0; i<count; i++) {
        isq = sqrt(i+1);
        *tmp = (rank+1)*isq*cos((version+1)*isq);
        tmp ++;
    }
    return 0;
}


int integrity_check(float *data, unsigned long count, int rank, int version) {

    register long i;
    register float isq, expected;
    register float* tmp = data;
    for (i = 0; i<count; i++) {
        isq = sqrt(i+1);
        expected = (rank+1)*isq*cos((version+1)*isq);
        if (*tmp != expected) {
            printf("integrity failed at %ld from %ld\n", i, count);
            return FAILED;
        }
        tmp ++;
    }
    return SUCCESS;
}

// unused functions

int equal_data(unsigned long* data1, unsigned long* data2, size_t count) {
    register long i;
    register unsigned long *tmp1 = data1, *tmp2 = data2;
    for (i=0; i<count; i++) {
        if (*tmp1 != *tmp2) {
            return 0;
        }
        tmp1 ++;
        tmp2 ++;
    }
    return 1;
}
