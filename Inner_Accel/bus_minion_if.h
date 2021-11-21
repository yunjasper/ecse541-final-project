/**
 * -----------------
 * bus_minion_if.cpp
 * -----------------
 * 
 * Bus minion interface header.
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
#include "main.h" // to get defines

class bus_minion_if : virtual public sc_interface
{
    public:
        virtual void Listen(unsigned int &req_addr, unsigned int &req_op, unsigned int &req_len, unsigned int &req_indexing_mode, 
                            unsigned int &req_j_loop, unsigned int &req_i_loop) = 0;
        virtual void Acknowledge() = 0;
        virtual void SendReadData(double data) = 0;
        virtual void ReceiveWriteData(double &data) = 0;
};