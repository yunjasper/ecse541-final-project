#include "systemc.h"
#include <fstream>
#include <string>
#include <queue>
#include "system.h"

// #define waveforms

class memory_component;
class oscillator;

using namespace std;

// initialize values for input and output addresses
unsigned int a_addr = A_ADDR;
unsigned int b_addr = B_ADDR;
unsigned int c_addr = C_ADDR;
unsigned int size = SIZE;
unsigned int loops = LOOPS;

unsigned int mult_cycles = 6;   // cycles needed for multiplication
unsigned int add_cycles = 1;    // cycles needed for addition
unsigned int asgn_cycles = 1;   // cycles needed for assignment 
unsigned int comp_cycles = 1;   // cycles needed for comparison

unsigned int cycle_count = 0;

// Bus Master Interface
class bus_master_if : virtual public sc_interface
{
  public:
    virtual void Request(unsigned int mst_id, unsigned int addr, unsigned int op, unsigned int len) = 0;
    virtual bool WaitForAcknowledge(unsigned int mst_id) = 0;
    virtual void ReadData(unsigned int &data) = 0;
    virtual void WriteData(unsigned int data) = 0;
};

// Bus Minion Interface 
class bus_minion_if : virtual public sc_interface
{
  public:
    virtual void Listen(unsigned int &req_addr, unsigned int &req_op, unsigned int &req_len) = 0;
    virtual void Acknowledge() = 0; 
    virtual void SendReadData(unsigned int data) = 0;
    virtual void ReceiveWriteData(unsigned int &data) = 0;
};

// bus component
class bus_component: public sc_module, public bus_master_if, public bus_minion_if{
    struct bus_state{
        unsigned int mst_id;
        unsigned int addr;
        unsigned int op;
        unsigned int len;
    };
    
    public:
        sc_in<sc_logic> Clk;
        queue<unsigned int> dataQueue;
        bus_state currentInControl;
        bus_state requestForControl;
        unsigned int currentLenSent = 0;
        unsigned int currentLenReceived = 0;
        bool acknowledged;
        
        SC_HAS_PROCESS(bus_component);
        bus_component(sc_module_name name): sc_module(name){
            sensitive << Clk.pos();
            SC_THREAD(busArbitration);

            currentInControl.mst_id = INIT_BUS_STATE;
            currentInControl.addr = INIT_BUS_STATE;
            currentInControl.op = INIT_BUS_STATE;
            currentInControl.len = INIT_BUS_STATE;

            requestForControl.mst_id = INIT_BUS_STATE;
            requestForControl.addr = INIT_BUS_STATE;
            requestForControl.op = INIT_BUS_STATE;
            requestForControl.len = INIT_BUS_STATE;

        } // bus_component (Done)

    // BUS ARBITRATION
    void busArbitration(){
        while(true){
            wait(Clk.posedge_event());
            if (currentLenSent == currentInControl.len || currentLenReceived == currentInControl.len){
                cout << "[BUS]\tCurrent Master Finished: " << currentInControl.mst_id << ", " << currentInControl.addr << ", " << currentInControl.op << ", " << currentInControl.len << endl;
                currentInControl.mst_id = INIT_BUS_STATE;
                currentInControl.addr = INIT_BUS_STATE;
                currentInControl.op = INIT_BUS_STATE;
                currentInControl.len = INIT_BUS_STATE;

                currentLenSent = 0;
                currentLenReceived = 0;
            }
            if(currentInControl.mst_id == INIT_BUS_STATE && requestForControl.mst_id != INIT_BUS_STATE){
                cout << "[BUS]\tChanging Masters, new Request: " << requestForControl.mst_id << ", " << requestForControl.addr << ", " << requestForControl.op << ", " << requestForControl.len << endl;
                currentInControl.mst_id = requestForControl.mst_id;
                currentInControl.addr = requestForControl.addr;
                currentInControl.op = requestForControl.op;
                currentInControl.len = requestForControl.len;

                requestForControl.mst_id = INIT_BUS_STATE;
                requestForControl.addr = INIT_BUS_STATE;
                requestForControl.op = INIT_BUS_STATE;
                requestForControl.len = INIT_BUS_STATE;
            }
        }
    } //busArbitration (Done)

