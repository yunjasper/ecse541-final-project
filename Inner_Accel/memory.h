/**
 * --------
 * memory.h
 * --------
 * 
 * Header file for the memory component for HW2.
 * 
 * Author: Jasper Yun (260651891)
 * 
 * ECSE 541: Design of MPSoC
 * Fall 2021
 * McGill University
 * 
 */

#include "systemc.h"
#include "bus_minion_if.h"
#include "main.h" // to get defines

#include <sstream>
#include <fstream>
#include <iostream>
#include <vector> // for dynamic memory array size

// get global variables from main.cpp
extern unsigned int addrL;
extern unsigned int addrA;
extern unsigned int mem_size;
extern unsigned int matrix_size;
extern unsigned int loops;

extern ofstream debug_log_file;

// given in instructions
class Memory: public sc_module
{
    public:
        sc_in<sc_logic> Clk;            // clock signal
        sc_port<bus_minion_if> if_bus;  // port to shared bus (memory is always minion)
        
        vector<double> memData; // empty, no elements
        unsigned int req_addr, req_op, req_len, req_indexing_mode, req_j_loop, req_i_loop; // received bus request details
        
        // thread for memory
        void do_memory() 
        {
            while (true)
            {
                // listen to bus and wait
                if_bus->Listen(req_addr, req_op, req_len, req_indexing_mode, req_j_loop, req_i_loop); // wait to receive bus request details

                #ifdef DEBUG_MEM
                cout << "[Memory] Listen() : got back req_addr = " << req_addr << ", req_op = " << req_op << ", req_len = " << req_len << endl;
                debug_log_file << "[Memory] Listen() : got back req_addr = " << req_addr << ", req_op = " << req_op << ", req_len = " << req_len << endl;
                #endif

                // check if bus request address is in memory range
                // memory starts at ADDR_MEM_MEM and ends at ADDR_MEM_MEM + mem_size
                // note the lowercase mem_size because CLI args may change defaults
                if (req_addr >= ADDR_MEM_MEM && req_addr <= ADDR_MEM_MEM + mem_size)
                {
                    // req_op should NOT be OP_NONE or OP_CALC
                    if (req_op == OP_NONE || req_op == OP_CALC)
                    {
                        cerr << "[Memory] ***ERROR: requested operation is invalid in memory" << endl;
                        debug_log_file << "[Memory] ***ERROR: requested operation is invalid in memory" << endl;
                    }
                    // if match, do acknowledge
                    if_bus->Acknowledge();

                    #ifdef DEBUG_MEM
                    cout << "[Memory] Acknowledged!" << endl;
                    debug_log_file << "[Memory] Acknowledged! current_master.addr = " << req_addr << endl;
                    #endif

                    // check that valid arguments were given
                    if (req_addr - ADDR_MEM_MEM + req_len >= mem_size)
                    {
                        cerr << "[Memory] ***ERROR: requested length is too large in memory" << endl;
                        debug_log_file << "[Memory] ***ERROR: requested length is too large in memory" << endl;
                        cerr << "[Memory] Requested address = " << req_addr - ADDR_MEM_MEM + req_len << endl;
                    }
                    // if match for memory read, do memory read and write data onto bus
                    if (req_op == OP_READ)
                    {
                        if(req_indexing_mode == ROW_MJR)
                        {
                            for (unsigned int i = 0; i < req_len; i++)
                            {
                                // adjust passed address to within array dimensions
                                #ifdef DEBUG_MEM
                                debug_log_file << "[Memory] SendReadData : addr = " << req_addr << endl;
                                #endif
                                if_bus->SendReadData(memData[req_addr - ADDR_MEM_MEM + i]);
                            }
                        }
                        else if(req_indexing_mode == COL_MJR)
                        {
                            for (unsigned int i = 0; i < req_len; i++)
                            {
                                // adjust passed address to within array dimensions
                                #ifdef DEBUG_MEM
                                debug_log_file << "[Memory] SendReadData : addr = " << req_addr << endl;
                                #endif
                                if_bus->SendReadData(memData[req_addr - ADDR_MEM_MEM + i*matrix_size]);
                            }
                        }
                        else
                        {
                            cerr << "[Memory] ***ERROR: requested indexing mode is invalid" << endl;
                            debug_log_file << "[Memory] ***ERROR: requested indexing mode is invalid" << endl;
                        }
                        
                    }
                    
                    // if match for memory write, get bus data and write into memory
                    else if (req_op == OP_WRITE)
                    {
                        for (unsigned int i = 0; i < req_len; i++)
                        {
                            double data;
                            if_bus->ReceiveWriteData(data);
                            memData[req_addr - ADDR_MEM_MEM + i] = data;
                            #ifdef DEBUG_MEM
                            debug_log_file << "[Memory] ReceiveWriteData : addr = " << req_addr << endl;
                            #endif
                        }
                    }
                }

                #ifdef DEBUG_MEM
                debug_log_file << "[Memory] State of memory: " << endl;
                // print_memory_log(0, MEM_SIZE - 1);
                #endif

                wait(Clk.posedge_event()); // let's just see if this works
            }
        }

