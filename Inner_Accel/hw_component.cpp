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

extern volatile unsigned int hardware_cycles;
extern volatile unsigned int bus_cycles;

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
        reg_Aik.push_back(0.0);
        reg_Lkj.push_back(0.0);
    }
    reg_Lij = 0.0;
    
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
        if_bus_minion->Listen(req_addr, req_op, req_len, req_indexing_mode, req_j_loop, req_i_loop);
        col_number = req_j_loop;
        row_number = req_i_loop;
        // ^^ basically renaming the variable, in optimized code we wouldn't need col/row_number so no cycle cost


        #ifdef DEBUG_HW
        cout << "[Hw_component] Listen() : got back req_addr = " << req_addr << ", req_op = " << req_op << ", req_len = " << req_len << endl;
        debug_log_file << "[Hw_component] Listen() : got back req_addr = " << req_addr << ", req_op = " << req_op << ", req_len = " << req_len << endl;
        #endif

        hardware_cycles += CYCLES_HW_CMP;
        if (req_addr == ADDR_MEM_HW)// && req_op == OP_CALC)
        {
            if_bus_minion->Acknowledge();
            
            // no data to receive since hardware will take over, so go to master part
            // but need to read a dummy data to satisfy bus
            double dummyData = 0; // can send whatever data, doesn't have to be zero, no cycle cost
            if_bus_minion->ReceiveWriteData(dummyData);

            #ifdef DEBUG_MEM
            cout << "[Hw_component] Acknowledged! Received dummy data!" << endl;
            debug_log_file << "[Hw_component] Acknowledged! Received dummy data!" << endl;
            #endif

            #ifdef DEBUG_HW
            debug_log_file << "row_num = " << row_number << ", loop = " << num_loops << endl;
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

            hardware_cycles += 3 * CYCLES_HW_ADD;
            data_len = row_number - (col_number + 1) + 1;\

            hardware_cycles += CYCLES_HW_ADD + CYCLES_HW_MUL * 2;
            unsigned int base_mem_addr_loop = ADDR_MEM_MEM + num_loops * matrix_size * matrix_size;
            
            hardware_cycles += CYCLES_HW_ADD * 4 + CYCLES_HW_MUL;
            hw_master_read_data(base_mem_addr_loop + addrA + row_number * matrix_size + (col_number + 1), reg_Aik); // read A_ik
            
            hardware_cycles += CYCLES_HW_ADD * 3 + CYCLES_HW_MUL;
            hw_master_read_data(base_mem_addr_loop + addrL + row_number * matrix_size + col_number, reg_Lij); // read L_ij
            
            hardware_cycles += CYCLES_HW_ADD * 4 + CYCLES_HW_MUL;
            hw_master_read_data(base_mem_addr_loop + addrL + (col_number + 1) * matrix_size + col_number, reg_Lkj, COL_MJR); // read L_kj

            #ifdef DEBUG_HW
            debug_log_file << "base_mem_addr_loop = " << base_mem_addr_loop << 
                " addrA_eff = " << base_mem_addr_loop + addrA + row_number * matrix_size << 
                " addrL_eff = " << base_mem_addr_loop + addrL + row_number * matrix_size + col_number << endl;
            debug_log_file << "[Hw_component] reg_Aik = "; hw_print_register(reg_Aik);
            #endif

            // 2. do calculations
            hardware_cycles += CYCLES_HW_ADD; // i = 0
            for(int i = 0; i < data_len; i++){
                hardware_cycles += CYCLES_HW_ADD + CYCLES_HW_CMP; // compare and i++

                hardware_cycles += CYCLES_HW_ADD + CYCLES_HW_MUL + CYCLES_HW_ADD; // last add is for accessing reg_Lkj[i]
                reg_Aik[i] -= reg_Lij * reg_Lkj[i];
            }

            // 3. write finished data (row * column) to memory. matrix C is stored as row major
            hardware_cycles += CYCLES_HW_ADD * 4 + CYCLES_HW_MUL;
            hw_master_write_data(base_mem_addr_loop + addrA + row_number * matrix_size + (col_number + 1), reg_Aik);

            // increment matrix row/column position and loop counters
            // cout << "row number: " << row_number << endl;
            // cout << "col number: " << col_number << endl;
            // cout << "matrix size: " << matrix_size << endl;
            Hw_done.write(SC_LOGIC_1);
            wait(Clk.posedge_event()); // at this point control is transferred back to software and things are in parallel again
            Hw_done.write(SC_LOGIC_0);

            if(row_number == matrix_size - 1){
                if(col_number == matrix_size - 2){
                    num_loops++;
                }
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
    if_bus_master->Request(MST_ID_HW, addr, OP_READ, data_len, indexing_mode);
    
    while (!(if_bus_master->WaitForAcknowledge(MST_ID_HW))) // blocking function, hangs
    {
        wait(Clk.posedge_event());
        bus_cycles++;
        #ifdef DEBUG_HW
        debug_log_file << "[Hw_component] hw_master_read_data() : Waiting for acknowledge from memory" << endl;
        #endif
    }
    
    hardware_cycles += CYCLES_HW_ADD; // i = 0
    for (unsigned int i = 0; i < data_len; i++ )
    {
        hardware_cycles += CYCLES_HW_ADD + CYCLES_HW_CMP;
        double data = 0;
        if_bus_master->ReadData(data);
        
        hardware_cycles += CYCLES_HW_ADD; // write data into reg[i]
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
        bus_cycles++;
        #ifdef DEBUG_HW
        debug_log_file << "[Hw_component] hw_master_read_data() : Waiting for acknowledge from memory" << endl;
        #endif
    }
    
    double data = 0; // in actual hardware would just read the data into a reg, no cycle cost in hw
    if_bus_master->ReadData(data);
    reg = data;
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
    if_bus_master->Request(MST_ID_HW, addr, OP_WRITE, data_len);
    
    while (!(if_bus_master->WaitForAcknowledge(MST_ID_HW))) // blocking function, hangs
    {
        wait(Clk.posedge_event());
        bus_cycles++;
        #ifdef DEBUG_HW
        debug_log_file << "[Hw_component] hw_master_write_data() : Waiting for acknowledge from memory" << endl;
        #endif
    }
    
    hardware_cycles += CYCLES_HW_ADD; // i = 0
    for (unsigned int i = 0; i < data_len; i++ )
    {
        hardware_cycles += CYCLES_HW_ADD + CYCLES_HW_CMP + CYCLES_HW_ADD; // last add cycle for accessing reg[i] 
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
        bus_cycles++;
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