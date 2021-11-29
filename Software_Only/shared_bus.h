/**
 * ------------
 * shared_bus.h
 * ------------
 * 
 * Implementation of the internal 
 * state of the shared bus.
 * 
 * Author: Jasper Yun (260651891)
 * 
 * ECSE 541: Design of MPSoC
 * Fall 2021
 * McGill University
 * 
 */

#include "systemc.h"
#include "bus_master_if.h"
#include "bus_minion_if.h"
#include "main.h"
#include <queue>

using namespace std;

extern volatile unsigned int bus_cycles;

// to keep track of the state of the bus
// struct holds info about the requested transaction
struct request
{
    unsigned int mst_id;
    unsigned int addr;
    unsigned int op;
    unsigned int len;
};

extern ofstream debug_log_file;
extern volatile unsigned int software_cycles;

// bus itself is only a "highway" for data to go onto/out of to different 
// memory addresses. so need to keep track of who the current master device
// on the bus is, and the master device id of the current request (which may
// not be the same as the current master)
class Shared_bus : public sc_module, public bus_master_if, public bus_minion_if
{
    public:

        request current_request; // current request
        request current_master;  // actual master device in control of the bus
        
        // can probably make all these private
        bool is_acknowledged = false; // request has been acknowledged
        unsigned int len_data_sent; // for ReadData()
        unsigned int len_data_received; // for WriteData()
        
        sc_in<sc_logic> Clk; // clock signal input
        queue<double> data_queue; // stores data from minions/masters

        // constructor
        SC_HAS_PROCESS(Shared_bus);
        Shared_bus(sc_module_name name) : sc_module(name)
        {
            sensitive << Clk.pos(); // make sensitive to clock positive edges
            SC_THREAD(do_shared_bus); // method that runs forever

            // set default bus state
            current_master.mst_id = BUS_IDLE_ID;
            current_master.addr = ADDR_BUS_IDLE;
            current_master.op = OP_NONE;
            current_master.len = LEN_BUS_IDLE;

            current_request.mst_id = BUS_IDLE_ID;
            current_request.addr = ADDR_BUS_IDLE;
            current_request.op = OP_NONE;
            current_request.len = LEN_BUS_IDLE;
        }
    
        void do_shared_bus()
        {
            while (true)
            {
                // whatever the bus does always once running
                wait(Clk.posedge_event());
                #ifdef DEBUG_BUS
                cout << "[Shared_bus] do_shared_bus() : Wait for next clock positive edge" << endl;
                debug_log_file << "[Shared_bus] do_shared_bus() : Wait for next clock positive edge" << endl;
                debug_log_file << "[Shared_bus] do_shared_bus() : current_master.mst_id = " << current_master.mst_id << ", current_master.addr = " << current_master.addr << ", current_master.len = " << current_master.len << ", op = " << current_master.op << endl;
                #endif

                // default state of bus: mst_id = 0U, addr = 0x99999999, op = OP_NONE, len = 0U
                // 2 cases: 
                //      1. bus is idle with pending request
                //      2. bus just finished a transaction
                // other cases = bus in use and arbitration is complete, so this method does not intervene
                if (current_master.mst_id == BUS_IDLE_ID && current_request.mst_id != BUS_IDLE_ID)
                {
                    #ifdef DEBUG_BUS
                    cout << "[Shared_bus] do_shared_bus() : Giving control to new request: MST_ID = " 
                        << current_request.mst_id << ", ADDR = " << current_request.addr 
                        << ", OP = " << current_request.op 
                        << ", LEN = " << current_request.len << endl;
                    
                    debug_log_file << "[Shared_bus] do_shared_bus() : Giving control to new request: MST_ID = " 
                        << current_request.mst_id << ", ADDR = " << current_request.addr 
                        << ", OP = " << current_request.op 
                        << ", LEN = " << current_request.len << endl;
                    #endif

                    // give control to request
                    current_master.mst_id = current_request.mst_id;
                    current_master.addr = current_request.addr;
                    current_master.op = current_request.op;
                    current_master.len = current_request.len;

                    // setup counters for transactions
                    len_data_received = 0;
                    len_data_sent = 0;

                    // clear request
                    current_request.mst_id = BUS_IDLE_ID;
                    current_request.addr = ADDR_BUS_IDLE;
                    current_request.op = OP_NONE;
                    current_request.len = LEN_BUS_IDLE;
                }
            
                // check if transaction for current bus master is finished
                else if (current_master.len == len_data_received || current_master.len == len_data_sent)
                {
                    // transaction done, set current master to idle state
                    current_master.mst_id = BUS_IDLE_ID;
                    current_master.addr = ADDR_BUS_IDLE;
                    current_master.op = OP_NONE;
                    current_master.len = LEN_BUS_IDLE;
                    
                    len_data_received = 0;
                    len_data_sent = 0;

                    // debug info
                    #ifdef DEBUG_BUS
                    cout << "[Shared_bus] do_shared_bus() : Entered idle state, end of function" << endl;
                    debug_log_file << "[Shared_bus] do_shared_bus() : Entered idle state, end of function" << endl;
                    #endif
                }

                // debug info
                #ifdef DEBUG_BUS
                cout << "[Shared_bus] do_shared_bus() : end of function" << endl;
                debug_log_file << "[Shared_bus] do_shared_bus() : end of function -- ";
                debug_log_file << "Current master: MST_ID = " << current_master.mst_id 
                    << ", ADDR = " << current_master.addr 
                    << ", OP = " << current_master.op 
                    << ", LEN = " << current_master.len << endl;
                #endif
            }
        }

