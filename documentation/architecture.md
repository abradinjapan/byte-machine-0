# Architecture

## Important Thing To Know

There are a few things that you should know when using or modifying this byte machine:

### Allocation Limits

The byte machine has a hard coded limit of 256 allocations per byte machine instance.

This is because allocations are made with the linux kernel and not the C standard library.

The linux kernel (at the time of writing) has roughly a default system wide maximum of 2 ^ 16 allocations.

If unlimited allocations were possible by the program, and that program made too many allocations, this would break the program and possibly the OS.

The byte machine's limit can be set higher, but this is not advised unless you know what you're doing.

The byte machine's allocation limit was also set to improve performance by limiting time spent performing allocations.

Thus you are expected to manage memory wisely by precalculating allocation sizes when possible.

### Input Sub-Buffer Limits

The byte machine's input sub-buffers are limited to 16 unique buffers.

The input sub-buffers are intended to be configurations.

The input sub-buffers are not intended for passing files that the program should be opening itself.

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

## How Memory is Addressed

Memory is addressed by using virtual pointers available to the byte machine process.

In other words, memory is addressed by using the hardware's virtual pointers.

## Standard Data Types

### Buffer

The `buffer` datatype is a 64-bit byte length combined with a 64-bit untyped pointer in that order.

This is returned with every allocation request and used for every deallocation request.
