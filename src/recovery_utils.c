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

#include <recovery_utils.h>


int global_negotiate_version(int *version, int *lostpgroup) {

    *version = 0;
    *lostpgroup = 0;

    // fill the lists of local versions
    int local_fullversions[cpcount];  // local FULLDATA versions
    int local_versions[cpcount];  // local XORDATA versions + FULLDATA versions
    opt_memset_zero(local_versions, cpcount*sizeof(int));
    opt_memset_zero(local_fullversions, cpcount*sizeof(int));
    cps_t *cp = cps;
    int i = 0, ifull = 0;
    while (cp != NULL) {
        if (cp->state == FULLDATA) {
            local_fullversions[ifull] = cp->version;
            ifull++;
        }
        local_versions[i] = cp->version;
        i++;
        cp = cp->next;
    }

    // explain why XORDATA in checkpoint call should be kept after a failure
    // during partner checkpoint? if for example there are two xor groups, one node
    // from each xor group (not partners) fail during
    // checkpoint call after xor, before partner completion
    // the xor data can recover the work

    // root of the community gathers all FULLDATA and XORDATA versions in the ccomm
    int ccomm_fullversions[cpcount*cranks], ccomm_versions[cpcount*cranks];
    int rc = MPI_Gather(local_fullversions, cpcount, MPI_INT, ccomm_fullversions, cpcount, MPI_INT, 0, ccomm);
    FAIL_IF_UNEXPECTED(rc, MPI_SUCCESS, "negotiate version gather full versions failed");
    rc = MPI_Gather(local_versions, cpcount, MPI_INT, ccomm_versions, cpcount, MPI_INT, 0, ccomm);
    FAIL_IF_UNEXPECTED(rc, MPI_SUCCESS, "negotiate version gather xor versions failed");

    if (crank == 0) {
        int ccomm_recoverables[cpcount], global_recoverables[cpcount];
        opt_memset_zero(ccomm_recoverables, sizeof(ccomm_recoverables));
        // Do computation, simulate, if failed, first check partner then xor (xn-1)
        // also implement intersect_op

        // preper a hash of all possible unique versions received from other ranks
        int hashset[cpcount+1], rvcount = cranks*cpcount;
        make_hashset(hashset, cpcount+1, ccomm_versions, rvcount);

        // for each unique version, construct the community states and check if it is recoverable
        int ccommstates[cranks], *rankvs, *rankfullvs;
        int r, v, repairphase, repairedcount = 0;
        for (i=0; i<cpcount+1; i++) {
            v = hashset[i];
            if (v == 0) continue;
            for (r=0; r<cranks; r++) {
                rankvs = ccomm_versions+(r*cpcount);
                rankfullvs = ccomm_fullversions+(r*cpcount);
                ccommstates[r] =  version_state_in_presented_versions(v, rankvs, rankfullvs);
            }
            repairphase = theoretical_repair(ccommstates);
            if (repairphase > 0) {
                ccomm_recoverables[repairedcount++] = v;
            }
            if (repairphase > 1) { // will be repaired in phase 2 or 3 and not the first phase (partner)
                *lostpgroup = 1;
            }
        }

        rc = MPI_Allreduce(ccomm_recoverables, global_recoverables, cpcount, MPI_INT, intersect_op, interccomm);
        FAIL_IF_UNEXPECTED(rc, MPI_SUCCESS, "negotiate version intersect between communities failed");

        for (i=0; i<cpcount; i++) {
            if (global_recoverables[i] > *version) {
                *version = global_recoverables[i];
            }
        }
    }

    // inform everyone in the community about the version
    rc = MPI_Bcast(version, 1, MPI_INT, 0, ccomm);
    FAIL_IF_UNEXPECTED(rc, MPI_SUCCESS, "negotiate version broadcast version to community failed");
    // inform everyone in the community if there is a failed partner group
    rc = MPI_Bcast(lostpgroup, 1, MPI_INT, 0, ccomm);
    FAIL_IF_UNEXPECTED(rc, MPI_SUCCESS, "negotiate version broadcast lostpgroup to community failed");

    return SUCCESS;
}

int assign_partner(int pstates[]) {
    // start from my index (prank) and go forward (roundroubin)
    // if encounter not_full increment barrier
    // if encounter full decriment barrier
    // if barrier zero: return index (my own index if no partner assigned)

    int pointer = prank;
    int waitingrank, waitingranks = 0;
    int direction = pstates[prank]*2 - 1; // if receiver backward search, else forward
    do {
        waitingrank = 1 - pstates[pointer]*2;    // 1 if not full, -1 if full
        waitingranks += waitingrank;
        if (waitingranks == 0) {
            break;
        }
        pointer = pointer + direction;    // go forward if full, backward if not full
        pointer = (pointer < 0) ? pranks-1: pointer%pranks;
    } while (pointer != prank);
    return pointer;
}