        // --- master ---
        /**
         * Bus master requests on bus for minion device to do its bidding.
         * 
         * @param mst_id ID of master device
         * @param addr address of memory to read from/write to
         * @param op ID of minion device
         * @param len length of data to be read/written
         * @return void
         */
        void Request(unsigned int mst_id, unsigned int addr, unsigned int op, unsigned int len)
        {
            #ifdef DEBUG_BUS
            cout << "[Shared_bus] Request() : Wait for two clock periods" << endl;
            debug_log_file << "[Shared_bus] Request() : Wait for two clock periods" << endl;
            #endif

            wait(Clk.posedge_event());
            #ifdef DEBUG_BUS
            debug_log_file << "[Shared_bus] Request() : first positive clock edge" << endl;
            #endif
            wait(Clk.posedge_event());
            bus_cycles += 2;
            #ifdef DEBUG_BUS
            debug_log_file << "[Shared_bus] Request() : second positive clock edge" << endl;
            #endif

            #ifdef DEBUG_BUS
            cout << "[Shared_bus] Request() : set current_request struct parameters" << endl;
            debug_log_file << "[Shared_bus] Request() : set current_request struct parameters" << endl;
            #endif
            // TODO: do we need error checking for valid requests? e.g. mst_id valid, op valid?
            // set current request to the details of the new request
            current_request.mst_id = mst_id;
            current_request.addr = addr;
            current_request.op = op;
            current_request.len = len;

            // debug info
            #ifdef DEBUG_BUS
            cout << "[Shared_bus] Request() : end of function" << endl;
            debug_log_file << "[Shared_bus] Request() : end of function" << endl;
            #endif
            
        }

        /**
         * Bus master waits for acknowledge signal from bus.
         * This function is blocking -- waits until acknowledge is received.
         * 
         * @param mst_id ID of master device
         * @return boolean
         */
        bool WaitForAcknowledge(unsigned int mst_id)
        {
            // two possibilities: 
            //      - bus is servicing another request (check mst_id)
            //      - bus is servicing this request, but minion has not acknowledged yet
            if (current_master.mst_id != mst_id || !is_acknowledged)
            {
                return false;
            }
            else 
            {
                is_acknowledged = false; // reset
                return true;
            }
        }

        /**
         * Once master device has access to bus, read data
         * off of the bus.
         * 
         * @param data pointer to data
         * @return void
         */
        void ReadData(double &data)
        {
            wait(Clk.posedge_event()); // wait 2 full clock cycles
            wait(Clk.posedge_event());
            bus_cycles += 2;
            
            // take data from minion and write it to the provided address
            if (len_data_sent < current_master.len) // from minion POV it's sent data
            {
                // need to check if queue is empty first!
                while (data_queue.empty())
                {
                    wait(Clk.posedge_event());
                    bus_cycles++;
                }
                
                // pop values from FIFO queue and put into &data
                data = data_queue.front(); // get value at front of queue
                data_queue.pop(); // remove value (returns void)
                #ifdef DEBUG_BUS 
                debug_log_file << "[Shared_bus] master ReadData() : current_master.len = " << current_master.len << ", len_data_sent = " << len_data_sent << ", read data = " << data << ", data_queue length = " << data_queue.size() << endl;
                #endif
                len_data_sent++;
            }

            #ifdef DEBUG_MEM
            if (len_data_sent == current_master.len)
            {
                cout << "[Shared_bus] ReadData() : final transaction completed" << endl;
                debug_log_file << "[Shared_bus] ReadData() : final transaction completed" << endl;
            }
            #endif
        }

