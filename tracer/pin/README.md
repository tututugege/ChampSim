# Intel PIN tracer

The included PIN tool `champsim_tracer.cpp` can be used to generate new traces.
It has been tested (April 2022) using PIN 3.22.

## Download and install PIN

Download the source of PIN from Intel's website, then build it in a location of your choice.

    wget https://software.intel.com/sites/landingpage/pintool/downloads/pin-3.22-98547-g7a303a835-gcc-linux.tar.gz
    tar zxf pin-3.22-98547-g7a303a835-gcc-linux.tar.gz
    cd pin-3.22-98547-g7a303a835-gcc-linux/source/tools
    make
    export PIN_ROOT=/your/path/to/pin

## Building the tracer

The provided makefile will generate `obj-intel64/champsim_tracer.so`.

    make
    $PIN_ROOT/pin -t obj-intel64/champsim_tracer.so -- <your program here>

The tracer has four options you can set:
```
-o
Specify the output file for your trace.
The default is default_trace.champsim

-op
Specify the output file for an optional op sidecar trace. If omitted, no
optrace is emitted.

-s <number>
Specify the number of instructions to skip in the program before tracing begins.
The default value is 0.

-t <number>
The number of instructions to trace, after -s instructions have been skipped.
The default value is 1,000,000.

-main-count <0|1>
If set to 1, do not emit a trace. Instead, count dynamic instructions until
the first instruction at `main` and print the result, then exit.

-count-until-ip <address>
Do not emit a trace. Instead, count dynamic instructions until the first
instruction at the given address and print the result, then exit.
```
For example, you could trace 200,000 instructions of the program ls, after skipping the first 100,000 instructions, with this command:

    pin -t obj/champsim_tracer.so -o traces/ls_trace.champsim -s 100000 -t 200000 -- ls

To emit a second sidecar file with per-instruction PIN/XED metadata, add `-op`:

    pin -t obj/champsim_tracer.so -o traces/ls_trace.champsim -op traces/ls_trace.optrace -s 100000 -t 200000 -- ls

The sidecar file contains fixed-size `op_trace_instr` records defined in `inc/trace_instruction.h`. Each record is aligned one-to-one with the primary
trace and stores the dynamic instruction number, IP, raw PIN/XED category and opcode, memory operand count, and a few boolean flags
(`is_branch`, `is_memory`, `is_lea`, `is_mov`, `is_prefetch`).

To measure how many dynamic instructions execute before entering `main`, run:

    pin -t obj/champsim_tracer.so -main-count 1 -- <your program>

To measure how many dynamic instructions execute before a specific PC/IP, run:

    pin -t obj/champsim_tracer.so -count-until-ip 0x4010d8 -- <your program>

Traces created with the champsim_tracer.so are approximately 64 bytes per instruction, but they generally compress down to less than a byte per instruction using xz compression.
