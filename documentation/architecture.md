# Architecture

## Input

The byte machine has one master input buffer that contains 1 - 16 sub-buffers for program input.

The 1st (0th indexed) input buffer must always be the program itself.

The rest of the input buffers are for configuring the program being run.

## Registers

The byte machine has 256 registers.

All data being operated on, with the exception of allocation handles, is stored in registers.

The registers are:

- The instruction pointer at register 0.
- The current instruction ID at register 1.
- The current instruction's parameters at registers 2 - 9.
- The input buffers length at register 10
- The input buffers pointer at register 11
- The error code register reference at register 12
- General purpose registers are registers 13 - 255

All 256 registers are fully readable and writable, there are NO restrictions; you have been warned.

## Memory

Memory is stored in buffers.

The `buffer` datatype is a 64-bit byte length combined with a 64-bit untyped pointer in that order.

There is a maximum of 256 buffers available for allocation per byte machine instance (not including input sub-buffers).

The buffer's untyped pointer is addressed by using virtual pointers available to the byte machine process.

In other words, memory is addressed by using the hardware's virtual pointers.

## Programs

Programs are always executed at byte 0 of the 0th input buffer.

Programs also always quit at the first quit instruction.