int partner_recover(cps_t **cp, int partner) {
    // send/recv prank to/from partner
    // receiver: cp->state != FULLDATA
    // sender: cp->state == FULLDATA
    // update cp->state of the receiver on success
    // after return each partner has its local checkpoint on memory and partner
    // checkpoints on disk


    // open file, remove old files
    int flags, rc, state = (*cp)->state;
    if (state == FULLDATA) {
        flags = FILE_OPEN_READ_FLAGS;
    }
    else {
        flags = FILE_OPEN_WRITE_FLAGS;
        // remvove the metadata of the old checkpoint
        char metapath[filepathsize];
        generate_metafilepath(metapath, (*cp)->version);
        remove(metapath);
    }
    char cppath[filepathsize];
    generate_cpfilepath(cppath, version);
    fd = open(cppath, flags, S_IRWXU);

    // load ranks data
    rc = pair_transfer_and_write(cp, fd, partner, ((*cp)->loaded == 0), 0);
    if (rc == SUCCESS) { // load xor data
        rc = pair_transfer_and_write(cp, fd, partner, ((*cp)->loaded == 0) && lostpgroup, 1);
        if (rc == SUCCESS) {
            (*cp)->loaded = 1;
            if ((*cp)->state != FULLDATA) {
                (*cp)->state = FULLDATA;
                write_metadata_file(*cp);
            }
        }
    }

    fsync(fd);
    close(fd);

    return rc;
}


int xor_recover(cps_t **cp, int lostrank) {

    int rc, lastchunk;
    size_t computedb = 0, xcomputedb = 0;
    unsigned char *xorparity = (*cp)->xorparity;
    data_t *data = (*cp)->data;
    size_t offset = 0, rec_offset = 0;
    size_t ldatasize = totaldatasize;
    size_t chunkb, chunkel, chunkparityel, chunkparityb, chunkdatab;

    xorstruct_t *xstruct = (*cp)->xorstruct, *xstrct;

    chunkb = xstruct->chunksize;
    chunkdatab = xstruct->chunkdatasize;
    chunkparityb = xstruct->chunkxparitysize;
    chunkparityel = chunkparityb/basesize;
    chunkel = chunkb/basesize;

    chunk = (unsigned char *) malloc(chunkb);

    unsigned char *recvchunk = NULL;
    if (xrank == lostrank) {
        chunkparityel = fill_xor_chunk(chunk, NULL, &offset, NULL, xstruct);
        recvchunk = (unsigned char *) malloc(chunkb);
    }

    while(computedb < ldatasize) {
        lastchunk = (ldatasize - computedb) < chunkdatab;
        chunkdatab = (lastchunk) ? ldatasize - computedb : chunkdatab;

        xstrct = (lastchunk) ? xstruct->remaining_xorstruct : xstruct;

        if (xrank != lostrank) {
            chunkparityel = fill_xor_chunk(chunk, &data, &offset, 
                    xorparity + xcomputedb, xstrct);
        }
        else if (lastchunk) {
            chunkparityel = fill_xor_chunk(chunk, NULL, &offset, NULL, 
                    xstrct);
        }

        rc = MPI_Reduce(chunk, recvchunk, chunkel, MPI_LONG, xor_op, lostrank, xcomm);
        if (rc != MPI_SUCCESS) {
            free(recvchunk);
            FAIL_IF_UNEXPECTED(rc, MPI_SUCCESS, "MPI: xor recovery mpi reduce failed");
        }
        if (xrank == lostrank) {
            extract_xor_chunk(&data, &rec_offset, xorparity + xcomputedb, recvchunk, xstrct);
        }

        xcomputedb += chunkparityel*basesize;
        computedb += chunkdatab;
    }

    assert(xcomputedb == xstruct->xorparitysize);

    if (xrank == lostrank) {
        printf ("xrec data %d len %ld\n",*((int *)((*cp)->data->rankdata)), (*cp)->data->rankdatarealbcount);
    }

    free(chunk);
    chunk = NULL;
    free(recvchunk);
    (*cp)->state = (xrank == lostrank) ? XORDATA : (*cp)->state;

    return SUCCESS;

}