    // MASTER FUNCTIONS
    void Request(unsigned int mst_id, unsigned int addr, unsigned int op, unsigned int len){
        // Request requires two clock cycles
        wait(Clk.posedge_event());
        cycle_count++;
        unsigned int req_mst_id = mst_id;
        unsigned int req_addr = addr;
        unsigned int req_op = op;
        unsigned int req_len = len;

        wait(Clk.posedge_event());
        cycle_count++;
        requestForControl.mst_id = req_mst_id;
        requestForControl.addr = req_addr;
        requestForControl.op = req_op;
        requestForControl.len = req_len;
        cout << "[BUS]\tNew Request: " << requestForControl.mst_id << ", " << requestForControl.addr << ", " << requestForControl.op << ", " << requestForControl.len << endl;

    } // Request (DONE)

    bool WaitForAcknowledge(unsigned int mst_id){
        // cout << "[BUS]\tWaitForAck currentInControl.mst_id = " << currentInControl.mst_id << " passed in mst_id = " << mst_id << " acknowledged = " << acknowledged << endl;
        if(currentInControl.mst_id != mst_id || !acknowledged){
            return false;
        }
        else{
            acknowledged = false;
            return true;
        }
    } // WaitForAcknowledge (Done)

    void ReadData(unsigned int &data){
        // Read requires two clock cycles
        wait(Clk.posedge_event());
        wait(Clk.posedge_event());
        if(currentInControl.mst_id == SW_ID) cycle_count += 2;

        cout << "[BUS]\tReadData currentInControl " << currentInControl.mst_id << ", " << currentInControl.addr << ", " << currentInControl.op << ", " << currentInControl.len << endl;
        if(currentLenReceived < currentInControl.len){
            while(dataQueue.empty()){
                wait(Clk.posedge_event());
                if(currentInControl.mst_id == SW_ID) cycle_count ++;
            }
            unsigned int readData = dataQueue.front();
            data = readData; // Set data
            dataQueue.pop();
            currentLenReceived += 1;
            cout << "[BUS]\tReadData: data = " << data << endl;
        }
    } // ReadData (Done)

    void WriteData(unsigned int data){
        // Write requires one clock cycles
        wait(Clk.posedge_event());
        if(currentInControl.mst_id == SW_ID) cycle_count++;
        if(currentLenSent < currentInControl.len){
            dataQueue.push(data);
            cout << "[BUS]\tWriteData success: data = " << data << endl;
            currentLenSent += 1;
        }
    } //WriteData (Done)

    // MINION FUNCTIONS
    void Listen(unsigned int &req_addr, unsigned int &req_op, unsigned int &req_len){  //This is a READ operation
        // cout << "[BUS] \t Minion Listening..." << endl;
        wait(Clk.posedge_event());
        wait(Clk.posedge_event());

        unsigned int current_addr = currentInControl.addr;
        unsigned int current_op = currentInControl.op;
        unsigned int current_len = currentInControl.len;

        req_addr = current_addr;
        req_op = current_op;
        req_len = current_len;
    } // Listen (Done)
    
    void Acknowledge(){
        // Acknowledge requires one clock cycles
        wait(Clk.posedge_event());
        while(acknowledged == true){
            wait(Clk.posedge_event());
        }
        acknowledged = true;

    } // Acknowledge (Done)

    void SendReadData(unsigned int data){
        wait(Clk.posedge_event());
        wait(Clk.posedge_event());
        dataQueue.push(data);
        cout << "[BUS]\tSendReadData is sending: data = " << data << endl; 
    } // SendReadData (Done)

    void ReceiveWriteData(unsigned int &data){
        wait(Clk.posedge_event());
        cout << "[BUS]\tReceivedWriteData (minion reading)" << endl;
        while(dataQueue.empty()){
            wait(Clk.posedge_event());
        }
        data = dataQueue.front(); // Set data
        cout << "[BUS]\tReceiveWriteData receiving data = " << data << endl;
        dataQueue.pop();
    } // ReceiveWriteData (Done)
};

// implements a clock signal
class oscillator: public sc_module{
    public:
        sc_out<sc_logic> clk;
        SC_HAS_PROCESS(oscillator);
        
