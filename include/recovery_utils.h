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

#ifndef RECOVERY_UTILS_H
#define RECOVERY_UTILS_H

#include <globals.h>
#include <common.h>
#include <mpi_utils.h>
#include <xor_struct.h>
#include <checkpoint_utils.h>

int global_negotiate_version(int* version, int* lostpgroup);
int assign_partner(int pstates[]);
int partner_recover(cps_t** cp, int partner);
int xor_recover(cps_t** cp, int failedrank);
int exchange_local_cps(cps_t** cp);
int theoretical_repair(int* ccommstates);
int version_state_in_presented_versions(int version, int *rankversions, int *rankfullversions);
int pair_transfer_and_write(cps_t **cp, int fd, int partner, int memload, int xortransfer);
int imbalance_partners(int pstates[]);
int load_local_metadata();
int load_local_checkpoint(cps_t** cp, size_t* offset);
int is_metafile(char* filename);

#endif
