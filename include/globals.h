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

#ifndef GLOBALS_H
#define GLOBALS_H

#include <mpi.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <limits.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <math.h>

#define SUCCESS 0
#define FAILED -1

#define NODATA 1
#define XORDATA 2
#define FULLDATA 3

#define MAX_CHUNK_ELEMENTS 200000
#define MAX_FILE_NAME_SIZE 30
#define FILE_OPEN_WRITE_FLAGS O_WRONLY | O_CREAT | O_TRUNC
#define FILE_OPEN_READ_FLAGS O_RDONLY

#define FAIL_IF_UNEXPECTED(rc, expected, msg)\
    if (check_code_failed(rc, expected, msg) != SUCCESS) {\
            return FAILED;\
    }

typedef struct xorstruct_type {
    size_t chunkxoffset;
    size_t chunkxparitysize;
    size_t chunkdatasize;
    size_t realdatasize;
    size_t chunksize;
    size_t xorparitysize;
    size_t marginsize;
    struct xorstruct_type *remaining_xorstruct;
} xorstruct_t;

typedef struct datalinkedlist {
    unsigned char *rankdata;
    size_t rankdatarealbcount;
    struct datalinkedlist *next;
} data_t;

typedef struct cplinkedlist {
    data_t *data;
    xorstruct_t *xorstruct;
    unsigned char *xorparity;
    size_t filesize;
    int version;
    int state;
    ssize_t failoffset;
    int loaded;
    struct cplinkedlist *next;
} cps_t;



data_t *protected_data;
xorstruct_t *xorstruct;
cps_t *cps;
char* dirpath;
unsigned char *chunk;
size_t basesize, dirpathsize, filepathsize, metadatasize, totaldatasize;
int rank, ranks, crank, cranks, prank, pranks, xrank, xranks;
int cpcount, version, fd;
int lostpgroup; // to see if there is any lost partner group to load the xor data in the first recovery phase
MPI_Comm ccomm, pcomm, xcomm, interccomm;
MPI_Op xor_op, intersect_op;

#endif
