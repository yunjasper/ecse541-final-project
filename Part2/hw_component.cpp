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
    for (unsigned int i = 0; i < matrix_size * matrix_size; i++)
    {
        reg_B.push_back(0); // full matrix
    }

    for (unsigned int i = 0; i < matrix_size; i++)
    {
        reg_A.push_back(0); // just rows
        reg_C.push_back(0);
    }
    
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
        if_bus_minion->Listen(req_addr, req_op, req_len);

        #ifdef DEBUG_HW
        cout << "[Hw_component] Listen() : got back req_addr = " << req_addr << ", req_op = " << req_op << ", req_len = " << req_len << endl;
        debug_log_file << "[Hw_component] Listen() : got back req_addr = " << req_addr << ", req_op = " << req_op << ", req_len = " << req_len << endl;
        #endif

        if (req_addr == ADDR_MEM_HW)// && req_op == OP_CALC)
        {
            if_bus_minion->Acknowledge();
            
            // no data to receive since hardware will take over, so go to master part
            // but need to read a dummy data to satisfy bus
            unsigned int dummyData = 0;
            if_bus_minion->ReceiveWriteData(dummyData);

            #ifdef DEBUG_MEM
            cout << "[Hw_component] Acknowledged! Received dummy data!" << endl;
            debug_log_file << "[Hw_component] Acknowledged! Received dummy data!" << endl;
            #endif

            #ifdef DEBUG_HW
            debug_log_file << "matrix_A_row_num = " << matrix_A_row_number << ", matrix_B_row_num = " << matrix_B_col_number << ", loop = " << num_loops << endl;
            #endif
            // master part, do calculations
            //      1. get data from memory and write into desktop (len = 2 rows of matrices A and B + 1 write!)
            //      2. do calculation
            //      3. write finished data back out to memory
            // for Part1, hardware does A_row * B_column acceleration
            // for Part2, hardware does full calculation of a single row of C (row of A * matrix B)
            
            // 1. get data
            #ifdef DEBUG_HW
            debug_log_file << "[Hw_component] Requesting row and column data, printing initial register states:" << endl;
            debug_log_file << "[Hw_component] reg_A = "; hw_print_register(reg_A);
            debug_log_file << "[Hw_component] reg_B = "; hw_print_register(reg_B);
            #endif
            
            unsigned int base_mem_addr_loop = ADDR_MEM_MEM + num_loops * matrix_size * matrix_size;
            hw_master_read_data(base_mem_addr_loop + addrA + matrix_A_row_number * matrix_size, reg_A); // read row of A, row major
            
            if (matrix_A_row_number == 0) // only need to read once
                hw_master_read_data(base_mem_addr_loop + addrB, reg_B); // read matrix B, *col major*

            #ifdef DEBUG_HW
            debug_log_file << "base_mem_addr_loop = " << base_mem_addr_loop << 
                " addrA_eff = " << base_mem_addr_loop + addrA + matrix_A_row_number * matrix_size << 
                " addrB_eff = " << base_mem_addr_loop + addrB + matrix_B_col_number * matrix_size << 
                " addrC_eff = " << base_mem_addr_loop + addrC + (matrix_A_row_number * matrix_size) + matrix_B_col_number << endl;
            debug_log_file << "[Hw_component] reg_A = "; hw_print_register(reg_A);
            debug_log_file << "[Hw_component] reg_B = "; hw_print_register(reg_B);
            #endif

            // 2. do calculations (row of A * matrix B)
            for (unsigned int i = 0; i < matrix_size; i++)
            {
                // extract row of A and column of B
                vector<unsigned int> col_B; // should get destructed at the end of the loop
                for (unsigned int j = 0; j < matrix_size; j++)
                {
                    col_B.push_back(reg_B[i * matrix_size + j]);
                }
                
                unsigned int sum = 0;
                for (unsigned int j = 0; j < matrix_size; j++)
                {
                    sum += reg_A[j] * col_B[j];

                    #ifdef DEBUG_HW
                    debug_log_file << "[Hw_component] calculating A*B --> sum += " << reg_A[i] << " * " << reg_B[i] << " = " << sum << endl;
                    #endif
                }
                reg_C[i] = sum; // entry into row of C
                
            }

            // 3. write finished data (row * column) to memory. matrix C is stored as row major
            hw_master_write_data(base_mem_addr_loop + addrC + (matrix_A_row_number * matrix_size), reg_C);
            
            matrix_A_row_number++;
            if (matrix_A_row_number == matrix_size)
            {
                num_loops++;
                matrix_A_row_number = 0;
            }
        }
    }
}

/**
 * Reads array of data from bus into hardware component register.
 */
void Hw_component::hw_master_read_data(unsigned int addr, vector<unsigned int>& reg)
{
    // request and wait
    #ifdef DEBUG_HW
    debug_log_file << "[Hw_component] hw_master_read_data() : Requesting : MST_ID = " << MST_ID_HW << ", ADDR = " << addr <<", OP = OP_READ, len = " << matrix_size << endl;
    #endif
    if_bus_master->Request(MST_ID_HW, addr, OP_READ, reg.size());
    
    while (!(if_bus_master->WaitForAcknowledge(MST_ID_HW))) // blocking function, hangs
    {
        wait(Clk.posedge_event());
        #ifdef DEBUG_HW
        debug_log_file << "[Hw_component] hw_master_read_data() : Waiting for acknowledge from memory" << endl;
        #endif
    }
    
    for (unsigned int i = 0; i < reg.size(); i++ )
    {
        unsigned int data = 0;
        if_bus_master->ReadData(data);
        reg[i] = data;

        #ifdef DEBUG_HW
        debug_log_file << "[Hw_component] hw_master_read_data() : Reading, i = " << i << endl;
        #endif
    }
}

/**
 * Writes array of data onto bus from hardware component register.
 */
void Hw_component::hw_master_write_data(unsigned int addr, vector<unsigned int>& reg)
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
void Hw_component::hw_master_write_data(unsigned int addr, unsigned int data)
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
void Hw_component::hw_print_register(vector<unsigned int>& reg)
{
    for (unsigned int i = 0; i < matrix_size; i++)
        debug_log_file << reg[i] << " ";
    debug_log_file << endl;
}