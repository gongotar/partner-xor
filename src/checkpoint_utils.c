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

#include <checkpoint_utils.h>


int xor_checkpoint(cps_t **cp) {

    size_t chunkdatab, chunkparityb, chunkb, chunkparityel;
    size_t computedb = 0, xcomputedb = 0;
    int recvcounts[xranks], i, lastchunk, rc;
    unsigned char *localdata = (*cp)->localdata;
    size_t ldatasize = (*cp)->ldatasize;

    xorstruct_t *xstruct = (*cp)->xorstruct;
    chunkb = xstruct->chunksize;
    chunkdatab = xstruct->chunkdatasize;
    chunkparityb = xstruct->chunkxparitysize;
    chunkparityel = chunkparityb/basesize;
    size_t xorparitysize = xstruct->xorparitysize;
    (*cp)->xorparity = (unsigned char *) malloc(xorparitysize);
    unsigned char *xorparity = (*cp)->xorparity;
    chunk = (unsigned char *) malloc(chunkb);

    for (i = 0; i<xranks; i++) {
        recvcounts[i] = chunkparityel;
    }

    while (computedb < ldatasize) {
        lastchunk = (ldatasize - computedb) < chunkdatab;

        if (lastchunk) {
            chunkdatab = ldatasize - computedb;
            chunkparityel = fill_xor_chunk(chunk, localdata + computedb, NULL, xstruct->remaining_xorstruct);
            for (i = 0; i<xranks; i++) {
                recvcounts[i] = chunkparityel;
            }
            chunkparityb = chunkparityel*basesize;
        }
        else {
            chunkparityel = fill_xor_chunk(chunk, localdata + computedb, NULL, xstruct);
        }

        // compute xor
        rc = MPI_Reduce_scatter(chunk, xorparity+xcomputedb, recvcounts, MPI_LONG, xor_op, xcomm);
        FAIL_IF_UNEXPECTED(rc, MPI_SUCCESS, "MPI: checkpoint failed: xor operation");
        computedb += chunkdatab;
        xcomputedb += chunkparityb;
    }
    free(chunk);
    chunk = NULL;

    (*cp)->xparitysize = xcomputedb;
    assert(xcomputedb == xorparitysize);

    return SUCCESS;
}

int partner_checkpoint(cps_t **cp) {

    // all nodes in pcomm get and write the cp on to the disk


    unsigned char *localdata = (*cp)->localdata;
    unsigned char *xorparity = (*cp)->xorparity;

    ssize_t failoffset = -1;
    size_t ldatasize = (*cp)->ldatasize;
    size_t xparitysize = (*cp)->xparitysize;
    size_t filesize = 0;
    int rc;

    if (pranks < 2) { // only xor
        rc = write_data(fd, localdata, ldatasize);
        FAIL_IF_UNEXPECTED(rc, SUCCESS, "checkpoint write failed");
        filesize += ldatasize;
        rc = write_data(fd, xorparity, xparitysize);
        FAIL_IF_UNEXPECTED(rc, SUCCESS, "checkpoint write failed");
        filesize += xparitysize;
    }
    else {
        rc = all_transfer_and_write(localdata, ldatasize, fd, &filesize, &failoffset);
        if (rc == SUCCESS) {
            rc = all_transfer_and_write(xorparity, xparitysize, fd, &filesize, &failoffset);
        }
        else {
            rc = write_data(fd, xorparity, xparitysize);
            FAIL_IF_UNEXPECTED(rc, SUCCESS, "checkpoint write failed");
            filesize += xparitysize;
        }
    }

    fsync(fd);
    close(fd);

    (*cp)->filesize = filesize;
    (*cp)->failoffset = failoffset;
    return rc;
}

