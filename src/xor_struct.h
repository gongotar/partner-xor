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

#ifndef XOR_STRUCT_H
#define XOR_STRUCT_H

#include <globals.h>
#include <common.h>

void compute_xstruct(xorstruct_t **xstruct, size_t datasize);
size_t fill_xor_chunk(void *chunk, data_t **data, size_t *offset, 
        void *parity, xorstruct_t *xstruct);
void extract_xor_chunk(data_t **data, size_t * offset, void *parity, 
        void *chunk, xorstruct_t *xstruct);


#endif
