/**
 * ----------------
 * hw_component.cpp
 * ----------------
 * 
 * Implements the hardware component
 * for HW2.
 * 
 * Author: Jasper Yun (260651891)
 * 
 * ECSE 541: Design of MPSoC
 * Fall 2021
 * McGill University
 * 
 */

#include "systemc.h"
#include "hw_component.h"

SC_HAS_PROCESS(Hw_component);

// constructor
Hw_component::Hw_component(sc_module_name name) : sc_module(name)
{
    #ifdef DEBUG
    cout << "[Hw_component] Starting hardware component constructor..." << endl; 
    debug_log_file << "[Hw_component] Starting hardware component constructor..." << endl; 
    #endif
    sensitive << Clk.pos(); // make sensitive to clock positive edges
    SC_THREAD(do_hw_component);

    #ifdef DEBUG
    cout << "[Hw_component] Initializing registers..." << endl; 
    debug_log_file << "[Hw_component] Initializing registers..." << endl; 
    #endif
    
    // initialize "registers"
    for (unsigned int i = 0; i < matrix_size; i++)
    {
        reg_Aik.push_back(0);
        reg_Lkj.push_back(0);
    }
    reg_Lij = 0;
    
    #ifdef DEBUG
    cout << "[Hw_component] End of hardware component constructor." << endl; 
    debug_log_file << "[Hw_component] End of hardware component constructor." << endl; 
    #endif
}

/**
 * Thread for hardware component which runs forever once started.
 */
void Hw_component::do_hw_component()
{
    while (true)
    {
        // minion part, wait for software call
        if_bus_minion->Listen(req_addr, req_op, req_len, req_indexing_mode);

        #ifdef DEBUG_HW
        cout << "[Hw_component] Listen() : got back req_addr = " << req_addr << ", req_op = " << req_op << ", req_len = " << req_len << endl;
        debug_log_file << "[Hw_component] Listen() : got back req_addr = " << req_addr << ", req_op = " << req_op << ", req_len = " << req_len << endl;
        #endif

        if (req_addr == ADDR_MEM_HW)// && req_op == OP_CALC)
        {
            if_bus_minion->Acknowledge();
            
            // no data to receive since hardware will take over, so go to master part
            // but need to read a dummy data to satisfy bus
            double dummyData = 0;
            if_bus_minion->ReceiveWriteData(dummyData);

            #ifdef DEBUG_MEM
            cout << "[Hw_component] Acknowledged! Received dummy data!" << endl;
            debug_log_file << "[Hw_component] Acknowledged! Received dummy data!" << endl;
            #endif

            #ifdef DEBUG_HW
            debug_log_file << "matrix_A_row_num = " << matrix_A_row_number << ", loop = " << num_loops << endl;
            #endif
            // master part, do calculations
            //      1. get data from memory and write into desktop (len = 2 rows of matrices A and B + 1 write!)
            //      2. do calculation
            //      3. write finished data back out to memory
            // for Part1, hardware does A_row * B_column acceleration
            // for Part2, hardware does full calculation of a single row of C
            
            // 1. get data
            #ifdef DEBUG_HW
            debug_log_file << "[Hw_component] Requesting row and column data, printing initial register states:" << endl;
            debug_log_file << "[Hw_component] reg_Aik = "; hw_print_register(reg_Aik);
            #endif
            
            unsigned int base_mem_addr_loop = ADDR_MEM_MEM + num_loops * matrix_size * matrix_size;
            hw_master_read_data(base_mem_addr_loop + addrA + row_number * matrix_size, reg_Aik); // read A_ik
            hw_master_read_data(base_mem_addr_loop + addrL + row_number * matrix_size + col_number, reg_Lij); // read L_ij
            hw_master_read_data(base_mem_addr_loop + addrL + row_number * matrix_size + col_number, reg_Lkj, COL_MJR); // read L_kj

            #ifdef DEBUG_HW
            debug_log_file << "base_mem_addr_loop = " << base_mem_addr_loop << 
                " addrA_eff = " << base_mem_addr_loop + addrA + matrix_A_row_number * matrix_size << 
                " addrL_eff = " << base_mem_addr_loop + addrL + SOME_OFFSET << endl;
            debug_log_file << "[Hw_component] reg_Aik = "; hw_print_register(reg_Aik);
            #endif

            // 2. do calculations
            for(int i = 0; i < row_number - (col_number + 1) + 1; i++){
                reg_Aik[i] -= reg_Lij * reg_Lkj[i];
            }

            // 3. write finished data (row * column) to memory. matrix C is stored as row major
            hw_master_write_data(base_mem_addr_loop + addrA + row_number * matrix_size, reg_Aik);

            // increment matrix row/column position and loop counters
            row_number++;
            if (row_number == matrix_size)
            {
                col_number++;
                row_number = 0;
            }
            if (col_number == matrix_size)
            {
                num_loops++;
                col_number = 0;
            }
        }
    }
}