int exchange_local_cps(cps_t** cp) {
    // allgather local cps
    // update cp->state of all nodes to FULLDATA if success
    // at the end all ranks have loaded+stored cps
    // out: data stored in the disk
    int rc = partner_checkpoint(cp);
    if (rc == SUCCESS) {
        rc = write_metadata_file(*cp);
    }
    FAIL_IF_UNEXPECTED(rc, SUCCESS, "exchange local checkpoints failed");

    return SUCCESS;
}

int theoretical_repair(int* ccommstates) {
    int xid, pid, pr, xr, index, pgfull, xgfails, recoveryphase = 1;

    // in every partner group, if someone has FULLDATA, mark everyone as FULLDATA
    for (pid = 0; pid<xranks; pid++) {
        pgfull = 0;
        for (pr = 0; pr<pranks; pr++) {
            index = pid*pranks + pr;
            // index = pr*xranks + pid;
            if (ccommstates[index] == FULLDATA) {
                pgfull = 1;
                break;
            }
        }
        if (pgfull) {
            for (pr = 0; pr<pranks; pr++) {
                index = pid*pranks + pr;
                // index = pr*xranks + pid;
                ccommstates[index] = FULLDATA;
            }
        }
    }

    // in every XOR group, if there are more than one NODATAs (after partner recovory),
    // then, there is no way to recover the NODATAs, return 0: not repairable
    for (xid = 0; xid<pranks; xid++) {
        xgfails = 0;
        for (xr = 0; xr<xranks; xr++) {
            index = xr*pranks+xid;
            // index = xid*xranks + xr;
            if (ccommstates[index] == NODATA) {
                recoveryphase = 3; // this case requires 3 phases of recovery
                xgfails ++;
                if (xgfails > 1) {
                    return 0;
                }
            }
        }
    }

    return recoveryphase;

}


int version_state_in_presented_versions(int version, int *rankversions, int *rankfullversions) {
    int i, xdata = 0;
    for (i=0; i<cpcount; i++) {
        if (rankfullversions[i] == version) {
            return FULLDATA;
        }
        if (rankversions[i] == version) {
            xdata = 1;
        }
    }
    return (xdata) ? XORDATA : NODATA;
}

int pair_transfer_and_write (cps_t **cp, int fd, int partner, int memload, int xor_transfer) {

    size_t size;
    size_t *filesize = &((*cp)->filesize);
    unsigned char *xorparity;
    if (!xor_transfer) {
        size = totaldatasize;
    }
    else {
        size = (*cp)->xorstruct->xorparitysize;
        xorparity = (*cp)->xorparity;
    }

    int state = (*cp)->state;
    data_t *data = (*cp)->data;
    size_t offset = 0;

    // adjust chunk size and create the chunk
    size_t chunkdatab, sentb = 0;
    if (MAX_CHUNK_ELEMENTS*basesize > size) {
        chunkdatab = fit_datasize(size, 1, basesize, 1);
    }
    else {
        chunkdatab = MAX_CHUNK_ELEMENTS*basesize;
    }
    size_t chunkdatael = chunkdatab/basesize;
    chunk = (unsigned char *) malloc(chunkdatab*pranks);
    ssize_t rs, ws, actualdatab = chunkdatab;
    int rc, lastchunk;

    while (sentb < size) {
        lastchunk = (size - sentb) < chunkdatab;

        if (lastchunk) {
            actualdatab = size - sentb;
            chunkdatab = fit_datasize(actualdatab, 1, basesize, 1);
            chunkdatael = chunkdatab/basesize;
        }

        if (state == FULLDATA) {
            rs = read(fd, chunk, chunkdatab*pranks);
            FAIL_IF_UNEXPECTED(rs, chunkdatab*pranks, "partner read checkpoint failed");
            rc = MPI_Send(chunk, chunkdatael*pranks, MPI_LONG, partner, 1, pcomm);
            FAIL_IF_UNEXPECTED(rc, MPI_SUCCESS, "MPI: partner send checkpoint failed");
        }
        else {
            rc = MPI_Recv(chunk, chunkdatael*pranks, MPI_LONG, partner, 1, pcomm, MPI_STATUS_IGNORE);
            FAIL_IF_UNEXPECTED(rc, MPI_SUCCESS, "MPI: partner recv checkpoint failed");
            ws = write(fd, chunk, chunkdatab*pranks);
            FAIL_IF_UNEXPECTED(ws, chunkdatab*pranks, "partner write checkpoint failed");
        }

        if (memload) {
            if (!xor_transfer) {
                memcpy_to_vars (chunk+prank*chunkdatab, &data, &offset, actualdatab);
            }
            else {
                opt_memcpy(xorparity+sentb, chunk+prank*chunkdatab, actualdatab);
            }
        }

        sentb += chunkdatab;
    }

    free(chunk);
    chunk = NULL;
    if (state != FULLDATA) {
        *filesize += sentb*pranks;
    }

    return SUCCESS;
}




