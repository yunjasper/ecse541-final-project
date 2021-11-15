/**
 * --------
 * main.cpp
 * --------
 * 
 * Main function to run all the other code for HW2.
 * 
 * Author: Jasper Yun (260651891)
 * 
 * ECSE 541: Design of MPSoC
 * Fall 2021
 * McGill University
 * 
 */

#include "systemc.h"
#include "main.h"
#include "sw_component.h"
#include "hw_component.h"
#include "memory.h"
#include "shared_bus.h"
#include "oscillator.h"

#include <iostream> // for printing debug messages to file
#include <fstream> 

// global variables to store defaults, initialized to defaults
unsigned int addrC = ADDR_C;
unsigned int addrA = ADDR_A;
unsigned int addrB = ADDR_B;
unsigned int matrix_size = MATRIX_SIZE; // matrix is (size x size) elements
unsigned int loops = LOOPS;
unsigned int mem_size = MEM_SIZE;

volatile unsigned int software_cycles = 0; // timing cycles for software

ofstream debug_log_file;

int sc_main(int argc, char* argv[])
{
    char* mem_filename;

    debug_log_file.open ("log_part1.txt", ofstream::trunc); // open in truncate mode, delete previous contents

    // print info about this program
    cout << "\n\nECSE 541: MPSoC Design, HW2 -- Part1" << endl;
    cout << "Software-Hardware Partitioned Matrix-Matrix Multiplication" << endl;
    debug_log_file << "ECSE 541: MPSoC Design, HW2 -- Part1" << endl;
    debug_log_file << "Software-Hardware Partitioned Matrix-Matrix Multiplication" << endl;

    // check CLI arguments
    if (argc != 2 && argc != 5 && argc != 6 && argc != 7)
    {
        cerr << "Usage: " << argv[0] << " <filename> [[[addrC addrA addrB] size] loops]" << endl;
        debug_log_file << "Usage: " << argv[0] << " <filename> [[[addrC addrA addrB] size] loops]" << endl;
        return -1;
    }

    // set custom parameters
    if (argc >= 5)
    {
        addrC = stoi(argv[2]);
        addrA = stoi(argv[3]);
        addrB = stoi(argv[4]);

        if (argc >= 6)
            matrix_size = stoi(argv[5]);
        
        if (argc == 7)
            loops = stoi(argv[6]);
    }

    // determine minimum memory size
    mem_size = matrix_size * matrix_size * loops + addrC + 2; // +2 to avoid dumb out of bounds problems

    cout << "addrA = " << addrA << ", addrB = " << addrB << ", addrC = " << addrC << endl;
    cout << "matrix_size = " << matrix_size << ", loops = " << loops  << ", mem_size = " << mem_size << endl;
    debug_log_file << "addrA = " << addrA << ", addrB = " << addrB << ", addrC = " << addrC << endl;
    debug_log_file << "matrix_size = " << matrix_size << ", loops = " << loops  << ", mem_size = " << mem_size << endl;

    // check addresses against memory size
    if (addrA + matrix_size * matrix_size * loops > addrB)
    {
        cerr << "*** ERROR in main: addrA + matrix_size  > addrB" << endl;
        debug_log_file << "*** ERROR in main: addrA + matrix_size  > addrB" << endl;
        return -1;
    }

    if (addrB + matrix_size * matrix_size * loops > addrC) 
    {
        cerr << "*** ERROR in main: addrB + matrix_size > addrC" << endl;
        debug_log_file << "*** ERROR in main: addrA + matrix_size  > addrB" << endl;
        return -1;
    }

    if (ADDR_MEM_MEM + mem_size > ADDR_MEM_HW)
    {
        cerr << "*** ERROR in main: ADDR_MEM_MEM + mem_size > ADDR_MEM_HW (memory encroaching on hardware memory-mapped IO)" << endl;
        debug_log_file << "*** ERROR in main: ADDR_MEM_MEM + mem_size > ADDR_MEM_HW (memory encroaching on hardware memory-mapped IO)" << endl;
        return -1;
    }

    #ifdef DEBUG
    cout << "mem_size = " << mem_size << endl;
    debug_log_file << "mem_size = " << mem_size << endl;
    #endif

    // got here, means we can proceed
    mem_filename = argv[1];

    // declare modules
    Shared_bus bus1("bus1");
    Oscillator osc1("osc1");
    Sw_component sw1("sw1");
    Hw_component hw1("hw1");
    Memory mem1("mem1", mem_filename);

    sc_signal<sc_logic> global_clk;

    // // connect bus ports of different modules
    sw1.if_bus(bus1);
    mem1.if_bus(bus1);
    hw1.if_bus_minion(bus1);
    hw1.if_bus_master(bus1);

    // // connect clock signal to every module
    osc1.clk(global_clk); // output to rest of bus
    sw1.Clk(global_clk);
    hw1.Clk(global_clk);
    mem1.Clk(global_clk);
    bus1.Clk(global_clk);

    // do simulation
    cout << "[main] Starting simulation! " << endl;
    debug_log_file << "[main] Starting simulation! " << endl;
    // sc_start();
    sc_start();

    cout << "[main] end of simulation" << endl;
    debug_log_file << "[main] end of simulation" << endl;

    double end_time = sc_time_stamp().to_seconds() * 1e9;
    cout << "[main] final simulation time: " << end_time << " ns" << endl;
    cout << "[main] total cycles: time / clock period = " << end_time / 6.67 << endl;
    cout << "[main] Total counted software cycles = " << software_cycles << endl;
    cout << "[main] final memory contents at addrC of each loop printed to log_part1.txt." << endl;
    debug_log_file << "[main] final simulation time: " << end_time << " ns" << endl;
    debug_log_file << "[main] total cycles: time / clock period = " << end_time / 6.67 << endl;
    debug_log_file << "[main] Total counted software cycles = " << software_cycles << endl;
    
    // print final contents of memory
    for (unsigned int i = 0; i < loops; i++)
    {
        debug_log_file << endl << "C matrix for loop = " << i << endl;
        mem1.print_memory_log(ADDR_MEM_MEM + addrC + i * matrix_size * matrix_size, ADDR_MEM_MEM + addrC + (i + 1) * matrix_size * matrix_size);
    }
    
    debug_log_file.close();

    return 0;
}