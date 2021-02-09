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

#include <combined.h>
#include <checkpoint_utils.h>
#include <recovery_utils.h>
#include <common.h>



int init(MPI_Comm comm, char* path, int cphistory, int n, int m) {

    FAIL_IF_UNEXPECTED((n >= 2) && (m >= 3), 1, "group size violation: the minimum XOR size is 3, the minimum partner size is 2");

    basesize = sizeof(unsigned long);

    version = 0;
    fd = -1;
    cpcount = cphistory;
    cps = NULL;
    protected_data = NULL;
    xorstruct = NULL;
    totaldatasize = 0;

    // initialize checkpoint directory and file names
    dirpath = path;
    dirpathsize = strlen(path);
    if (dirpath[dirpathsize - 1] == '/') {
        dirpath[dirpathsize - 1] = '\0';
        dirpathsize -= 1;
    }
    filepathsize = dirpathsize + MAX_FILE_NAME_SIZE;
    mkdir(dirpath, S_IRWXU);

    // metadata file size
    metadatasize = sizeof(((cps_t *)0)->version);
    metadatasize += sizeof(((cps_t *)0)->filesize);
    metadatasize += sizeof(((cps_t *)0)->state);
    metadatasize += sizeof(((cps_t *)0)->failoffset);

    // fetch MPI ranks
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &ranks);
    MPI_Comm_set_errhandler(comm, MPI_ERRORS_RETURN);

    // get the number of ranks per node
    int namesize = 256;
    char hostname[namesize], prevhostname[namesize];
    gethostname(hostname, namesize);
    int prevr = rank-1, nextr = (rank+1)%ranks;
    if (prevr < 0) prevr = ranks-1;
    MPI_Sendrecv(hostname, namesize, MPI_CHAR, nextr, rank,
                prevhostname, namesize, MPI_CHAR, prevr, prevr,
                comm, MPI_STATUS_IGNORE);
    long changedhost = strcmp(hostname, prevhostname) != 0;
    long nhosts;
    MPI_Allreduce(&changedhost, &nhosts, 1, MPI_LONG, MPI_SUM, comm);
    FAIL_IF_UNEXPECTED((nhosts >= 6), 1, "the combined checkpoints need at least 6 nodes");

    int nodesize = ranks / nhosts;
    int nrank = rank % nodesize;

    // create the combined community ccomm containing xcomm and pcomm of an enclosed combined group of ranks
    //int communityid = rank/(m*n);
    //int communitykey = rank % (m*n);
    int communityid = nodesize*(int)(rank/(nodesize*m*n)) + nrank;
    int communitykey = (int)(rank/nodesize) % (m*n);

    int suc = 1;
    suc &= MPI_SUCCESS == MPI_Comm_split(comm, communityid, communitykey, &ccomm);
    suc &= MPI_SUCCESS == MPI_Comm_set_errhandler(ccomm, MPI_ERRORS_RETURN);
    suc &= MPI_SUCCESS == MPI_Comm_rank(ccomm, &crank);
    suc &= MPI_SUCCESS == MPI_Comm_size(ccomm, &cranks);
    // create a communicator for the communication between rank 0 of communities
    int ccolor = (crank == 0) ? 1 : MPI_UNDEFINED;
    suc &= MPI_SUCCESS == MPI_Comm_split(comm, ccolor, communityid, &interccomm);

    // create pcomm and xcomm communicators
    // this works in accordance to theoretical_repair method
    int pid = crank / n;
    int xid = crank % n;
    suc &= MPI_SUCCESS == MPI_Comm_split(ccomm, xid, pid, &xcomm);
    suc &= MPI_SUCCESS == MPI_Comm_split(ccomm, pid, xid, &pcomm);
    suc &= MPI_SUCCESS == MPI_Comm_rank(xcomm, &xrank);
    suc &= MPI_SUCCESS == MPI_Comm_size(xcomm, &xranks);
    suc &= MPI_SUCCESS == MPI_Comm_rank(pcomm, &prank);
    suc &= MPI_SUCCESS == MPI_Comm_size(pcomm, &pranks);

    // create xor operation in MPI
    suc &= MPI_SUCCESS == MPI_Op_create((MPI_User_function*)compute_xor_op, 1, &xor_op);
    suc &= MPI_SUCCESS == MPI_Op_create((MPI_User_function*)intersect_list_op, 1, &intersect_op);

    FAIL_IF_UNEXPECTED(suc, 1, "initialize failed");

    return SUCCESS;
}

int protect(void* data, size_t size) {
    // could make comm allreduce max of size, to allow variable sizes per rank / or step:
    // in this case every checkpoint call should pad zero before/after rank data


    data_t *current_data = (data_t *) malloc (sizeof (data_t));
    current_data->next = protected_data;
    protected_data = current_data;

    current_data->rankdata = (unsigned char*) data;
    current_data->rankdatarealbcount = size;
    totaldatasize += size;

    return SUCCESS;
}

