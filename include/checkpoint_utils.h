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

#ifndef CHECKPOINT_UTILS_H
#define CHECKPOINT_UTILS_H

#include <globals.h>
#include <common.h>
#include <mpi_utils.h>
#include <xor_struct.h>

int xor_checkpoint(cps_t **cp);
int partner_checkpoint(cps_t **cp);
int all_transfer_and_write(cps_t **cp, int fd);
int write_data(int fd, void *data, size_t size);
int write_metadata_file(cps_t *cp);
size_t fit_datasize(size_t datasize, int segments, size_t basesize, int increase_size);

#endif
