/**
 * -----------------
 * bus_master_if.cpp
 * -----------------
 * 
 * Bus master interface header.
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

class bus_master_if : virtual public sc_interface
{
    public:
        virtual void Request(unsigned int mst_id, unsigned int addr, unsigned int op, unsigned int len) = 0;
        virtual bool WaitForAcknowledge(unsigned int mst_id) = 0;
        virtual void ReadData(unsigned int &data) = 0;
        virtual void WriteData(unsigned int data) = 0;
};