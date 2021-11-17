#ifndef SYSTEM_H
#define SYSTEM_H

#define A_ADDR 12
#define B_ADDR 16384
#define C_ADDR 32768
#define SIZE 4
#define LOOPS 10
#define MEMORY_SIZE (1 << (10+6))
#define CLK_CYCLE 6.66 // in nanoseconds

#define INIT_BUS_STATE UINT_MAX

#define OP_READ 0
#define OP_WRITE 1
#define OP_CALC 2

#define SW_ID 0
#define HW_ID 1


#endif