int all_transfer_and_write(unsigned char *buffer, size_t size, int fd, size_t *filesize, ssize_t *failoffset) {

    ssize_t ws;
    int lastchunk, rc;
    size_t chunkdatab, sentb = 0;
    if (MAX_CHUNK_ELEMENTS*basesize > size) {
        chunkdatab = fit_datasize(size, 1, basesize, 1);
    }
    else {
        chunkdatab = MAX_CHUNK_ELEMENTS*basesize;
    }

    size_t chunkdatael = chunkdatab/basesize;
    chunk = (unsigned char *) malloc(chunkdatab*pranks);
    while (sentb < size) {

        lastchunk = (size - sentb) < chunkdatab;
        if (lastchunk) {
            size_t remainingb = size - sentb;
            chunkdatab = fit_datasize(remainingb, 1, basesize, 1);
            chunkdatael = chunkdatab/basesize;
        }

        rc = MPI_Allgather(buffer+sentb, chunkdatael, MPI_LONG, chunk, chunkdatael, MPI_LONG, pcomm);

        if (rc == SUCCESS) {
            ws = write(fd, chunk, chunkdatab*pranks);
            FAIL_IF_UNEXPECTED(ws, chunkdatab*pranks, "checkpoint write failed");
            *filesize += chunkdatab*pranks;
            sentb += chunkdatab;
        }
        else {
            *failoffset = sentb;
            rc = write_data(fd, buffer+sentb, size-sentb);
            FAIL_IF_UNEXPECTED(rc, SUCCESS, "checkpoint write failed");
            *filesize += size - sentb;
            FAIL_IF_UNEXPECTED(1, 0, "MPI: partner checkpoint transfer failed");
        }
    }
    free(chunk);
    chunk = NULL;

    return SUCCESS;
}


int write_data(int fd, void *data, size_t size) {

    unsigned char *buffer = (unsigned char*) data;
    size_t written = 0, chsize = SSIZE_MAX, adjustedsize;
    ssize_t ws;
    while (written < size) {
        adjustedsize = (size - written > chsize) ? chsize : size - written;
        ws = write(fd, buffer+written, adjustedsize);
        FAIL_IF_UNEXPECTED(ws, adjustedsize, "checkpoint write failed");
        written += adjustedsize;
    }

    return SUCCESS;
}

int write_metadata_file(cps_t* cp) {

    char metapath[filepathsize];
    generate_metafilepath(metapath, cp->version);
    fd = open(metapath, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);

    unsigned char metadata[metadatasize];
    unsigned char* metaoffset = metadata;
    num_to_bytes(metaoffset, cp->version, sizeof(cp->version));
    metaoffset += sizeof(cp->version);
    num_to_bytes(metaoffset, cp->filesize, sizeof(cp->filesize));
    metaoffset += sizeof(cp->filesize);
    num_to_bytes(metaoffset, cp->state, sizeof(cp->state));
    metaoffset += sizeof(cp->state);
    num_to_bytes(metaoffset, cp->failoffset, sizeof(cp->failoffset));

    ssize_t ws = write(fd, metadata, metadatasize);
    FAIL_IF_UNEXPECTED(ws, metadatasize, "metadata write failed");

    close(fd);
    return SUCCESS;
}

size_t fit_datasize(size_t datasize, int segments, size_t basesize, int increase_size) {

    if (datasize == 0) {
        return 0;
    }

    long num1 = segments, num2 = (long) basesize, gcd, lcm, rem, numerator, denominator;

    if(num1 > num2) {
        numerator = num1;
        denominator = num2;
    }
    else {
        numerator = num2;
        denominator = num1;
    }

    rem = numerator % denominator;

    while(rem != 0) {
        numerator = denominator;
        denominator = rem;
        rem = numerator % denominator;
    }

    gcd = denominator;
    lcm = (num1 * num2) / gcd;

    size_t fitting_size;
    if (increase_size) {
        fitting_size = datasize + lcm - datasize%lcm;
    }
    else {
        fitting_size = datasize - datasize%lcm;
    }
    return fitting_size;
}
