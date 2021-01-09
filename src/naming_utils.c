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

#include <naming_utils.h>


void generate_metafilepath(char* metapath, int version) {
    sprintf(metapath, "%s/meta-%d-%d.dat", dirpath, version%cpcount, rank);
}

void generate_cpfilepath(char* cppath, int version) {
    sprintf(cppath, "%s/cp-%d-%d.dat", dirpath, version%cpcount, rank);
}

int is_myfile(char *filename) {

    char mysigniture[MAX_FILE_NAME_SIZE];
    int signlen = sprintf(mysigniture, "-%d.dat", rank);

    int len = strlen(filename);
    int offset = len - signlen;

    return strncmp(filename+offset, mysigniture, signlen) == 0;
}