        /**
         * Once master device has access to bus, write data
         * onto the bus.
         * 
         * @param data data to write
         * @return void
         */
        void WriteData(double data)
        {
            wait(Clk.posedge_event()); // wait 1 full clock cycle
            bus_cycles++;

            // take data from master and put into FIFO queue
            if (len_data_received < current_master.len) // from minion POV
            {
                data_queue.push(data);
                len_data_received++;
                #ifdef DEBUG_BUS 
                debug_log_file << "[Shared_bus] master WriteData() : current_master.len = " << current_master.len << ", len_data_received = " << len_data_received << ", wrote data = " << data << ", data_queue length = " << data_queue.size() << endl;
                #endif
            }

            #ifdef DEBUG_MEM
            if (len_data_received == current_master.len)
            {
                cout << "[Shared_bus] WriteData() : final transaction completed" << endl;
                debug_log_file << "[Shared_bus] WriteData() : final transaction completed" << endl;
            }
            #endif
        }

        // --- minion ---

        /**
         * Bus minion listens to the bus by getting the current state.
         */
        void Listen(unsigned int &req_addr, unsigned int &req_op, unsigned int &req_len)
        {
            wait(Clk.posedge_event()); // wait 1 full clock cycle
            bus_cycles++;
            // TODO: Listen() is a read instruction so need another clock wait?
            
            // get current state of bus
            req_addr = current_master.addr;
            req_op = current_master.op;
            req_len = current_master.len;

            #ifdef DEBUG_BUS
            cout << "[Shared_bus] Listen() : listening to bus master parameters: mst_id = " << current_master.mst_id << ", req_addr = " << current_master.addr << ", req_op = " << current_master.op << ", req_len = " << current_master.len;
            if (current_master.addr == ADDR_BUS_IDLE) cout << "  (bus idle)";
            // cout << "[Shared_bus] Listen() : listening to bus request parameters: req_addr = " << current_request.addr << ", req_op = " << current_request.op << ", req_len = " << current_request.len << endl; 
            cout << "[Shared_bus] Listen() : total simulation time = " << sc_time_stamp().to_seconds()*1e9 << "ns" << endl;
            debug_log_file << "[Shared_bus] Listen() : listening to bus master parameters: mst_id = " << current_master.mst_id << ", req_addr = " << current_master.addr << ", req_op = " << current_master.op << ", req_len = " << current_master.len; 
            if (current_master.addr == ADDR_BUS_IDLE) debug_log_file << "  (bus idle)";

            cout << endl; debug_log_file << endl;
            // debug_log_file << "[Shared_bus] Listen() : listening to bus request parameters: req_addr = " << current_request.addr << ", req_op = " << current_request.op << ", req_len = " << current_request.len << endl; 
            #endif
        }

        /**
         * When req_op matches minion id, acknowledge and do stuff.
         */
        void Acknowledge()
        {
            wait(Clk.posedge_event()); // wait 1 full clock cycle
            bus_cycles++;
            is_acknowledged = true;
        }

        /**
         * Puts data into FIFO queue to be sent back to master.
         */
        void SendReadData(double data)
        {
            wait(Clk.posedge_event()); // wait 2 full clock cycles
            wait(Clk.posedge_event());
            bus_cycles += 2;
            if (len_data_sent < current_master.len)
            {
                data_queue.push(data);
            }

            #ifdef DEBUG_BUS 
            debug_log_file << "[Shared_bus] minion SendReadData() : current_master.len = " << current_master.len << ", len_data_sent = " << len_data_sent << ", sent read data = " << data << ", data_queue length = " << data_queue.size() << endl;
            #endif
        }

        /**
         * Gets data from FIFO queue from master device to be sent to minion.
         */
        void ReceiveWriteData(double &data)
        {
            wait(Clk.posedge_event()); // wait 1 full clock cycle
            bus_cycles++;
            
            // check if queue is empty
            while (data_queue.empty())
            {
                bus_cycles++;
                wait(Clk.posedge_event());
            }
            // get data
            data = data_queue.front();
            data_queue.pop();

            #ifdef DEBUG_BUS 
            debug_log_file << "[Shared_bus] mminion ReceiveWriteData() : received write data = " << data << ", data_queue length = " << data_queue.size() << endl;
            #endif
        }
};