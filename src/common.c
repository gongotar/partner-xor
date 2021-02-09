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

#include <common.h>


int check_code_failed(long rc, long expected, char* failure) {

    if (rc == expected) {
        return SUCCESS;
    }
    else if (strncmp(failure, "MPI:", 4) == 0) {
        char msg[MPI_MAX_ERROR_STRING];
        int len, eclass;
        MPI_Error_class(rc, &eclass);
        MPI_Error_string(eclass, msg, &len);
        fprintf(stderr, "mpi failed, rc %ld, combined msg: %s,\n \
                errno %d, errno msg: %s,\n \
                mpi msg: %s\n", rc, failure, errno, strerror(errno),  msg);
    }
    else {
        fprintf(stderr, "rc %ld, expected %ld, combined msg: %s,\n \
                errno %d, errno msg: %s\n", rc, expected, failure, errno, strerror(errno));
    }

    free_cps_mem();
    if (chunk != NULL) {
        free(chunk);
    }

    if (xorstruct != NULL) {
        if (xorstruct->remaining_xorstruct != NULL) {
            free(xorstruct->remaining_xorstruct);
        }
        free(xorstruct);
        xorstruct = NULL;
    }

    close(fd);

    return FAILED;
}

void make_hashset(int *hashset, int hashcount, int *arr, int arrcount) {
    opt_memset_zero(hashset, hashcount*sizeof(int));
    int i;
    int v, hashkey;
    for (i = 0; i<arrcount; i++) {
        v = arr[i];
        if (v == 0) continue;
        hashkey = v%hashcount;
        hashset[hashkey] = v;
    }
}

int free_cps_mem() {
    // free all checkpoints in memory
    while (cps != NULL) {
        cps_t* ncps = cps->next;
        if (cps->xorparity != NULL) {
            free(cps->xorparity);
        }
        free(cps);
        cps = ncps;
    }

    return SUCCESS;
}

int free_cps_files() {

    // free all checkpoints
    struct dirent *de;
    DIR *dr = opendir(dirpath);
    char filepath[filepathsize];
    int rc, ret = SUCCESS;

    while ((de = readdir(dr)) != NULL) {
        if (is_myfile(de->d_name)) {
            sprintf(filepath, "%s/%s", dirpath, de->d_name);
            rc = remove(filepath);
            if (rc < 0) {
                perror("remove checkpoints failed");
                ret = FAILED;
            }
        }
    }
    closedir(dr);
    return ret;
}

void num_to_bytes(unsigned char* dest, unsigned long num, size_t size) {
    int i;
    for (i = 0; i<size; i++) {
        dest[i] = (num>>8*i) & 0xFF;
    }
}

unsigned long bytes_to_num(unsigned char* bytes, size_t size) {
    unsigned long num;
    memcpy(&num, bytes, size);
    return num;
}


void opt_memcpy(void *dest, void const *src, size_t size) {
    register unsigned long len = size;
    register unsigned char *rcdest = (unsigned char *) dest;
    register unsigned char *rcsrc = (unsigned char *) src;
    while (((unsigned long)rcdest & 7l) && len) {
        *rcdest++ = *rcsrc++;
        len--;
    }
    register unsigned long *rdest = (unsigned long *) rcdest;
    register unsigned long *rsrc = (unsigned long *) rcsrc;
    while (len >= 8) {
        *rdest++ = *rsrc++;
        len -= 8;
    }
    rcdest = (unsigned char*) rdest;
    rcsrc = (unsigned char*) rsrc;
    while (len) {
        *rcdest++ = *rcsrc++;
        len --;
    }
}

void opt_memset_zero(void *dest, size_t size) {
    register unsigned long len = size;
    register unsigned char *rcdest = (unsigned char *) dest;
    while (((unsigned long)rcdest & 7l) && len) {
        *rcdest ++ = '\0';
        len--;
    }
    register unsigned long *rdest = (unsigned long *) rcdest;
    while (len >= 8) {
        *rdest++ = 0;
        len -= 8;
    }
    rcdest = (unsigned char*) rdest;
    while (len) {
        *rcdest++ = '\0';
        len --;
    }
}

void memcpy_from_vars (data_t **data, size_t *offset, unsigned char *destbuffer, size_t copysize) {
    unsigned char *cdata = (unsigned char *) (*data)->rankdata+*offset;
    size_t var_size = (*data)->rankdatarealbcount-*offset;
    if (var_size < copysize) {
        size_t cum_size = 0;
        do {
            opt_memcpy(destbuffer+cum_size, cdata, var_size);
            cum_size += var_size;
            *data = (*data)->next;
            cdata = (unsigned char *) (*data)->rankdata;
            var_size = (*data)->rankdatarealbcount;
        }
        while (var_size < copysize - cum_size);
        opt_memcpy(destbuffer + cum_size, cdata, copysize - cum_size);
        *offset = copysize - cum_size;
    }
    else {
        opt_memcpy(destbuffer, cdata, copysize);
        *offset += copysize;
    }
}

void memcpy_to_vars (unsigned char *srcbuffer, data_t **data, size_t *offset, size_t copysize) {
    unsigned char *cdata = (unsigned char *) (*data)->rankdata+*offset;
    size_t var_size = (*data)->rankdatarealbcount-*offset;
    if (var_size < copysize) {
        size_t cum_size = 0;
        do {
            opt_memcpy(cdata, srcbuffer + cum_size, var_size);
            cum_size += var_size;
            *data = (*data)->next;
            cdata = (unsigned char *) (*data)->rankdata;
            var_size = (*data)->rankdatarealbcount;
        }
        while (var_size < copysize - cum_size);
        opt_memcpy(cdata, srcbuffer + cum_size, copysize - cum_size);
        *offset = copysize - cum_size;
    }
    else {
        opt_memcpy (cdata, srcbuffer, copysize);
        *offset += copysize;
    }

}

