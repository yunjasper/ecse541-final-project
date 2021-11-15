/**
 * Thanks to Oliver Miller for writing this golden model.
 * 
 * @author: Oliver Miller
 * 
 */

#include "systemc.h"
#include <fstream>
#include <string>
#include <iostream>

using namespace std;

#define MEMORY_SIZE (1 << (10+6))

// For homework 2
#define LOOPS 1000
#define SIZE 6
#define ADDRA 16000
#define ADDRB 16037
#define ADDRC 16074


int memory[MEMORY_SIZE];

void memoryInit(string memfile) {
    // open the memory initialization file
    ifstream memfileStream(memfile);

    string entry = "";
    unsigned int i = 0;

    //cout << "Initializing memory of size " << MEMORY_SIZE << endl;

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
    for (; i<MEMORY_SIZE; i++)
        memory[i] = 0;

} // memoryInit

void printResults(int addrA, int addrB, int addrC, int size, int loops) {
    cout << "Golden Answer" << endl;
    cout << "a matrix" << endl;

    int i, j;

    for (i = 0; i < size; i++) {
        for(j = 0; j < size; j++) {
            cout << memory[i*size + j + addrA] << "  ";
        }
        cout << endl;
    }
    cout << endl;
    cout << "b matrix" << endl;
    for (i = 0; i < size; i++) {
        for(j = 0; j < size; j++) {
            cout << memory[i*size + j + addrB] << "  ";
        }
        cout << endl;
    }
    cout << endl;
    cout << "c matrix" << endl;
    for (i = 0; i < size; i++) {
        for(j = 0; j < size; j++) {
            cout << memory[i*size + j + addrC] << "  ";
        }
        cout << endl;
    }
    cout << endl;
}

int sc_main(int argc, char* argv[]) {
    unsigned int addrC, addrA, addrB, size, loops;

    // Handle command line arguments
    if(argc >= 5) {
        addrC = stoi(argv[2]);
        addrA = stoi(argv[3]);
        addrB = stoi(argv[4]);

        if(argc == 6) {
            size = stoi(argv[5]);
            loops = LOOPS;
        } else if(argc == 7) {
            size = stoi(argv[5]);
            loops = stoi(argv[6]);
        } else {
            size = SIZE;
            loops = LOOPS;
        }
    } else {
        addrA = ADDRA;
        addrB = ADDRB;
        addrC = ADDRC;
        size = SIZE;
        loops = LOOPS;
    }

    // Initialize memory
    memoryInit(argv[1]);

    // Matrix multiplication
    // unsigned int n;
    unsigned int i,j,k;

    // doesn't matter that it doesn't do the correct multiplication for multiple loops
    // we just need the timing info

    unsigned int total_cycles = 0, total_iters = 0;
    unsigned int zero_init_i_cycles = 0, zero_init_i_iters = 0;
    unsigned int zero_init_j_cycles = 0, zero_init_j_iters = 0;
    unsigned int i_cycles = 0, i_iters = 0;
    unsigned int j_cycles = 0, j_iters = 0;
    unsigned int k_cycles = 0, k_iters = 0;

    for(unsigned int n=0; n<loops; n++) {
        total_cycles++; total_iters++;
        
        for (i=0; i<size; i++) {
            total_cycles++; zero_init_i_cycles++; zero_init_i_iters++;
            for (j=0; j<size; j++) {
                total_cycles++; zero_init_i_cycles++; zero_init_j_cycles++; zero_init_j_iters++;
                memory[i*size + j + addrC] = 0;
            }
        }

        for(i=0; i<size; i++) { // Total Cycles: 7579000, Execs: 1000, Iters: 5
            total_cycles++; i_cycles++; i_iters++;
            
            for(j=0; j<size; j++) { // Total Cycles: 7520000, Execs: 5000, Iters: 5
                total_cycles++; i_cycles++; j_cycles++; j_iters++;

                for(k=0; k<size; k++) { // Total Cycles: 7225000, Execs: 25000, Iters: 5
                    total_cycles++; i_cycles++; j_cycles++; k_cycles++; k_iters++;
                    memory[i*size + j + addrC] += memory[i*size + k + addrA] * memory[k*size + j + addrB];
                }
            }
        }
    }
    cout << "\nPerformance statistics: \nTotal loops = " << total_cycles << ", total iters = " << total_iters 
        << "\nZero-init_i loops = " << zero_init_i_cycles << ", zero-init_i iters = " << zero_init_i_iters 
        << "\nZero-init_j loops = " << zero_init_j_cycles << ", zero-init_j iters = " << zero_init_j_iters 
        << "\ni-loop loops = " << i_cycles << ", i-loop iters = " << i_iters 
        << "\nj-loop loops = " << j_cycles << ", j-loop iters = " << j_iters 
        << "\nk-loop loops = " << k_cycles << ", k-loop iters = " << k_iters << endl << endl;

    printResults(addrA, addrB, addrC, size, loops);

    return 0;
} // main