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

#ifndef EXTRA_H
#define EXTRA_H

#include <globals.h>

// needed for the API test
int generate_data(float *data, unsigned long count, int rank, int version);
int integrity_check(float *data, unsigned long count, int rank, int version);

// extra unused functions
int equal_data(unsigned long* data1, unsigned long* data2, size_t count);


#endif
