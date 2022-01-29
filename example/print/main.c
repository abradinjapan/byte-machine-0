#include <stdio.h>

#include "../../BM0.h"

BM0__buffer BM0__example__create_program(BM0__et* allocation_error) {
    BM0__buffer output = BM0__create_buffer(allocation_error, 10000);
    BM0__buffer msg = BM0__create_buffer_from_c_string_copy(allocation_error, "Hello World!\n");
    void* index = output.p_data;
    unsigned long long rodata_offset = 5000;
    void* rodata_index = output.p_data + rodata_offset;

    // zero out registers
    for (unsigned long long i = BM0__rt__REGISTER_COUNT; i < BM0__define__register_count; i++) {
        index = BM0__write_instruction__write_register(index, i, 0);
    }

    // write constant values
    index = BM0__write_instruction__write_register(index, 200, sizeof(unsigned long long));
    index = BM0__write_instruction__write_register(index, 201, rodata_offset);
    index = BM0__write_instruction__write_register(index, 202, 1);
    index = BM0__write_instruction__write_register(index, 203, msg.p_length);

    // get input buffers pointer
    index = BM0__write_instruction__register_to_register(index, BM0__rt__input_buffers_pointer_register, 100);

    // adjust input buffers pointer copy to be at input buffer address
    index = BM0__write_instruction__operate(index, 255, 127, BM0__ot__integer__add, 100, 200, 100);

    // store input buffer 0 address
    index = BM0__write_instruction__buffer_to_register(index, 100, sizeof(void*), 100);

    // adjust input buffer 0 address to be at msg
    index = BM0__write_instruction__operate(index, 255, 127, BM0__ot__integer__add, 100, 201, 100);

    // print msg
    index = BM0__write_instruction__do_x86_64_linux_syscall_limited(index, BM0__st__write, 202, 100, 203, 0, 0, 0, 101);

    // exit gracefully
    index = BM0__write_instruction__quit(index, 255, 255);

    // write msg to rodata
    rodata_index = BM0__write_instruction__write_buffer_to_program(rodata_index, msg);

    // clean up data
    BM0__deallocate(msg.p_data, msg.p_length);

    return output;
}

BM0__buffer BM0__example__create_program_input(BM0__et* allocation_error, BM0__buffer program) {
    BM0__buffer output = BM0__create_buffer(allocation_error, sizeof(BM0__buffer));

    // set 0th sub-buffer to be the executed program
    ((BM0__buffer*)output.p_data)[0] = program;

    return output;
}

void BM0__example__destroy_program_input(BM0__buffer byte_machine_input) {
    // deallocate sub-buffers
    BM0__deallocate(((BM0__buffer*)byte_machine_input.p_data)[0].p_data, ((BM0__buffer*)byte_machine_input.p_data)[0].p_length);

    // deallocate byte machine input buffer
    BM0__deallocate(byte_machine_input.p_data, byte_machine_input.p_length);

    return;
}

int main() {
    BM0__et building_allocation_error = BM0__et__no_error;
    BM0__et runtime_error = BM0__et__no_error;
    BM0__buffer byte_machine_input;

    // build program & input
    printf("Creating Byte Machine Program.\n");
    byte_machine_input = BM0__example__create_program_input(&building_allocation_error, BM0__example__create_program(&building_allocation_error));
    if (building_allocation_error != BM0__et__no_error) {
        printf("Buildtime Error Code: %llu\n", (unsigned long long)runtime_error);

        return 0;
    }

    // run program
    printf("Running Byte Machine Program.\n");
    BM0__run_byte_machine(&runtime_error, byte_machine_input, BM0__boolean__true);

    // clean up
    printf("Cleaning Up Byte Machine Program & Printing Errors.\n");
    printf("Runtime Error Code: %llu\n", (unsigned long long)runtime_error);
    BM0__example__destroy_program_input(byte_machine_input);

    return 0;
}
