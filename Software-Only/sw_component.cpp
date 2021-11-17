/**
 * ----------------
 * sw_component.cpp
 * ----------------
 * 
 * Implementation of the software
 * component.
 * 
 * Author: Jasper Yun (260651891)
 * 
 * ECSE 541: Design of MPSoC
 * Fall 2021
 * McGill University
 * 
 */

#include "sw_component.h"
#include "main.h"

using namespace std;

// get global variables from main.cpp
extern unsigned int addrC;
extern unsigned int addrA;
extern unsigned int addrB;
extern unsigned int matrix_size;
extern unsigned int loops;
extern volatile unsigned int software_cycles;

/**
 * Performs the matrix-matrix multiplications
 * and calls the hardware co-processor through
 * the shared bus.
 * 
 * For Part1, inner-most loop is partitioned to 
 * hardware co-processor.
 */
void Sw_component::do_sw_component()
{
    unsigned int n; 
    unsigned int i, j, k;

    wait(DELAY_SW_ADD); // set n = 0
    software_cycles++;
    for (n = 0; n < loops; n++)
    {
        wait(DELAY_SW_ADD); wait(DELAY_SW_CMP); // n++, compare to loops
        software_cycles += 2;

        cout << "[Sw_component] loop = " << n + 1 << endl;
        #ifdef DEBUG_SW
        cout << "[Sw_component] loop = " << n << endl;
        debug_log_file << "[Sw_component] loop = " << n << endl;
        #endif

        // initialize output region to zero
        #ifdef DEBUG_SW
        cout << "[Sw_component] Requesting : MST_ID = " << MST_ID_SW << ", ADDR = " << ADDR_MEM_MEM + addrC <<", OP = OP_WRITE, len = " << matrix_size * matrix_size << endl;
        debug_log_file << "[Sw_component] Requesting : MST_ID = " << MST_ID_SW << ", ADDR = " << ADDR_MEM_MEM + addrC <<", OP = OP_WRITE, len = " << matrix_size * matrix_size << endl;
        #endif
        wait(DELAY_SW_ADD * 2); wait(DELAY_SW_MUL * 3);
        software_cycles += 20;

        if_bus->Request(MST_ID_SW, ADDR_MEM_MEM + addrC + n * matrix_size * matrix_size, OP_WRITE, matrix_size * matrix_size); // one transaction
        
        while (!(if_bus->WaitForAcknowledge(MST_ID_SW))) // will wait until ack signal is received
        {
            software_cycles++;
            wait(Clk.posedge_event()); // wait for next positive clock edge
            #ifdef DEBUG_SW
            cout << "[Sw_component] Waiting for Acknowledge from memory..." << endl;
            debug_log_file << "[Sw_component] Waiting for Acknowledge from memory..." << endl;
            #endif
        }
        #ifdef DEBUG_SW
        cout << "[Sw_component] Acknowledge received from memory" << endl;
        debug_log_file << "[Sw_component] Acknowledge received from memory" << endl;
        #endif
        
        wait(DELAY_SW_ADD); // i = 0
        software_cycles++;
        for (i = 0; i < matrix_size * matrix_size; i++)
        {
            wait(DELAY_SW_ADD); wait(DELAY_SW_CMP); wait(DELAY_SW_MUL); // i++, compare, matrix_size * matrix_size
            software_cycles += 8;

            if_bus->WriteData(0); // initialize Matrix C to all zeros
            #ifdef DEBUG_SW
            cout << "[Sw_component] WriteData = 0, i = " << i << endl;
            debug_log_file << "[Sw_component] WriteData = 0, i = " << i << endl;
            #endif
        }

        // do matrix multiplications -- in Part1, inner-most loop partitioned to hardware (row * column)
        // software reads back the result of (row * column) and stores a local array in software to assemble a single row of matrix C.

        // vector<unsigned int> reg_A(matrix_size, 0); // size matrix_size, init to 0
        // vector<unsigned int> reg_B(matrix_size, 0); // in actual hardware, would be registers with fixed size that do not need init to zero
        unsigned int reg_A, reg_B, reg_C;

        wait(DELAY_SW_ADD); // i = 0
        software_cycles++;
        for (i = 0; i < matrix_size; i++) // row number -- execs: matrix_size * loops, iters: matrix_size
        {
            wait(DELAY_SW_ADD); wait(DELAY_SW_CMP); // i++, compare
            software_cycles += 2;
            
            wait(DELAY_SW_ADD); // j = 0
            software_cycles++;
            for (j = 0; j < matrix_size; j++) // column number -- execs: matrix_size * matrix_size * loops, iters: matrix_size
            {
                wait(DELAY_SW_ADD); wait(DELAY_SW_CMP); // j++, compare
                software_cycles += 2;

                wait(DELAY_SW_ADD); // k = 0
                for (k = 0; k < matrix_size; k++)
                {
                    wait(DELAY_SW_ADD); wait(DELAY_SW_CMP); // k++, compare
                    
                    wait(DELAY_SW_ADD); wait(DELAY_SW_MUL * 2); // add, 2 muls (mul is 2 operands only)
                    software_cycles += 13;
                    unsigned int base_mem_addr_loop = ADDR_MEM_MEM + n * matrix_size * matrix_size;

                    wait(DELAY_SW_ADD * 3); wait(DELAY_SW_MUL);
                    software_cycles += 9;
                    sw_master_read_data(base_mem_addr_loop + addrA + i * matrix_size + k, reg_A);
                    
                    wait(DELAY_SW_ADD * 3); wait(DELAY_SW_MUL);
                    software_cycles += 9;
                    sw_master_read_data(base_mem_addr_loop + addrB + j * matrix_size + k, reg_B);

                    wait(DELAY_SW_ADD * 3); wait(DELAY_SW_MUL);
                    software_cycles += 9;
                    sw_master_read_data(base_mem_addr_loop + addrC + i * matrix_size + j, reg_C);

                    wait(DELAY_SW_MUL); // multiply
                    software_cycles += 6;
                    unsigned int temp = reg_A * reg_B;
                    
                    wait(DELAY_SW_ADD);
                    software_cycles++;
                    reg_C += temp;

                    wait(DELAY_SW_ADD * 3); wait(DELAY_SW_MUL);
                    software_cycles += 9;
                    sw_master_write_data(base_mem_addr_loop + addrC + (i * matrix_size + j), reg_C);
                }
            }
        }
    }

    sc_stop(); // get here means finished multiplying, terminate program
}

