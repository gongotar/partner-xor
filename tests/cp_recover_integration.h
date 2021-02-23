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


#ifndef CP_RECOVER_INTEGRATION_C
#define CP_RECOVER_INTEGRATION_C

#include <test_header.h>
#include <checkpoint_utils.h>
#include <recovery_utils.h>

size_t var1_size;
size_t var2_size;
size_t var3_size;

unsigned char *var1;
unsigned char *var2;
unsigned char *var3;

void xor_cp_recover_multiple_chunk_multiple_var_test ();
void partner_cp_recover_multiple_chunk_multiple_var_test ();

#endif
