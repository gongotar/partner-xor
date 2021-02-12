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

#ifndef COMMON_H
#define COMMON_H

#include <globals.h>
#include <naming_utils.h>

int check_code_failed(long rc, long expected, char* failure);
void make_hashset(int *hashset, int hashcount, int *arr, int arrcount);
int free_cps_mem();
int free_cps_files();

// assets
void num_to_bytes(unsigned char* dest, unsigned long num, size_t size);
unsigned long bytes_to_num(unsigned char* bytes, size_t size);
void opt_memcpy(void *dest, void const *src, size_t size);
void opt_memset_zero(void *dest, size_t size);
void memcpy_from_vars (data_t **data, size_t *offset, unsigned char *destbuffer, size_t copysize);
void memcpy_to_vars (unsigned char *srcbuffer, data_t **data, size_t *offset, size_t copysize);
#endif
