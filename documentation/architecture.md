# Architecture

## Input

The byte machine has one master input buffer that contains 1 - 16 sub-buffers for program input.

The 16 sub-buffer maximum is arbitrary, this limit must be changed pre-compile time however.

Reasoning:

- The 1st (0th indexed) input buffer must always be the program itself.
- The rest of the input buffers are for configuring the program being run.
- 16 unique inputs should be enough to configure any program; intended best practice is that the program is responsible for opening files that it needs, not the calling function.

## Registers

The byte machine has 256 registers.

This is for a few reasons:

- To make it impossible for an 8-bit register number to generate an invalid register number exception.
- To give ample room for program wide constants.
- To give ample room for storing temporary data.
- To make caching less intense on computer processor.

All data being operated on, with the exception of allocation handles, is stored in registers.

This includes:

- The instruction pointer at register 0.
- The current instruction ID at register 1.
- The current instruction's parameters at registers 2 - 9.
- The input buffers length at register 10
- The input buffers pointer at register 11
- The error code register reference at register 12

All 256 registers are fully readable and writable, there are NO restrictions; you have been warned.

This is for a few reasons:

- To give the most amount of freedom to the programmer.
- To keep instructions very simple.
- To keep the amount of instructions to nearly an absolute minimum.
- To keep address jumping from being a low level nightmare.

## How Memory is Addressed

Memory is addressed by using virtual pointers available to the byte machine process.

In other words, memory is addressed by using the hardware's virtual pointers.

This is for a few reasons:

- To make it easy to keep track of buffers.
- To give the programmer the most amount of freedom.
- To reduce complexity for the byte machine and programmer.

## Standard Data Types

### Buffer

The `buffer` datatype is a 64-bit byte length combined with a 64-bit untyped pointer in that order.

This is used for every allocation request and deallocation request.