        oscillator(sc_module_name name): sc_module(name){
            SC_THREAD(oscillate);
        }
    
    // while oscillate is running, change clk
    void oscillate(){
        while(true) {
            clk.write(SC_LOGIC_0);
            wait(CLK_CYCLE/2, SC_NS);
            clk.write(SC_LOGIC_1);
            wait(CLK_CYCLE/2, SC_NS);
            // cout << "[CLK]\tPeriod Elapsed" << endl;
        }
    }
};

// memory_component implements a model of memory
class memory_component: public sc_module {
    public: 
            sc_in<sc_logic> Clk;
            sc_port<bus_minion_if> memoryBusInterface;

            unsigned int memory[MEMORY_SIZE] = {0};
            // TODO FIX size

            unsigned int req_addr, req_op, req_len;

            SC_HAS_PROCESS(memory_component);
            memory_component(string memfile, sc_module_name name){
                // write memory from text file
                // open the memory initialization file
                ifstream memfileStream(memfile);
                string entry = "";
                int i = 0;
                cout << "Initializing memory of size " << MEMORY_SIZE << endl;

                // iterate until we run out of memory or the file ends
                for (; i<MEMORY_SIZE; i++) {
                    // get a token, delimited by a space
                    if (getline(memfileStream, entry, ' ')) {
                        // turn the token into an int and save it
                        memory[i] = stoi(entry);
                    }
                    else {
                        // leave the loop when the file ends
                        break;
                    }
                } // for

                // if the init file isn't big enough to initialize all memory locations,
                // fill the rest with zeroes
                for (; i<MEMORY_SIZE; i++){
                    memory[i] = 0;
                }

                // MAKE B INTO COLUMN MAJOR
                int b[size*size] = {0};
                for(int offset = b_addr; offset < c_addr; offset += size*size){
                    for(int row = 0; row < size; row++){
                        for(int col = 0; col < size; col++){
                            b[row*size+col] = memory[offset+col*size+row];
                        }
                    }
                    for(int row = 0; row < size; row++){
                        for(int col = 0; col < size; col++){
                            memory[offset + row*size+col] = b[row*size+col];
                        }
                    }
                }                
                SC_THREAD(runMemory);

            }


    void runMemory(){
        while(true){
            memoryBusInterface->Listen(req_addr, req_op, req_len);
            if(req_op == OP_READ){
                memoryBusInterface->Acknowledge();
                cout << "[MEM]\tMem sending length: " << req_len << endl;
                for(int i = 0; i < req_len; i++){
                    cout << "[MEM]\tSendReadData Input: " << memory[req_addr + i] << endl ;
                    memoryBusInterface->SendReadData(memory[req_addr + i]);
                }
            }
            else if(req_op == OP_WRITE){
                memoryBusInterface->Acknowledge();
                cout << "[MEM]\tMem writing length " << req_len << endl;
                cout << "[MEM]\tAcknowledged" << endl;
                for(int i = 0; i < req_len; i++){
                    unsigned int data;
                    memoryBusInterface->ReceiveWriteData(data);
                    memory[req_addr + i] = data;
                }
            }
            // wait(Clk.posedge_event());  // I am scared to remove this
        }
    }

    // prints contents of memory
    void memoryPrint(unsigned int start, unsigned int end) {
        // ensure end is valid
        if (end >= MEMORY_SIZE) {
            end = MEMORY_SIZE-1;
            cerr << "*** ERROR in memoryPrint: end > MEMORY_SIZE-1" << endl; 
        }

        // print the requested values
        for (unsigned int i=start; i<end; i++) {
            cout << memory[i] << " ";
        }

        cout << endl;
    } // memoryPrint
};

// software component
class software_component: public sc_module{
    public:
    sc_in<sc_logic> Clk;
    sc_port<bus_master_if> softwareBusInterface;
    SC_HAS_PROCESS(software_component);

        software_component(sc_module_name name): sc_module(name){
            SC_THREAD(runSoftware);
            sensitive << Clk.pos();
        }
    
