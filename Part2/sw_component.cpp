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
    unsigned int n, i;

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
        software_cycles += 2; // 2 clock cycles for request
        

        while (!(if_bus->WaitForAcknowledge(MST_ID_SW))) // will wait until ack signal is received
        {
            wait(Clk.posedge_event()); // wait for next positive clock edge
            software_cycles++;
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

        wait(DELAY_SW_ADD); // i = 0
        software_cycles++;
        for (i = 0; i < matrix_size; i++) // row number
        {
            wait(DELAY_SW_ADD); wait(DELAY_SW_CMP); // i++, compare
            software_cycles += 2;
            // request that hardware do processing.
            // request as SW master, requesting that bus minion do work (so use ADDR_MEM_HW), work is calculation (so OP_CALC)
            #ifdef DEBUG_SW
            cout << "[Sw_component] Requesting : MST_ID = " << MST_ID_SW << ", ADDR = " << ADDR_MEM_HW <<", OP = OP_CALC, len = " << 1 << endl;
            debug_log_file << "[Sw_component] Requesting : MST_ID = " << MST_ID_SW << ", ADDR = " << ADDR_MEM_HW <<", OP = OP_CALC, len = " << 1 << ", i = " << i << ", j = " << j << endl;
            #endif
            if_bus->Request(MST_ID_SW, ADDR_MEM_HW, OP_CALC, 1); // waste of a clock cycle but prevent early release of bus
            software_cycles += 2; // request takes 2 cycles
            
            while (!(if_bus->WaitForAcknowledge(MST_ID_SW)))
            {
                wait(Clk.posedge_event()); // needed otherwise loops forever here
                software_cycles++;
                #ifdef DEBUG_SW
                cout << "[Sw_component] Waiting for Acknowledge from hardware..." << endl;
                debug_log_file << "[Sw_component] Waiting for Acknowledge from hardware...i = " << i << ", j = " << j << endl;
                #endif
            }
            #ifdef DEBUG_SW
            cout << "[Sw_component] Acknowledge received from hardware! i = " << i << ", j = " << j << endl;
            debug_log_file << "[Sw_component] Acknowledge received from hardware! i = " << i << ", j = " << j << endl;
            #endif
            if_bus->WriteData(0); // dummy data

            // no timing penalty for these calculations because assume optimized at compile/design time
            #ifdef DATA_REUSE // all values tuned by hand :(
                    
                    if (matrix_size > 12)
                    {
                        if (i == 0)
                            wait((2 * matrix_size + 3) * matrix_size * CLOCK_PERIOD);
                        else
                            wait((6 * matrix_size) * CLOCK_PERIOD);
                    }
                    else if (matrix_size >= 6)
                    {
                        if (i == 0)
                            wait((100 * matrix_size) * CLOCK_PERIOD); // don't make new request before hardware is done
                        else
                            wait((6 * matrix_size) * CLOCK_PERIOD);
                    }
                    else if (matrix_size >= 4)
                    {
                        if (i == 0)
                            wait((15 * matrix_size) * CLOCK_PERIOD); // don't make new request before hardware is done
                        else
                            wait((10 * matrix_size) * CLOCK_PERIOD);
                    }
                    else if (matrix_size >= 2)
                    {
                        if (i == 0)
                            wait((13 * matrix_size) * CLOCK_PERIOD); // don't make new request before hardware is done
                        else
                            wait((10 * matrix_size) * CLOCK_PERIOD);
                    }
                    else if (matrix_size == 1)
                    {
                        wait((23 * matrix_size) * CLOCK_PERIOD); // no idea why this has to be so high, probably because no burst transactions at this point
                    }
                    
                #else
                    if (matrix_size >= 12)
                        wait((2 * matrix_size + 3) * matrix_size * CLOCK_PERIOD); // don't make new request before hardware is done
                    else if (matrix_size >= 6)
                        wait((100 * matrix_size) * CLOCK_PERIOD); // don't make new request before hardware is done
                    else if (matrix_size >= 4)
                        wait((15 * matrix_size) * CLOCK_PERIOD); // don't make new request before hardware is done
                    else if (matrix_size >= 2)
                        wait((13 * matrix_size) * CLOCK_PERIOD); // don't make new request before hardware is done
                    else if (matrix_size == 1)
                        wait((23 * matrix_size) * CLOCK_PERIOD); // no idea why this has to be so high, probably because no burst transactions at this point
                #endif
            
            // now wait for hardware to do mulitplication and store in memory
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
