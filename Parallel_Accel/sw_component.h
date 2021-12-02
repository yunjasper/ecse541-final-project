/**
 * --------------
 * sw_component.h
 * --------------
 * 
 * Class declaration for the 
 * software component.
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
#include <vector>

extern ofstream debug_log_file;

class Sw_component : public sc_module
{
    public:
        sc_in<sc_logic> Clk;
        sc_in<sc_logic> Hw_done;
        sc_port<bus_master_if> if_bus;  // port to access hardware via shared bus as master

        Sw_component(sc_module_name name); // constructor
        void do_sw_component();
        void sw_master_write_data(unsigned int addr, double data);
        void sw_master_read_data(unsigned int addr, std::vector<double>& reg);
        void sw_master_read_data(unsigned int addr, double& datum);
};