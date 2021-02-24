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

#ifndef COMBINED_H
#define COMBINED_H

#include <mpi.h>


#ifdef __cplusplus
extern "C" {
#endif

#define COMB_SUCCESS 0
#define COMB_FAILED -1

// interface

/**
Create the communicators of partner, XOR, and
community groups. initialize the environment for
creating combined checkpoints.
*/
int COMB_Init(MPI_Comm comm, char *config_file);

/**
Mark the data to be protected in the combined checkpoints.
*/
int COMB_Protect(void *data, size_t size);

/**
Recovers from the most recent recoverable checkpoint.
Restart will be filled with the found restart version.
*/
int COMB_Recover(int *restart);

/**
Create combined checkpoints using the protected data.
*/
int COMB_Checkpoint();

/**
Finalize the communicators, optionally clean up the memory.
*/
int COMB_Finalize(int cleanup);


#ifdef __cplusplus
}
#endif

#endif
