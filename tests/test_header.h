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

#ifndef TEST_HEADER_H
#define TEST_HEADER_H

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

#define assert_eq_int(v1, v2) do { \
    if (v1 != v2) { \
        fprintf (stderr, "%s:%d: %d != %d\n", \
                __FILE__, __LINE__, (int)v1, (int)v2); \
    } \
    assert (v1 == v2); \
    } while(0)

#define assert_eq_int_id(v1, v2, id) do { \
    if (v1 != v2) { \
        fprintf (stderr, "%s:%d: %d != %d, at %d\n", \
                __FILE__, __LINE__, (int)v1, (int)v2, id); \
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

int rank, ranks;
size_t size;



#endif
