/**
 *  Copyright (c) 2017 Leonardo A. Bautista-Gomez
 *  All rights reserved
 *
 *  @file   heatdis.h
 *  @author Leonardo A. Bautista Gomez
 *  @date   January, 2014
 *  @brief  Heat distribution code to test FTI.
 */

#ifndef _HEATDIS_H
#define _HEATDIS_H

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <mpi.h>

#define PRECISION   0.00001
#define ITER_TIMES  600
#define ITER_OUT    30
#define WORKTAG     50
#define REDUCE      5
#define CP_INTERVAL 30


#ifdef __cplusplus
extern "C" {
#endif

void initData(int nbLines, int M, int rank, double *h);
double doWork(int numprocs, int rank, int M, int nbLines, double *g, double *h);

#ifdef __cplusplus
}
#endif

#endif /* ----- #ifndef _HEATDIS_H  ----- */