/**
 * Consturctor sets up thread and makes sensitive to clock.
 */
SC_HAS_PROCESS(Sw_component);
Sw_component::Sw_component(sc_module_name name) : sc_module(name)
{
    SC_THREAD(do_sw_component);
    sensitive << Clk.pos(); // make sensitive to clock positive edges
}

/**
 * Wrapper for writing datum onto bus.
 */ 
void Sw_component::sw_master_write_data(unsigned int addr, unsigned int data)
{
    if_bus->Request(MST_ID_SW, addr, OP_WRITE, 1);
    while (!(if_bus->WaitForAcknowledge(MST_ID_SW))) 
    {
        wait(Clk.posedge_event());
        software_cycles++;
    }
    if_bus->WriteData(data);
}

/**
 * Wrapper for reading datum off bus.
 */
void Sw_component::sw_master_read_data(unsigned int addr, unsigned int& datum)
{
    // request and wait
    #ifdef DEBUG_SW
    debug_log_file << "[Sw_component] sw_master_read_data() : Requesting : MST_ID = " << MST_ID_HW << ", ADDR = " << addr <<", OP = OP_READ, len = " << matrix_size << endl;
    #endif
    if_bus->Request(MST_ID_SW, addr, OP_READ, 1);
    
    while (!(if_bus->WaitForAcknowledge(MST_ID_SW))) // blocking function, hangs
    {
        wait(Clk.posedge_event());
        software_cycles++;
        #ifdef DEBUG_SW
        debug_log_file << "[Sw_component] sw_master_read_data() : Waiting for acknowledge from memory" << endl;
        #endif
    }
    
    if_bus->ReadData(datum);

    #ifdef DEBUG_SW
    debug_log_file << "[Sw_component] sw_master_read_data() : Reading, i = " << i << endl;
    #endif

    // to avoid messing with bus timing, put this in bulk wait after read data is done
    wait(DELAY_SW_ADD);
    software_cycles++;
}

/**
 * Wrapper for reading data off bus.
 */
void Sw_component::sw_master_read_data(unsigned int addr, vector<unsigned int>& reg)
{
    // request and wait
    #ifdef DEBUG_SW
    debug_log_file << "[Sw_component] sw_master_read_data() : Requesting : MST_ID = " << MST_ID_HW << ", ADDR = " << addr <<", OP = OP_READ, len = " << matrix_size << endl;
    #endif
    if_bus->Request(MST_ID_SW, addr, OP_READ, matrix_size);
    
    while (!(if_bus->WaitForAcknowledge(MST_ID_SW))) // blocking function, hangs
    {
        wait(Clk.posedge_event());
        software_cycles++;
        #ifdef DEBUG_SW
        debug_log_file << "[Sw_component] sw_master_read_data() : Waiting for acknowledge from memory" << endl;
        #endif
    }
    
    for (unsigned int i = 0; i < matrix_size; i++ )
    {
        unsigned int data = 0;
        if_bus->ReadData(data);
        reg[i] = data;

        #ifdef DEBUG_SW
        debug_log_file << "[Sw_component] sw_master_read_data() : Reading, i = " << i << endl;
        #endif
    }

    // bulk wait for function accumulates to (matrix_size) * (DELAY_SW_ADD*3 + DELAY_SW_CMP) + DELAY_SW_ADD
    // to avoid messing with bus timing, put this in bulk wait after read data is done
    wait(matrix_size * (DELAY_SW_ADD * 3 + DELAY_SW_CMP) + DELAY_SW_ADD);
    software_cycles += matrix_size * (3 + 1) + 1;
}