/**
 * ------
 * main.h
 * ------
 * 
 * Defines the defaults for the main program
 * if no arguments are passed on the command line.
 * 
 * Author: Jasper Yun (260651891)
 * 
 * ECSE 541: Design of MPSoC
 * Fall 2021
 * McGill University
 * 
 */

#pragma once

#include <math.h>

#define CLOCK_PERIOD sc_time(6.67, SC_NS) // ns, 150 MHz

// Op values
#define OP_NONE         0U              // do nothing operation
#define OP_READ         1U              // read op
#define OP_WRITE        2U              // write op
#define OP_CALC         3U              // calculation op for hardware

// Master IDs
#define BUS_IDLE_ID     0U              // shared bus idle state mst_id
#define MST_ID_SW       1U              // software master
#define MST_ID_HW       2U              // hardware master

// parameters
#define MATRIX_SIZE     6               // size of arrays (SIZE x SIZE)
#define LOOPS           5               // loops
#define MATRIX_NUM_EL   MATRIX_SIZE*MATRIX_SIZE

// Addresses of matrices in memory
#define ADDR_A          0               // Matrix A address
#define ADDR_L          10000           // Matrix L address
#define MEM_SIZE        5000000         // some base default size if none specified

// addresses of memory-mapped components (hardware and software!) 
// (taken from L05 slides from ECSE 444 F21)
#define ADDR_BUS_IDLE   0x99999999      // shared bus idle state memory address
#define ADDR_MEM_SW     0x00000000      // code starts near address 0x0
#define ADDR_MEM_HW     0x40000000      // hardware partition as on-chip peripheral
#define ADDR_MEM_MEM    0x20000000      // on-chip SRAM

#define LEN_BUS_IDLE    0U              // shared bus idle state request length

#define DATA_REUSE

// software calculation time penalties in clock cycles
#define DELAY_SW_ADD    1*CLOCK_PERIOD  // add/sub/assign takes 1 clock cycle
#define DELAY_SW_MUL    6*CLOCK_PERIOD  // mul/div takes 6 clock cycles
#define DELAY_SW_CMP    1*CLOCK_PERIOD  // cmp takes 1 clock cycle

// for debugging
// #define DEBUG
// #define DEBUG_SW
// #define DEBUG_HW
// #define DEBUG_MEM
// #define DEBUG_BUS

#include <stdio.h>
// #include <vector>

using namespace std;