        // constructor
        SC_HAS_PROCESS(Memory);
        Memory(sc_module_name name, char* memInitFilename): sc_module(name)
        {
            // initialize memory (copied from memory_component.h)
            #ifdef DEBUG_MEM
            cout << "[Memory] Beginning memory initialization..." << endl;
            debug_log_file << "[Memory] Beginning memory initialization..." << endl;
            #endif

            // reading file
            string line;
            ifstream file;
            file.open(memInitFilename);

            // check if file is open
            if (!file.is_open())
            {
                cerr << "[Memory] ***ERROR! Cannot open file for memory initialization, setting all to zero" << endl;
                debug_log_file << "[Memory] ***ERROR! Cannot open file for memory initialization, setting all to zero" << endl;
                for (unsigned int i = 0; i < mem_size; i++)
                {
                    memData.push_back(0);
                }
            }

            string token;
            getline(file, line);
            istringstream iss(line);
            unsigned int position = 0;

            // beware that input file could be too long!
            while (getline(iss, token, ' '))
            {
                if (position == mem_size)
                {
                    cerr << "[Memory] Input file too long! Stopped." << endl;
                    debug_log_file << "[Memory] Input file too long! Stopped." << endl;
                    break;
                }

                memData.push_back(stoi(token));
                position++;
            }

            // initialize rest of memory to zero if file too short
            if (position != mem_size)
            {
                cerr << "[Memory] Input file too short! Initializing rest of memory to zero" << endl;
                debug_log_file << "[Memory] Input file too short! Initializing rest of memory to zero" << endl;
                while (position < mem_size)
                {
                    memData.push_back(0);
                    position++;

                }
            }
            
            #ifdef DEBUG_MEM
            cout << "[Memory] Success! Initialized memory" << endl;
            #endif
            
            // register do_memory() as process and make sensitive to clock changes
            SC_THREAD(do_memory);
            sensitive << Clk.pos();
            
            #ifdef DEBUG_MEM
            cout << "[Memory] Exiting memory initialization constructor" << endl;
            debug_log_file << "[Memory] Exiting memory initialization constructor" << endl;
            cout << "[Memory] State of memory now:" << endl;
            print_memory(ADDR_MEM_MEM + addrA, ADDR_MEM_MEM + addrL + matrix_size);
            #endif
        }

        /**
         * prints contents of memory from start to end addresses
         * @param start start print address
         * @param end end print address
         * @return void
         */
        void print_memory(unsigned int start, unsigned int end)
        {
            // check validity of passed arguments
            if (start < ADDR_MEM_MEM)
            {
                start = ADDR_MEM_MEM;
                cerr << "[Memory] Error! Start address smaller than input 1 address" << endl;
                debug_log_file << "[Memory] Error! Start address smaller than input 1 address" << endl;
            }

            if (end >= ADDR_MEM_MEM + mem_size)
            {
                end = ADDR_MEM_MEM + mem_size - 1;
                cerr << "[Memory] Error! End address larger than memory size" << endl;
                debug_log_file << "[Memory] Error! End address larger than memory size" << endl;
            }

            // print
            for (unsigned i = start; i < end; i++)
            {
                cout << memData[i - ADDR_MEM_MEM] << " ";
                // if ((i - ADDR_MEM_MEM) % matrix_size == 0 && i != start)
                //     cout << endl; // newline
            }
            cout << endl;
        }

        void print_memory_log(unsigned int start, unsigned int end)
        {
            // check validity of passed arguments
            if (start < ADDR_MEM_MEM)
            {
                start = ADDR_MEM_MEM;
                cerr << "[Memory] Error! Start address smaller than input 1 address" << endl;
                debug_log_file << "[Memory] Error! Start address smaller than input 1 address" << endl;
            }

            if (end >= ADDR_MEM_MEM + mem_size)
            {
                end = ADDR_MEM_MEM + mem_size - 1;
                cerr << "[Memory] Error! End address larger than memory size" << endl;
                debug_log_file << "[Memory] Error! End address larger than memory size" << endl;
            }

            // print
            for (unsigned i = start; i < end; i++)
            {
                debug_log_file << memData[i - ADDR_MEM_MEM] << " ";
                // if ((i - ADDR_MEM_MEM) % matrix_size == 0 && i != start)
                //     cout << endl; // newline
            }
            debug_log_file << endl;
        }

};