    void runSoftware(){
        wait(1*asgn_cycles*CLK_CYCLE); // for n = 0
        cycle_count += asgn_cycles;
        for(int n = 0; n < loops; n++){ //LOOPS loop
            wait(1*comp_cycles);  //for n < loops
            cycle_count += 1*comp_cycles;  
            wait(1*add_cycles*CLK_CYCLE);  // for n++ 
            cycle_count += 1*add_cycles;  

            // WRITE C TO ALL 0s INITIALLY
            cout << "[BUS]\tRequest Initiated (write, set to 0)" << endl;

            wait(3*mult_cycles*CLK_CYCLE); // 3 multiplications for request
            cycle_count += 3*mult_cycles;
            wait(1*add_cycles*CLK_CYCLE); // 1 add for request
            cycle_count += 1*add_cycles;
            softwareBusInterface->Request(SW_ID, size*size*n + c_addr, OP_WRITE, size*size);
            while(!(softwareBusInterface->WaitForAcknowledge(SW_ID))){
                wait(Clk.posedge_event());
                cycle_count++;
                
            }
            wait(1*asgn_cycles); // for i = 0
            cycle_count++;
            for(unsigned int i = 0; i < size*size; i++){
                wait(1*comp_cycles*CLK_CYCLE);  // for every i < size*size
                cycle_count += 1*comp_cycles;
                wait(1*add_cycles*CLK_CYCLE);  // for every i++
                cycle_count += 1*add_cycles;
                wait(1*mult_cycles*CLK_CYCLE);  // for every size*size
                cycle_count += 1*mult_cycles;

                softwareBusInterface->WriteData(0);
            }

            wait(1*asgn_cycles*CLK_CYCLE);  // for i = 0
            cycle_count += 1*asgn_cycles;
            for(unsigned int i = 0; i < size; i++){
                wait(1*add_cycles*CLK_CYCLE);  // for i++
                cycle_count += 1*add_cycles;
                wait(1*comp_cycles*CLK_CYCLE);  // for i < size
                cycle_count += 1*comp_cycles;

                cout << "[SW]\tRequest Calc " << i*size << endl;
                softwareBusInterface->Request(SW_ID, INIT_BUS_STATE, OP_CALC, 1);
                while(!(softwareBusInterface->WaitForAcknowledge(SW_ID))){
                    wait(Clk.posedge_event());
                    cycle_count++;
                }

                wait(1*asgn_cycles);  // for data = 0
                cycle_count += 1*asgn_cycles;
                unsigned int data = 0;
                softwareBusInterface->ReadData(data); // dummy data

                cout << "[SW]\tCalc " << i*size << " acknowledged" << endl;
                wait(50 * CLK_CYCLE, SC_NS);  //
            }
        } // end LOOPS loop
        sc_stop();
    }
};

// TODO ADD LOOPS
// hardware component
class hardware_component: public sc_module{
    public:
        sc_in<sc_logic> Clk;
        sc_port<bus_master_if> masterHardwareBusInterface;
        sc_port<bus_minion_if> minionHardwareBusInterface;
        SC_HAS_PROCESS(hardware_component);
        unsigned int req_addr, req_op, req_len;

        hardware_component(sc_module_name name): sc_module(name){
            SC_THREAD(runHardware);
            sensitive << Clk.pos();
        }
    
