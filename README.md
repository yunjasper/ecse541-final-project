# ECSE 541 HW2

Jasper Yun (260651891) \
Fall 2021 \
McGill University

<!-- ## Table of contents

1. [Compiling](#compiling)
2. [Running](#running)
3. [File Organization](#file-organization)
4. [Memory Organization](#memory-organization)
5. [Expected Output](#expected-output) -->

---

## Compiling
Use the `makefile` to compile the project. You may need to change the `SYSTEMC_HOME` path to match your installation settings. I developed this project on Ubuntu. If you installed SystemC without specifying the C++11 standard, I do not know if there will be problems during compilation.

The following command is executed when the `make` command is entered. You may need to change the `-L$(SYSTEMC_HOME)/lib-linux64` part if you are on a different Linux distribution.

    g++ -g3 -Wall -Wextra -std=c++11 -I. -I$(SYSTEMC_HOME)/include -L. -L$(SYSTEMC_HOME)/lib-linux64 $(FILES) -o main -lsystemc -lm


## Running
To run the project, the usage is shown below;  `<filename>` is a required argument, `[arg]` is optional.

    ./mm <memoryInitFile> [[[addrC addrA addrB] size] loops]

Examples:

    ./mm mem_test.txt
    ./mm mem_test.txt 10000 0 5000 6 10

---

## File Organization

There are 3 parts to this assignment as detailed below.
* `Part1`: hardware coprocessor multiplies a row of `A` with a column of `B`. 
* `Part2`: hardware multiplies a row of `A` with the matrix `B`.
* `Software-Only`: pure software implementation (with bus) of matrix multiplication.

The following table describes the files in each part. Note that the `Software-Only` part does not have hardware files.

Filename    |   Description
---------   |   ------------
`bus_master_if.h` | declaration of bus master interface
`bus_minion_if.h` | declaration of bus minion interface
`hw_component.cpp`| implementation of hardware coprocessor
`hw_component.h`  | declaration of hardware coprocessor
`main.cpp`  |   main function which sets up and runs the simulation.
`main.h`    |   `#define` statements for default values
`memory.h`    | declaration and implementation for `Memory`
`oscillator.h`| declaration and implementation of the clock signal oscillator
`shared_bus.h`    | declaration and implementation of the shared bus
`sw_component.cpp` | implementation of software component
`sw_component.h`      | declaration of software component
`log_part1.txt` | output log file for `Part1`
`log_part2.txt` | output log file for `Part2`
`log_sw_only.txt` | output log file for `Software-Only`

---

## Memory Organization
Memory intialization uses the specified input text file. All data is assumed to be in row-major format in the text file. Also, the input addresses are checked based on the input size and the input number of loops. Errors are printed on the terminal if the addresses specified are not valid for the size and loops passed and the program terminates.

---

## Expected Output

Simulation progress is printed to the terminal to display the input parameters, loops, and final simulation time. The results C matrices of each loop from the calculations are saved to a text file in the same folder as the executable. See below for an example output on the terminal. Expressions printed to the terminal and text file begin with `[name]` to indicate the file that produced the output.

    jyun6@ubuntu:~/Desktop/ECSE541/HW2/Software-Only$ ./mm mem_test.txt 10000 0 5000 6 3

            SystemC 2.3.3-Accellera --- Sep 30 2021 08:16:06
            Copyright (c) 1996-2018 by all Contributors,
            ALL RIGHTS RESERVED

    ECSE 541: MPSoC Design, HW2 -- Software Only
    Software-Hardware Partitioned Matrix-Matrix Multiplication
    addrA = 0, addrB = 5000, addrC = 10000
    matrix_size = 6, loops = 3, mem_size = 10110
    [Memory] Input file too short! Initializing rest of memory to zero
    [main] Starting simulation! 
    [Sw_component] loop = 1
    [Sw_component] loop = 2
    [Sw_component] loop = 3

    Info: /OSCI/SystemC: Simulation stopped by user.
    [main] end of simulation
    [main] final simulation time: 365746 ns
    [main] total cycles: time / clock period = 54834.5
    [main] final memory contents at addrC of each loop printed to log_sw_only.txt.
    [main] software total cycles counted = 56133


The associated output file for the above example is shown below.

    ECSE 541: MPSoC Design, HW2 -- Software Only
    Software-Hardware Partitioned Matrix-Matrix Multiplication
    addrA = 0, addrB = 5000, addrC = 10000
    matrix_size = 6, loops = 3, mem_size = 10110
    [Memory] Input file too short! Initializing rest of memory to zero
    [main] Starting simulation! 
    [main] end of simulation
    [main] final simulation time: 365746 ns
    [main] total cycles: time / clock period = 54834.5
    [main] software total cycles counted = 56133

    C matrix for loop = 0
    17347 19198 18811 15058 18671 28862 9531 13118 10351 8200 12711 17968 18324 22752 19500 16346 13780 23222 13921 14168 11076 13802 13439 19041 9838 15418 12267 6948 10598 17456 10080 9198 4900 12396 8983 11096 

    C matrix for loop = 1
    14124 12736 6035 10325 11372 8360 18302 13832 14439 10729 18558 14485 21352 16948 17378 13020 20225 16015 14359 10827 5721 9288 12599 9511 17606 9398 8263 9803 10950 9598 18707 14723 13063 11652 16385 13814 

    C matrix for loop = 2
    16363 20167 19594 11327 17845 20465 19922 29157 29569 21462 27650 29417 11466 15941 18669 16992 16086 18385 14500 22566 19070 12711 19526 18530 11178 22401 17387 19334 19110 20939 8656 12300 13537 10291 12345 13913 
