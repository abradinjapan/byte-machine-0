# Instructions

Apologies, please review the BM0__write_instruction__N functions in file BM0.h to get an understanding of instruction parameters.

There are currently only 9 instructions.

## Quit

This instruction quits the current instance of the byte machine and returns a buffer.

## Write Register

This instruction writes a 64-bit value to a register.

## Allocate

This instruction tries to allocate a specified amount of bytes and returns a buffer.

## Deallocate

This instruction deallocates a buffer's bytes.

## Buffer To Register

This instruction takes a specified amount of bytes from a buffer and reads it into a register.

## Register To Register

This instruction fully copies one register to another register.

## Register To Buffer

This instruction takes a specified amount of bytes from a register and writes it to a buffer.

## Operate

This instruction performs binary, integer and comparison operations on registers.

This instruction is the only conditionally executed instruction.

## Do x86_64 Linux Syscall Limited

This instruction performs opening, closing, reading, writing and getting of file statistics on files.