    void runHardware(){
        unsigned int currentCol = 0;
        unsigned int currentRow = 0;
        unsigned int currentLoop = 0;
        while(true){
            minionHardwareBusInterface->Listen(req_addr, req_op, req_len);
            if(req_op == OP_CALC){
                minionHardwareBusInterface->Acknowledge();
                
                minionHardwareBusInterface->SendReadData(0);

                unsigned int row_a[size] = {0};
                unsigned int b[size*size] = {0};
                unsigned int c[size] = {0};

                // fill row_a with the first "size" elements from A_ADDR (row major)
                cout << "[HW]\tRequesting A" << endl;
                masterHardwareBusInterface->Request(HW_ID, size*size*currentLoop + a_addr + currentRow*size, OP_READ, size);
                while(!(masterHardwareBusInterface->WaitForAcknowledge(HW_ID))){
                    wait(Clk.posedge_event());
                }
                cout << "[HW]\tRequesting A acknowledged" << endl;
                for(unsigned int i = 0; i < size; i++){
                    masterHardwareBusInterface->ReadData(row_a[i]);
                    // cout << "[HW]\tIndex: " << i << endl;
                }

                // fill row_a with the first "size" elements from B_ADDR (col major)
                cout << "[HW]\tRequesting B" << endl;
                masterHardwareBusInterface->Request(HW_ID, size*size*currentLoop + b_addr + currentCol*size, OP_READ, size*size);
                while(!masterHardwareBusInterface->WaitForAcknowledge(HW_ID)){
                    wait(Clk.posedge_event());
                }
                cout << "[HW]\tRequesting B acknowledged" << endl;
                for(unsigned int i = 0; i < size*size; i++){
                    masterHardwareBusInterface->ReadData(b[i]);
                }
                
                for(unsigned int i = 0; i < size; i++){
                    for(unsigned int j = 0; j < size; j++){
                        c[i] += row_a[j] * b[i*size + j];
                    }
                }
                cout << c;

                // fill element of c with result
                masterHardwareBusInterface->Request(HW_ID, size*size*currentLoop + c_addr + currentRow*size + currentCol, OP_WRITE, size);
                while(!masterHardwareBusInterface->WaitForAcknowledge(HW_ID)){
                    wait(Clk.posedge_event());
                }
                for(unsigned int i = 0; i < size; i++){
                    masterHardwareBusInterface->WriteData(c[i]);
                }

                currentRow++;
                if(currentRow == size){
                    currentLoop++;
                    currentRow = 0;
                }
            }

        }
    }
};

int sc_main(int argc, char* argv[]) {
    // next, check usage
    if (argc != 2 && argc != 5 && argc != 6 && argc != 7) {
        cerr << "Usage: " << argv[0] << "<memoryInitFile> [[[addrC addrA addrB] size] loops]" << endl;
        return 0;
    } // if

    // set custom addresses
    if (argc >= 5) {
        c_addr = stoi(argv[2]);
        a_addr = stoi(argv[3]);
        b_addr = stoi(argv[4]);

        if (argc >= 6)
            size = stoi(argv[5]);
            if (argc == 7){
                loops = stoi(argv[6]);
            }
    } // if

    // TEST INPUTS
    cout << "c_addr: " << c_addr << endl;
    cout << "a_addr: " << a_addr << endl;
    cout << "b_addr: " << b_addr << endl;
    cout << "size: " << size << endl;
    cout << "loops: " << loops << endl;

    memory_component *mem = new memory_component(argv[1], "Memory");
    bus_component *bus = new bus_component("Bus");
    software_component *sw = new software_component("SW");
    hardware_component *hw = new hardware_component("HW");
    oscillator *osc = new oscillator("Oscillator");

    sc_signal<sc_logic> CLK;

    osc->clk(CLK);
    mem->Clk(CLK);
    bus->Clk(CLK);
    sw->Clk(CLK);
    hw->Clk(CLK);

    sw->softwareBusInterface(*bus);
    hw->masterHardwareBusInterface(*bus);
    hw->minionHardwareBusInterface(*bus);
    mem->memoryBusInterface(*bus);

    cout << "initial a matrix:" << endl;
    for(int i = 0; i < size; i++){
        for(int j = 0; j < size; j++){
            cout << (mem->memory)[a_addr + size*i + j] << "\t";
        }
        cout << endl;
    }
    cout << endl;

    sc_start();

    
    int loop_to_print = 5;

    cout << "final a matrix:" << endl;
    for(int i = 0; i < size; i++){
        for(int j = 0; j < size; j++){
            cout << (mem->memory)[size*size*loop_to_print + a_addr + size*i + j] << "\t";
        }
        cout << endl;
    }
    cout << endl;

    cout << "final b matrix:" << endl;
    for(int i = 0; i < size; i++){
        for(int j = 0; j < size; j++){
            cout << (mem->memory)[size*size*loop_to_print + b_addr + size*j + i] << "\t";
        }
        cout << endl;
    }
    cout << endl;

    cout << "final c matrix:" << endl;
    for(int i = 0; i < size; i++){
        for(int j = 0; j < size; j++){
            cout << (mem->memory)[size*size*loop_to_print + c_addr + size*i + j] << "\t";
        }
        cout << endl;
    }
    cout << endl;

    cout << "Software Cycles: " << cycle_count << endl;

    return 0;
} // main
