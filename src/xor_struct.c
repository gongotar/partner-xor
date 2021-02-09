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

#include <xor_struct.h>


void compute_xstruct(xorstruct_t **xstruct, size_t datasize) {

    size_t maxchunkb = MAX_CHUNK_ELEMENTS*basesize;
    size_t segmentsize;

    // chunk segment size
    if (maxchunkb >= datasize) { // small data
        segmentsize = (size_t) datasize/(xranks-1) + (datasize% (xranks-1) > 0);
        segmentsize += basesize - segmentsize % basesize;
    }
    else {                      // large data
        segmentsize = (size_t) maxchunkb/(xranks-1);
        segmentsize = segmentsize - segmentsize % basesize;
    }

    // chunk structure
    (*xstruct)->chunkxparitysize = segmentsize;
    (*xstruct)->chunkxoffset = xrank*segmentsize;
    (*xstruct)->chunkdatasize = (xranks-1)*segmentsize;
    (*xstruct)->chunksize =  (*xstruct)->chunkdatasize + (*xstruct)->chunkxparitysize;

    // xor parity size
    if (maxchunkb >= datasize) { // small data
        (*xstruct)->xorparitysize = (*xstruct)->chunkxparitysize;
        (*xstruct)->marginsize = (*xstruct)->chunkdatasize - datasize;
    }
    else {                      // large data
        long nchunk = datasize/(*xstruct)->chunkdatasize;
        size_t remaining_data = datasize % (*xstruct)->chunkdatasize;
        // if at the end there will be an smaller chunk for the remaining data
        if (remaining_data > 0) {
            (*xstruct)->remaining_xorstruct = (xorstruct_t *) malloc(sizeof(xorstruct_t));
            compute_xstruct(&(*xstruct)->remaining_xorstruct, remaining_data);
            (*xstruct)->xorparitysize = nchunk*segmentsize+(*xstruct)->remaining_xorstruct->xorparitysize;
        }
        else {
            (*xstruct)->remaining_xorstruct = NULL;
            (*xstruct)->xorparitysize = nchunk*segmentsize;
        }
        (*xstruct)->marginsize = 0;
    }


}

size_t fill_xor_chunk(void *xchunk, data_t **data, size_t *offset, 
        void *parity, xorstruct_t *xstruct) {

    unsigned char *cchunk = (unsigned char *) xchunk;

    unsigned char *xdata;
    if (parity != NULL) {
        xdata = (unsigned char *) parity;
    }

    size_t realsize = xstruct->chunkdatasize - xstruct->marginsize;
    size_t fitted_size = xstruct->chunkdatasize;
    size_t paritysize = xstruct->chunkxparitysize;
    size_t chunkparityoffset = xstruct->chunkxoffset;

    if (data == NULL) {
        opt_memset_zero(cchunk, fitted_size + paritysize);
        return paritysize/basesize;
    }

    if (realsize <= chunkparityoffset) {

        memcpy_from_vars (data, offset, cchunk, realsize);
        if (parity == NULL) {
            opt_memset_zero(cchunk+realsize, fitted_size+paritysize-realsize);
        }
        else {
            opt_memset_zero(cchunk+realsize, fitted_size-realsize);
            opt_memcpy(cchunk+fitted_size, xdata, paritysize);
        }
    }
    else{
        memcpy_from_vars (data, offset, cchunk, chunkparityoffset);
        if (parity == NULL) {
            opt_memset_zero(cchunk+chunkparityoffset, paritysize);
        }
        else {
            opt_memcpy(cchunk+chunkparityoffset, xdata, paritysize);
        }

        memcpy_from_vars (data, offset, cchunk+chunkparityoffset+paritysize, realsize-chunkparityoffset);
        opt_memset_zero(cchunk+realsize+paritysize, fitted_size-realsize);
    }
    return paritysize/basesize;

}


void extract_xor_chunk(data_t **data, size_t *offset, 
        void *parity, void *xchunk, xorstruct_t *xstruct) {

    unsigned char *cparity = (unsigned char *) parity;
    unsigned char *cchunk = (unsigned char *) xchunk;


    size_t chunkparityoffset = xstruct->chunkxoffset;
    size_t datasize = xstruct->chunkdatasize - xstruct->marginsize;
    size_t paritysize = xstruct->chunkxparitysize;
    if (datasize <= chunkparityoffset) {
        memcpy_to_vars (cchunk, data, &offset, datasize);
        opt_memcpy(cparity, cchunk+chunkparityoffset, paritysize);
    }
    else {
        memcpy_to_vars (cchunk, data, &offset, chunkparityoffset);
        opt_memcpy(cparity, cchunk+chunkparityoffset, paritysize);
        memcpy_to_vars (cchunk+chunkparityoffset+paritysize, data, 
                &offset, datasize - chunkparityoffset);
    }

}