int recover(int *restart) {

    // version decision
    load_local_metadata();
    int rc = global_negotiate_version(&version, &lostpgroup);
    FAIL_IF_UNEXPECTED(rc, MPI_SUCCESS, "MPI: negotiate version failed");

    *restart = version;
    if (version == 0) {
        free_cps_mem();
        return SUCCESS;
    }

    // fetch and init the corresponding checkpoint
    cps_t* cp = NULL;
    cps_t* tmpcp = cps;
    while (tmpcp != NULL) {
        if (tmpcp->version == version) {
            cp = tmpcp;
            break;
        }
        tmpcp = tmpcp->next;
    }
    if (cp == NULL) {
        cp = (cps_t*) malloc(sizeof(cps_t));
        cp->filesize = 0;
        cp->state = NODATA;
        cp->version = version;
        cp->next = cps;
        cps = cp;
    }

    cp->data = protected_data;
    xorstruct = (xorstruct_t *) malloc(sizeof(xorstruct_t));
    compute_xstruct(&xorstruct, totaldatasize); 
    cp->xorstruct = xorstruct;
    cp->loaded = 0;
    cp->xorparity = NULL;
    if (lostpgroup) {
        cp->xorparity = (unsigned char *) malloc(cp->xorstruct->xorparitysize);
    }

    // ------ start recovery ------
    // phase partner recovery
    int pstates[pranks];
    int bfull = (cp->state == FULLDATA);
    rc = MPI_Allgather(&bfull, 1, MPI_INT, pstates, 1, MPI_INT, pcomm);
    FAIL_IF_UNEXPECTED(rc, MPI_SUCCESS, "MPI: partner recovery pstates allgather failed");
    while (imbalance_partners(pstates)) {
        int partner = assign_partner(pstates);
        if (partner != prank) {
            rc = partner_recover(&cp, partner);
            FAIL_IF_UNEXPECTED(rc, SUCCESS, "partner recovery failed");
        }
        bfull = (cp->state == FULLDATA);
        rc = MPI_Allgather(&bfull, 1, MPI_INT, pstates, 1, MPI_INT, pcomm);
        FAIL_IF_UNEXPECTED(rc, MPI_SUCCESS, "MPI: partner recovery pstates allgather failed");
    }

    // load checkpoint if not already loaded in partner phase
    if (!cp->loaded) {
        if (cp->filesize > 0) {
            size_t offset = 0;
            rc = load_local_checkpoint(&cp, &offset); // load the local data
            if (lostpgroup) {
                printf("loading XOR\n");
                rc = load_local_checkpoint(&cp, &offset); // load the xor parity
            }
            FAIL_IF_UNEXPECTED(rc, SUCCESS, "cannot load local checkpoint");
        }
        cp->loaded = 1;
    }

    // phase xor recovery
    int nodatas;
    int bnodata = (cp->state == NODATA)*(xranks + xrank); // for other ranks to detect the failed rank
    rc = MPI_Allreduce(&bnodata, &nodatas, 1, MPI_INT, MPI_SUM, xcomm);
    FAIL_IF_UNEXPECTED(rc, MPI_SUCCESS, "MPI: xor recovery xstate check failed");
    if (nodatas >= xranks && nodatas < 2*xranks) {
        int failedrank = nodatas - xranks;
        rc = xor_recover(&cp, failedrank);
        FAIL_IF_UNEXPECTED(rc, SUCCESS, "xor recovery failed");
    }

    // phase exchange checkpoints
    int allxor;
    int bxordata = (cp->state == XORDATA);
    rc = MPI_Allreduce(&bxordata, &allxor, 1, MPI_INT, MPI_LAND, pcomm);
    FAIL_IF_UNEXPECTED(rc, MPI_SUCCESS, "MPI: exchange recovery pstate check failed");
    if (allxor) {
        rc = exchange_local_cps(&cp);
        FAIL_IF_UNEXPECTED(rc, SUCCESS, "exchange recovery failed");
    }
    MPI_Barrier(ccomm);
    free_cps_mem();

    return SUCCESS;
}

int checkpoint() {

    version += 1;
    int rc;

    // xorstruct is hopefully created already by recover.
    // Though, if recover is not called for any reason, the first 
    // checkpoint call creates the xorstruct.
    if (xorstruct == NULL) {
        xorstruct = (xorstruct_t *) malloc(sizeof(xorstruct_t));
        compute_xstruct(&xorstruct, totaldatasize); 
    }

    cps_t *cp = (cps_t*) malloc(sizeof(cps_t));
    cp->version = version;
    cp->state = NODATA;
    cp->xorstruct = xorstruct;
    cp->data = protected_data;
    cp->next = cps;
    cps = cp;

    // create checkpoint
    rc = xor_checkpoint(&cp);

    if (rc == SUCCESS) {
        cp->state = XORDATA;


        // remvove the metadata of the old checkpoint
        char metapath[filepathsize];
        generate_metafilepath(metapath, cp->version);
        remove(metapath);

        // checkpoint file
        char cppath[filepathsize];
        generate_cpfilepath(cppath, cp->version);
        fd = open(cppath, FILE_OPEN_WRITE_FLAGS, S_IRWXU);

        // partner checkpoint
        rc = partner_checkpoint(&cp);
        if (rc == SUCCESS) {
            cp->state = FULLDATA;
        }

        rc = write_metadata_file(cp);
    }
    free_cps_mem();
    return rc;
}

int finalize() {

    free_cps_mem();
    if (xorstruct != NULL) {
        if (xorstruct->remaining_xorstruct != NULL) {
            free(xorstruct->remaining_xorstruct);
        }
        free(xorstruct);
        xorstruct = NULL;
    }

    close(fd);

    int suc = 1;
    suc &= SUCCESS == free_cps_files();

    // free created communicators
    suc &= MPI_SUCCESS == MPI_Comm_free(&pcomm);
    suc &= MPI_SUCCESS == MPI_Comm_free(&xcomm);
    if (crank == 0) {
        suc &= MPI_SUCCESS == MPI_Comm_free(&interccomm);
    }
    suc &= MPI_SUCCESS == MPI_Comm_free(&ccomm);
    suc &= MPI_SUCCESS == MPI_Op_free(&xor_op);
    suc &= MPI_SUCCESS == MPI_Op_free(&intersect_op);

    if (!suc) {
        perror("finalize failed");
        return FAILED;
    }

    return SUCCESS;
}