/**
 * Reads array of data from bus into hardware component register.
 */
void Hw_component::hw_master_read_data(unsigned int addr, vector<double>& reg, unsigned int indexing_mode)
{
    // request and wait
    #ifdef DEBUG_HW
    debug_log_file << "[Hw_component] hw_master_read_data() : Requesting : MST_ID = " << MST_ID_HW << ", ADDR = " << addr <<", OP = OP_READ, len = " << matrix_size << endl;
    #endif
    if_bus_master->Request(MST_ID_HW, addr, OP_READ, row_number - (col_number + 1) + 1, indexing_mode);
    
    while (!(if_bus_master->WaitForAcknowledge(MST_ID_HW))) // blocking function, hangs
    {
        wait(Clk.posedge_event());
        #ifdef DEBUG_HW
        debug_log_file << "[Hw_component] hw_master_read_data() : Waiting for acknowledge from memory" << endl;
        #endif
    }
    
    for (unsigned int i = 0; i < matrix_size; i++ )
    {
        double data = 0;
        if_bus_master->ReadData(data);
        reg[i] = data;

        #ifdef DEBUG_HW
        debug_log_file << "[Hw_component] hw_master_read_data() : Reading, i = " << i << endl;
        #endif
    }
}

/**
 * Reads single datum from bus into hardware component register.
 */
void Hw_component::hw_master_read_data(unsigned int addr, double& reg, unsigned int indexing_mode)
{
    // request and wait
    #ifdef DEBUG_HW
    debug_log_file << "[Hw_component] hw_master_read_data() : Requesting : MST_ID = " << MST_ID_HW << ", ADDR = " << addr <<", OP = OP_READ, len = " << matrix_size << endl;
    #endif
    if_bus_master->Request(MST_ID_HW, addr, OP_READ, 1, indexing_mode);
    
    while (!(if_bus_master->WaitForAcknowledge(MST_ID_HW))) // blocking function, hangs
    {
        wait(Clk.posedge_event());
        #ifdef DEBUG_HW
        debug_log_file << "[Hw_component] hw_master_read_data() : Waiting for acknowledge from memory" << endl;
        #endif
    }
    
    double data = 0;
    if_bus_master->ReadData(data);
    reg = data;

    #ifdef DEBUG_HW
    debug_log_file << "[Hw_component] hw_master_read_data() : Reading, i = " << i << endl;
    #endif
}

/**
 * Writes array of data onto bus from hardware component register.
 */
void Hw_component::hw_master_write_data(unsigned int addr, vector<double>& reg)
{
    // request and wait
    #ifdef DEBUG_HW
    debug_log_file << "[Hw_component] hw_master_write_data() : Requesting : MST_ID = " << MST_ID_HW << ", ADDR = " << addr <<", OP = OP_WRITE, len = " << matrix_size << endl;
    #endif
    if_bus_master->Request(MST_ID_HW, addr, OP_WRITE, matrix_size);
    
    while (!(if_bus_master->WaitForAcknowledge(MST_ID_HW))) // blocking function, hangs
    {
        wait(Clk.posedge_event());
        #ifdef DEBUG_HW
        debug_log_file << "[Hw_component] hw_master_write_data() : Waiting for acknowledge from memory" << endl;
        #endif
    }
    
    for (unsigned int i = 0; i < matrix_size; i++ )
    {
        if_bus_master->WriteData(reg[i]);

        #ifdef DEBUG_HW
        debug_log_file << "[Hw_component] hw_master_write_data() : Writing, i = " << i << endl;
        #endif
    }
}

/**
 * Writes single datum onto bus from hardware component register.
 */
void Hw_component::hw_master_write_data(unsigned int addr, double data)
{
    // request and wait
    #ifdef DEBUG_HW
    debug_log_file << "[Hw_component] hw_master_write_data() : Requesting : MST_ID = " << MST_ID_HW << ", ADDR = " << addr <<", OP = OP_WRITE, len = " << 1 << endl;
    #endif
    if_bus_master->Request(MST_ID_HW, addr, OP_WRITE, 1);
    
    while (!(if_bus_master->WaitForAcknowledge(MST_ID_HW))) // blocking function, hangs
    {
        wait(Clk.posedge_event());
        #ifdef DEBUG_HW
        debug_log_file << "[Hw_component] hw_master_write_data() : Waiting for acknowledge from memory" << endl;
        #endif
    }
    
    #ifdef DEBUG_HW
    debug_log_file << "[Hw_component] hw_master_write_data() single : Written" << endl;
    #endif
    if_bus_master->WriteData(data);
}

/**
 * Prints contents of a register.
 */
void Hw_component::hw_print_register(vector<double>& reg)
{
    for (unsigned int i = 0; i < matrix_size; i++)
        debug_log_file << reg[i] << " ";
    debug_log_file << endl;
}