/**
 * --------------
 * hw_component.h
 * --------------
 * 
 * Class declaration for the
 * hardware component.
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
#include "bus_master_if.h"
#include "bus_minion_if.h"
#include "main.h"
#include <vector> // dynamic register sizing

// get global variables
extern unsigned int matrix_size;
extern unsigned int addrA, addrL;
extern ofstream debug_log_file;

class Hw_component : public sc_module
{
    public:
        // ports
        sc_port<bus_master_if> if_bus_master; // port to access hardware via shared bus as master
        sc_port<bus_minion_if> if_bus_minion; // port to access hardware via shared bus as minion
        sc_in<sc_logic> Clk; // clock signal

        // variables to keep track of row/column to be mulitplying
        // when hardware responds to request, increment
        unsigned int row_number = 0;
        unsigned int col_number = 0;
        unsigned int num_loops = 0;

        // hardware processor has to store data somewhere, call them registers
        // since this is a custom processor, make the registers as large as needed
        // for Part1, do row * column acceleration
        // for Part2, do matrix * matrix acceleration
        vector<double> reg_Aik;
        double reg_Lij;
        vector<double> reg_Lkj;

        // to listen to bus, need variables to store request details
        unsigned int req_addr, req_op, req_len, req_indexing_mode;

        // constructor
        Hw_component(sc_module_name name);
        void do_hw_component();
        void hw_master_read_data(unsigned int addr, vector<double>& reg, unsigned int indexing_mode = ROW_MJR);
        void hw_master_read_data(unsigned int addr, double& reg, unsigned int indexing_mode = ROW_MJR);
        void hw_master_write_data(unsigned int addr, vector<double>& reg); // write array
        void hw_master_write_data(unsigned int addr, double data); // write single
        void hw_print_register(vector<double>& reg);
};