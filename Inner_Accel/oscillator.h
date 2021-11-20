/**
 * ------------
 * oscillator.h
 * ------------
 * 
 * Implements a clock signal.
 * Functional code was provided in assignment instructions.
 * Same as from HW1.
 * 
 * Author: Jasper Yun (260651891)
 * 
 * ECSE 541: Design of MPSoC
 * Fall 2021
 * McGill University
 * 
 */

#pragma once

#include "systemc.h"
#include "main.h"

extern ofstream debug_log_file;

class Oscillator: public sc_module
{
    public:
        sc_out<sc_logic> clk; // output is a clock signal

        // constructor
        SC_HAS_PROCESS(Oscillator);
        Oscillator(sc_module_name name): sc_module(name)
        {
            SC_THREAD(do_oscillator);
        }
        
        void do_oscillator()
        {
            while (true)
            {
                clk.write(SC_LOGIC_0);
                wait(CLOCK_PERIOD / 2);
                clk.write(SC_LOGIC_1);
                wait(CLOCK_PERIOD / 2);
                #ifdef DEBUG
                debug_log_file << "\n[Oscillator] period elapsed: " << sc_time_stamp().to_seconds() * 1e9 << endl << endl;
                #endif
            }
        }
};