int imbalance_partners(int pstates[]) {
    int bnfull = 0, bfull = 0;
    int i;
    for (i = 0; i<pranks; i++) {
        bnfull = bnfull || !pstates[i];
        bfull = bfull || pstates[i];
        if (bfull && bnfull) {
            return 1;
        }
    }
    return 0;
}

int load_local_metadata() {

    // check meta data of checkpoints, data will not be loaded
    struct dirent *de;
    DIR *dr = opendir(dirpath);
    char filepath[filepathsize];
    unsigned char  metadata[metadatasize];
    unsigned char* metaoffset;
    ssize_t rs;
    cps_t* cp;

    if (cps != NULL) {
        free_cps_mem();
    }

    while ((de = readdir(dr)) != NULL) {
        if (is_myfile(de->d_name) && is_metafile(de->d_name)) {
            // read meta file
            sprintf(filepath, "%s/%s", dirpath, de->d_name);
            fd = open(filepath,  FILE_OPEN_READ_FLAGS);
            rs = read(fd, metadata, metadatasize);
            if (rs != metadatasize) {
                closedir(dr);
                FAIL_IF_UNEXPECTED(rs, metadatasize, "load metadata failed");
            }

            // write metadata to cps mem
            cp = (cps_t*) malloc(sizeof(cps_t));
            metaoffset = metadata;
            cp->version = bytes_to_num(metaoffset, sizeof(cp->version));
            metaoffset += sizeof(cp->version);
            cp->filesize = bytes_to_num(metaoffset, sizeof(cp->filesize));
            metaoffset += sizeof(cp->filesize);
            cp->state = bytes_to_num(metaoffset, sizeof(cp->state));
            metaoffset += sizeof(cp->state);
            cp->failoffset = bytes_to_num(metaoffset, sizeof(cp->failoffset));
            cp->xorparity = NULL;
            cp->next = cps;
            cps = cp;
        }
    }
    closedir(dr);
    return SUCCESS;
}

int load_local_checkpoint(cps_t** cp, size_t *offset) {

    ssize_t rs;
    size_t chunkdatab, readb = 0, size, failoffset = (*cp)->failoffset;
    data_t *data = (*cp)->data;
    size_t data_offset = 0;
    unsigned char *buffer;
    if (*offset == 0) {  // loading the local data
        size = totaldatasize;
    }
    else {  // loading the xor parity
        size = (*cp)->xorstruct->xorparitysize;
        buffer = (*cp)->xorparity;
    }

    if (MAX_CHUNK_ELEMENTS*basesize > size) {
        chunkdatab = fit_datasize(size, 1, basesize, 1);
    }
    else {
        chunkdatab = MAX_CHUNK_ELEMENTS*basesize;
    }

    size_t actualdatab = chunkdatab, seekoffset;

    if (*offset == 0) {
        buffer = (unsigned char *) malloc (actualdatab);
    }

    char cppath[filepathsize];
    generate_cpfilepath(cppath, (*cp)->version);
    fd = open(cppath, FILE_OPEN_READ_FLAGS);

    int lastchunk;

    while (readb < size) {
        lastchunk = (size - readb) < chunkdatab;
        if (lastchunk) {
            actualdatab = size - readb;
            chunkdatab = fit_datasize(actualdatab, 1, basesize, 1);
        }
        if (failoffset >= 0 && readb*pranks+*offset > failoffset) {
            seekoffset = readb*pranks + *offset;
        }
        else {
            seekoffset = readb*pranks + chunkdatab*prank + *offset;
        }
        lseek(fd, seekoffset, SEEK_SET);
        if (*offset == 0) { // read to the protected data
            rs = read(fd, buffer, actualdatab);
            memcpy_to_vars (buffer, &data, &data_offset, actualdatab);
        }
        else { // red to xor buffer
            rs = read(fd, buffer+readb, actualdatab);
        }
        FAIL_IF_UNEXPECTED(rs, actualdatab, "load local checkpoint failed");

        readb += actualdatab;
    }

    // update the current offset
    *offset = seekoffset+actualdatab;

    close(fd);
    return SUCCESS;

}

