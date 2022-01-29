#ifndef __BM0__
#define __BM0__

/* Include */
// performing allocation and deallocation
#include <sys/mman.h>

// doing linux syscalls
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

// debug info
#include <stdio.h>

/* Define */
typedef enum BM0__define {
    BM0__define__register_count = 256,
    BM0__define__max_allocation_count = 256,
    BM0__define__max_input_sub_buffer_count = 16,
    BM0__define__max_parameter_count = 8
} BM0__define;

/* Boolean */
// boolean
typedef enum BM0__boolean {
    BM0__boolean__false,
    BM0__boolean__true
} BM0__boolean;

/* Library */
unsigned long long BM0__null_terminated_string_length_with_null(char* c_string) {
    unsigned long long length = 0;

    while (c_string[length] != 0) {
        length++;
    }

    return length + 1;
}

unsigned long long BM0__null_terminated_string_length_without_null(char* c_string) {
    unsigned long long length = 0;

    while (c_string[length] != 0) {
        length++;
    }

    return length;
}

void* BM0__allocate(unsigned long long length) {
    return mmap(0, length, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
}

void BM0__deallocate(void* address, unsigned long long length) {
    munmap(address, length);

    return;
}

void BM0__copy_bytes(void* source, unsigned long long length, void* destination) {
    for (unsigned long long i = 0; i < length; i++) {
        *((unsigned char*)(destination + i)) = *((unsigned char*)source + i);
    }

    return;
}

void BM0__change_void_pointer_in_place(void** variable_reference, unsigned long long change) {
    *variable_reference = (void*)(((unsigned long long)(*variable_reference)) + change);

    return;
}

/* Error */
// error type
typedef enum BM0__et {
    // none
    BM0__et__no_error,

    // allocation and deallocation
    BM0__et__allocation_failure__at_maximum,
    BM0__et__allocation_failure__os_rejected_request,
    BM0__et__deallocation_failure,

    // input
    BM0__et__invalid_input_buffer, // critical error
    BM0__et__out_of_bounds_input_buffer_requested,

    // buffers & registers
    BM0__et__invalid_byte_transfer_size,

    // operations
    BM0__et__unimplemented_operation, // critical error
    BM0__et__division_by_zero_attempted,
    BM0__et__modulus_by_zero_attempted,

    // syscalls
    BM0__et__unimplemented_syscall, // critical error

    // instruction reading
    BM0__et__unimplemented_instruction_ID // critical error
} BM0__et;

/* Buffer */
typedef struct BM0__buffer {
    unsigned long long p_length;
    void* p_data;
} BM0__buffer;

void BM0__destroy_buffer(BM0__buffer buffer) {
    BM0__deallocate(buffer.p_data, buffer.p_length);

    return;
}

BM0__buffer BM0__create_null_buffer() {
    BM0__buffer output;

    output.p_length = 0;
    output.p_data = 0;

    return output;
}

BM0__buffer BM0__create_buffer(BM0__et* error, unsigned long long length) {
    BM0__buffer output;
    void* allocation;

    // attempt allocation
    allocation = BM0__allocate(length);

    // deal with error or create buffer
    if (allocation == 0) {
        // set error
        *error = BM0__et__allocation_failure__os_rejected_request;

        // create blank buffer
        output = BM0__create_null_buffer();
    } else {
        // create buffer
        output.p_length = length;
        output.p_data = allocation;
    }

    return output;
}

BM0__buffer BM0__create_buffer_from_c_string_copy(BM0__et* error, char* c_string) {
    BM0__buffer output;
    unsigned long long length = BM0__null_terminated_string_length_without_null(c_string);

    // allocate buffer
    output = BM0__create_buffer(error, length);

    // copy string
    BM0__copy_bytes((void*)c_string, length, output.p_data);

    return output;
}

/* Allocation Management */
typedef struct BM0__allocations {
    BM0__buffer p_buffers[BM0__define__max_allocation_count];
} BM0__allocations;

BM0__boolean BM0__check_allocation_exists(BM0__allocations* allocations, unsigned long long handle) {
    return (BM0__boolean)((*allocations).p_buffers[handle].p_data == 0);
}

void BM0__create_null_allocations(BM0__allocations* allocations) {
    // make all buffers null
    for (unsigned long long i = 0; i < BM0__define__max_allocation_count; i++) {
        (*allocations).p_buffers[i] = BM0__create_null_buffer();
    }

    return;
}

unsigned long long BM0__allocate_buffer_to_allocations(BM0__et* error, BM0__allocations* allocations, unsigned long long allocation_size) {
    // go through buffers until an empty is found or no empty buffers are found
    for (unsigned long long i = 0; i < BM0__define__max_allocation_count; i++) {
        // check if buffer is available
        if (BM0__check_allocation_exists(allocations, i)) {
            // create allocation
            (*allocations).p_buffers[i] = BM0__create_buffer(error, allocation_size);

            // found empty buffer, give back allocation handle
            return i;
        }
    }

    // no empty buffers are found
    *error = BM0__et__allocation_failure__at_maximum;

    // return 1 over the maximum indexable value
    return BM0__define__max_allocation_count;
}

unsigned long long BM0__allocation_count(BM0__allocations* allocations) {
    unsigned long long output = 0;

    // go through all buffers, counting all used slots
    for (unsigned long long i = 0; i < BM0__define__max_allocation_count; i++) {
        if ((*allocations).p_buffers[i].p_data != 0) {
            output++;
        }
    }

    return output;
}

void BM0__deallocate_buffer_from_allocations(BM0__et* error, BM0__allocations* allocations, unsigned long long handle) {
    // check if the buffer is in use
    if ((*allocations).p_buffers[handle].p_data != 0) {
        // destroy buffer
        BM0__destroy_buffer((*allocations).p_buffers[handle]);

        // zero buffer
        (*allocations).p_buffers[handle] = BM0__create_null_buffer();
    // cannot deallocate non-existent buffer
    } else {
        *error = BM0__et__deallocation_failure;
    }

    return;
}

/* Byte Machine */
// instruction length type
typedef enum BM0__ilt {
    BM0__ilt__quit = 4,
    BM0__ilt__write_register = 11,
    BM0__ilt__allocate = 6,
    BM0__ilt__deallocate = 3,
    BM0__ilt__buffer_to_register = 5,
    BM0__ilt__register_to_register = 4,
    BM0__ilt__register_to_buffer = 5,
    BM0__ilt__operate = 8,
    BM0__ilt__do_x86_64_linux_syscall_limited = 10
} BM0__ilt;

// register type
typedef enum BM0__rt {
    BM0__rt__instruction_pointer_register = 0,
    BM0__rt__instruction_ID_register = 1,
    BM0__rt__instruction_parameter_register_0 = 2,
    BM0__rt__instruction_parameter_register_1 = 3,
    BM0__rt__instruction_parameter_register_2 = 4,
    BM0__rt__instruction_parameter_register_3 = 5,
    BM0__rt__instruction_parameter_register_4 = 6,
    BM0__rt__instruction_parameter_register_5 = 7,
    BM0__rt__instruction_parameter_register_6 = 8,
    BM0__rt__instruction_parameter_register_7 = 9,
    BM0__rt__input_buffers_length_register = 10,
    BM0__rt__input_buffers_pointer_register = 11,
    BM0__rt__instruction_error_code_register_register = 12,
    BM0__rt__REGISTER_COUNT = 13
} BM0__rt;

// instruction type
typedef enum BM0__it {
    BM0__it__quit,
    BM0__it__write_register,
    BM0__it__allocate,
    BM0__it__deallocate,
    BM0__it__buffer_to_register,
    BM0__it__register_to_register,
    BM0__it__register_to_buffer,
    BM0__it__operate,
    BM0__it__do_x86_64_linux_syscall_limited
} BM0__it;

// operation type
typedef enum BM0__ot {
    // binary
    BM0__ot__binary__right_shift,
    BM0__ot__binary__left_shift,
    BM0__ot__binary__not,
    BM0__ot__binary__and,
    BM0__ot__binary__or,
    BM0__ot__binary__xor,

    // integer
    BM0__ot__integer__add,
    BM0__ot__integer__subtract,
    BM0__ot__integer__multiply,
    BM0__ot__integer__divide,
    BM0__ot__integer__modulous,

    // Comparison
    BM0__ot__comparison__less_than,
    BM0__ot__comparison__equal_to,
    BM0__ot__comparison__not_equal_to,
    BM0__ot__comparison__greater_than
} BM0__ot;

// syscall type
typedef enum BM0__st {
    BM0__st__read,
    BM0__st__write,
    BM0__st__open,
    BM0__st__close,
    BM0__st__stat,
    BM0__st__fstat
} BM0__st;

BM0__buffer BM0__run_byte_machine(BM0__et* error, BM0__buffer input_buffers_buffer, BM0__boolean final_debug_info) {
    // output
    BM0__buffer output = BM0__create_null_buffer();

    // allocations
    BM0__allocations* allocations = (BM0__allocations*)BM0__allocate(sizeof(BM0__buffer) * BM0__define__max_allocation_count);

    // registers
    void* regs[BM0__define__register_count];

    // temps
    struct stat stat_temporary;

    // check input for at least one buffer
    if ((input_buffers_buffer.p_length < sizeof(BM0__buffer) && input_buffers_buffer.p_length > (sizeof(BM0__buffer) * BM0__define__max_input_sub_buffer_count)) || input_buffers_buffer.p_length % sizeof(BM0__buffer) != 0) {
        *error = BM0__et__invalid_input_buffer;

        return output;
    }

    // setup
    *error = BM0__et__no_error;
    regs[BM0__rt__instruction_pointer_register] = ((BM0__buffer*)(input_buffers_buffer.p_data))[0].p_data; // setup instruction pointer
    regs[BM0__rt__input_buffers_pointer_register] = input_buffers_buffer.p_data; // setup the pointer to the input buffers
    regs[BM0__rt__input_buffers_length_register] = (void*)input_buffers_buffer.p_length; // setup the length of the input buffers
    BM0__create_null_allocations(allocations);

    // process instructions
    while (BM0__boolean__true) {
        // clear necessary registers
        regs[BM0__rt__instruction_parameter_register_0] = 0;
        regs[BM0__rt__instruction_parameter_register_1] = 0;
        regs[BM0__rt__instruction_parameter_register_2] = 0;
        regs[BM0__rt__instruction_parameter_register_3] = 0;
        regs[BM0__rt__instruction_parameter_register_4] = 0;
        regs[BM0__rt__instruction_parameter_register_5] = 0;
        regs[BM0__rt__instruction_parameter_register_6] = 0;
        regs[BM0__rt__instruction_parameter_register_7] = 0;

        // get instruction ID
        BM0__copy_bytes(regs[BM0__rt__instruction_pointer_register], 2, (void*)&regs[BM0__rt__instruction_ID_register]);

        // go to proper instruction's operations
        switch ((BM0__it)(unsigned short)(unsigned long long)regs[BM0__rt__instruction_ID_register]) {
        case BM0__it__quit:
            // read parameters
            BM0__copy_bytes(regs[BM0__rt__instruction_pointer_register] + 2, 1, &regs[BM0__rt__instruction_parameter_register_0]);
            BM0__copy_bytes(regs[BM0__rt__instruction_pointer_register] + 3, 1, &regs[BM0__rt__instruction_parameter_register_1]);

            // perform action
            output.p_data = regs[(unsigned char)(unsigned long long)regs[BM0__rt__instruction_parameter_register_0]];
            output.p_length = (unsigned long long)regs[(unsigned char)(unsigned long long)regs[BM0__rt__instruction_parameter_register_1]];

            if (final_debug_info == BM0__boolean__true) {
                printf("Instruction 'quit' called, dumping registers, dumping output and quitting byte machine...\n");

                for (unsigned long long i = 0; i < 32; i++) {
                    printf("\t[ ");

                    for (unsigned long long j = 0; j < 8; j++) {
                        printf("%llu ", (unsigned long long)regs[(i * 8) + j]);
                    }

                    printf("]\n");
                }

                printf("\tOutput: [ %llu, %llu ]\n", (unsigned long long)output.p_data, (unsigned long long)output.p_length);
            }

            return output;
        case BM0__it__write_register:
            // read parameters
            BM0__copy_bytes(regs[BM0__rt__instruction_pointer_register] + 2, 1, &regs[BM0__rt__instruction_parameter_register_0]);
            BM0__copy_bytes(regs[BM0__rt__instruction_pointer_register] + 3, 8, &regs[BM0__rt__instruction_parameter_register_1]);

            // perform action
            regs[(unsigned char)(unsigned long long)regs[BM0__rt__instruction_parameter_register_0]] = regs[BM0__rt__instruction_parameter_register_1];

            // change instruction index
            BM0__change_void_pointer_in_place(&regs[BM0__rt__instruction_pointer_register], BM0__ilt__write_register);

            break;
        case BM0__it__allocate:
            // read parameters
            BM0__copy_bytes(regs[BM0__rt__instruction_pointer_register] + 2, 1, &regs[BM0__rt__instruction_parameter_register_0]);
            BM0__copy_bytes(regs[BM0__rt__instruction_pointer_register] + 3, 1, &regs[BM0__rt__instruction_parameter_register_1]);
            BM0__copy_bytes(regs[BM0__rt__instruction_pointer_register] + 4, 1, &regs[BM0__rt__instruction_parameter_register_2]);
            BM0__copy_bytes(regs[BM0__rt__instruction_pointer_register] + 5, 1, &regs[BM0__rt__instruction_parameter_register_3]);

            // perform action
            regs[(unsigned char)(unsigned long long)regs[BM0__rt__instruction_parameter_register_1]] = (void*)BM0__allocate_buffer_to_allocations((BM0__et*)&regs[(unsigned char)(unsigned long long)regs[BM0__rt__instruction_error_code_register_register]], allocations, (unsigned long long)regs[(unsigned char)(unsigned long long)regs[BM0__rt__instruction_parameter_register_0]]);
            if (regs[(unsigned char)(unsigned long long)regs[BM0__rt__instruction_parameter_register_1]] < (void*)(unsigned long long)(unsigned short)BM0__define__max_allocation_count) {
                // allocate
                regs[(unsigned char)(unsigned long long)regs[BM0__rt__instruction_parameter_register_2]] = (*allocations).p_buffers[(unsigned long long)regs[(unsigned char)(unsigned long long)regs[BM0__rt__instruction_parameter_register_1]]].p_data;
                regs[(unsigned char)(unsigned long long)regs[BM0__rt__instruction_parameter_register_3]] = (void*)((*allocations).p_buffers[(unsigned long long)regs[(unsigned char)(unsigned long long)regs[BM0__rt__instruction_parameter_register_1]]].p_length);
            } else {
                // make registers null
                regs[(unsigned char)(unsigned long long)regs[BM0__rt__instruction_parameter_register_2]] = 0;
                regs[(unsigned char)(unsigned long long)regs[BM0__rt__instruction_parameter_register_3]] = 0;
            }

            // change instruction index
            BM0__change_void_pointer_in_place(&regs[BM0__rt__instruction_pointer_register], BM0__ilt__allocate);

            break;
        case BM0__it__deallocate:
            // read parameters
            BM0__copy_bytes(regs[BM0__rt__instruction_pointer_register] + 2, 1, &regs[BM0__rt__instruction_parameter_register_0]);

            // perform action
            BM0__deallocate_buffer_from_allocations((BM0__et*)&regs[(unsigned char)(unsigned long long)regs[BM0__rt__instruction_error_code_register_register]], allocations, (unsigned long long)regs[(unsigned char)(unsigned long long)regs[BM0__rt__instruction_parameter_register_0]]);

            // change instruction index
            BM0__change_void_pointer_in_place(&regs[BM0__rt__instruction_pointer_register], BM0__ilt__deallocate);

            break;
        case BM0__it__buffer_to_register:
            // read parameters
            BM0__copy_bytes(regs[BM0__rt__instruction_pointer_register] + 2, 1, &regs[BM0__rt__instruction_parameter_register_0]);
            BM0__copy_bytes(regs[BM0__rt__instruction_pointer_register] + 3, 1, &regs[BM0__rt__instruction_parameter_register_1]);
            BM0__copy_bytes(regs[BM0__rt__instruction_pointer_register] + 4, 1, &regs[BM0__rt__instruction_parameter_register_2]);

            // perform action
            if ((unsigned long long)regs[BM0__rt__instruction_parameter_register_1] <= sizeof(unsigned long long)) {
                BM0__copy_bytes(regs[(unsigned char)(unsigned long long)regs[BM0__rt__instruction_parameter_register_0]], (unsigned long long)regs[BM0__rt__instruction_parameter_register_1], (void*)&regs[(unsigned char)(unsigned long long)regs[BM0__rt__instruction_parameter_register_2]]);
            } else {
                regs[(unsigned char)(unsigned long long)regs[BM0__rt__instruction_error_code_register_register]] = (void*)(unsigned long long)(unsigned short)BM0__et__invalid_byte_transfer_size;
            }

            // change instruction index
            BM0__change_void_pointer_in_place(&regs[BM0__rt__instruction_pointer_register], BM0__ilt__buffer_to_register);

            break;
        case BM0__it__register_to_register:
            // read parameters
            BM0__copy_bytes(regs[BM0__rt__instruction_pointer_register] + 2, 1, &regs[BM0__rt__instruction_parameter_register_0]);
            BM0__copy_bytes(regs[BM0__rt__instruction_pointer_register] + 3, 1, &regs[BM0__rt__instruction_parameter_register_1]);

            // perform action
            regs[(unsigned char)(unsigned long long)regs[BM0__rt__instruction_parameter_register_1]] = regs[(unsigned char)(unsigned long long)regs[BM0__rt__instruction_parameter_register_0]];

            // change instruction index
            BM0__change_void_pointer_in_place(&regs[BM0__rt__instruction_pointer_register], BM0__ilt__register_to_register);

            break;
        case BM0__it__register_to_buffer:
            // read parameters
            BM0__copy_bytes(regs[BM0__rt__instruction_pointer_register] + 2, 1, &regs[BM0__rt__instruction_parameter_register_0]);
            BM0__copy_bytes(regs[BM0__rt__instruction_pointer_register] + 3, 1, &regs[BM0__rt__instruction_parameter_register_1]);
            BM0__copy_bytes(regs[BM0__rt__instruction_pointer_register] + 4, 1, &regs[BM0__rt__instruction_parameter_register_2]);

            // perform action
            if ((unsigned long long)regs[BM0__rt__instruction_parameter_register_1] <= sizeof(unsigned long long)) {
                BM0__copy_bytes((void*)&regs[(unsigned char)(unsigned long long)regs[BM0__rt__instruction_parameter_register_0]], (unsigned long long)regs[BM0__rt__instruction_parameter_register_1], regs[(unsigned char)(unsigned long long)regs[BM0__rt__instruction_parameter_register_2]]);
            } else {
                regs[(unsigned char)(unsigned long long)regs[BM0__rt__instruction_error_code_register_register]] = (void*)(unsigned long long)(unsigned short)BM0__et__invalid_byte_transfer_size;
            }

            // change instruction index
            BM0__change_void_pointer_in_place(&regs[BM0__rt__instruction_pointer_register], BM0__ilt__register_to_buffer);

            break;
        case BM0__it__operate:
            // read parameters
            BM0__copy_bytes(regs[BM0__rt__instruction_pointer_register] + 2, 1, &regs[BM0__rt__instruction_parameter_register_0]);
            BM0__copy_bytes(regs[BM0__rt__instruction_pointer_register] + 3, 1, &regs[BM0__rt__instruction_parameter_register_1]);
            BM0__copy_bytes(regs[BM0__rt__instruction_pointer_register] + 4, 1, &regs[BM0__rt__instruction_parameter_register_2]);
            BM0__copy_bytes(regs[BM0__rt__instruction_pointer_register] + 5, 1, &regs[BM0__rt__instruction_parameter_register_3]);
            BM0__copy_bytes(regs[BM0__rt__instruction_pointer_register] + 6, 1, &regs[BM0__rt__instruction_parameter_register_4]);
            BM0__copy_bytes(regs[BM0__rt__instruction_pointer_register] + 7, 1, &regs[BM0__rt__instruction_parameter_register_5]);

            // perform action
            if ((unsigned char)(unsigned long long)regs[BM0__rt__instruction_parameter_register_1] < (unsigned char)32) {
                // use if wanting byte machine to look for bit and get operation directly from operation parameter
                regs[BM0__rt__instruction_parameter_register_6] = (void*)(unsigned long long)(unsigned short)BM0__boolean__false;
                regs[BM0__rt__instruction_parameter_register_7] = (void*)regs[BM0__rt__instruction_parameter_register_2];
            } else if ((unsigned char)(unsigned long long)regs[BM0__rt__instruction_parameter_register_1] < (unsigned char)64) {
                // use if wanting byte machine to look for bit and get operation from operation parameter's specified register
                regs[BM0__rt__instruction_parameter_register_6] = (void*)(unsigned long long)(unsigned short)BM0__boolean__false;
                regs[BM0__rt__instruction_parameter_register_7] = regs[(unsigned char)(unsigned long long)regs[BM0__rt__instruction_parameter_register_2]];
            } else if ((unsigned char)(unsigned long long)regs[BM0__rt__instruction_parameter_register_1] < (unsigned char)128) {
                // use if wanting byte machine to always perform operation and get operation directly from operation parameter
                regs[BM0__rt__instruction_parameter_register_6] = (void*)(unsigned long long)(unsigned short)BM0__boolean__true;
                regs[BM0__rt__instruction_parameter_register_7] = (void*)regs[BM0__rt__instruction_parameter_register_2];
            } else {
                // use if wanting byte machine to always perform operation and get operation from operation parameter's specified register
                regs[BM0__rt__instruction_parameter_register_6] = (void*)(unsigned long long)(unsigned short)BM0__boolean__true;
                regs[BM0__rt__instruction_parameter_register_7] = regs[(unsigned char)(unsigned long long)regs[BM0__rt__instruction_parameter_register_2]];
            }

            if (regs[BM0__rt__instruction_parameter_register_6] || ((unsigned long long)(((unsigned long long)regs[(unsigned char)(unsigned long long)regs[BM0__rt__instruction_parameter_register_0]]) & (unsigned long long)((1 << (unsigned char)(unsigned long long)regs[BM0__rt__instruction_parameter_register_1]))) > 0)) {
                switch ((BM0__ot)(unsigned short)(unsigned long long)regs[BM0__rt__instruction_parameter_register_7]) {
                case BM0__ot__binary__right_shift:
                    regs[(unsigned char)(unsigned long long)regs[BM0__rt__instruction_parameter_register_5]] = (void*)((unsigned long long)regs[(unsigned char)(unsigned long long)regs[BM0__rt__instruction_parameter_register_3]] >> (unsigned long long)regs[(unsigned char)(unsigned long long)regs[BM0__rt__instruction_parameter_register_4]]);
                    break;
                case BM0__ot__binary__left_shift:
                    regs[(unsigned char)(unsigned long long)regs[BM0__rt__instruction_parameter_register_5]] = (void*)((unsigned long long)regs[(unsigned char)(unsigned long long)regs[BM0__rt__instruction_parameter_register_3]] << (unsigned long long)regs[(unsigned char)(unsigned long long)regs[BM0__rt__instruction_parameter_register_4]]);
                    break;
                case BM0__ot__binary__not:
                    regs[(unsigned char)(unsigned long long)regs[BM0__rt__instruction_parameter_register_5]] = (void*)(~(unsigned long long)regs[(unsigned char)(unsigned long long)regs[BM0__rt__instruction_parameter_register_3]]);
                    break;
                case BM0__ot__binary__and:
                    regs[(unsigned char)(unsigned long long)regs[BM0__rt__instruction_parameter_register_5]] = (void*)((unsigned long long)regs[(unsigned char)(unsigned long long)regs[BM0__rt__instruction_parameter_register_3]] & (unsigned long long)regs[(unsigned char)(unsigned long long)regs[BM0__rt__instruction_parameter_register_4]]);
                    break;
                case BM0__ot__binary__or:
                    regs[(unsigned char)(unsigned long long)regs[BM0__rt__instruction_parameter_register_5]] = (void*)((unsigned long long)regs[(unsigned char)(unsigned long long)regs[BM0__rt__instruction_parameter_register_3]] | (unsigned long long)regs[(unsigned char)(unsigned long long)regs[BM0__rt__instruction_parameter_register_4]]);
                    break;
                case BM0__ot__binary__xor:
                    regs[(unsigned char)(unsigned long long)regs[BM0__rt__instruction_parameter_register_5]] = (void*)((unsigned long long)regs[(unsigned char)(unsigned long long)regs[BM0__rt__instruction_parameter_register_3]] ^ (unsigned long long)regs[(unsigned char)(unsigned long long)regs[BM0__rt__instruction_parameter_register_4]]);
                    break;
                case BM0__ot__integer__add:
                    regs[(unsigned char)(unsigned long long)regs[BM0__rt__instruction_parameter_register_5]] = (void*)((unsigned long long)regs[(unsigned char)(unsigned long long)regs[BM0__rt__instruction_parameter_register_3]] + (unsigned long long)regs[(unsigned char)(unsigned long long)regs[BM0__rt__instruction_parameter_register_4]]);
                    break;
                case BM0__ot__integer__subtract:
                    regs[(unsigned char)(unsigned long long)regs[BM0__rt__instruction_parameter_register_5]] = (void*)((unsigned long long)regs[(unsigned char)(unsigned long long)regs[BM0__rt__instruction_parameter_register_3]] - (unsigned long long)regs[(unsigned char)(unsigned long long)regs[BM0__rt__instruction_parameter_register_4]]);
                    break;
                case BM0__ot__integer__multiply:
                    regs[(unsigned char)(unsigned long long)regs[BM0__rt__instruction_parameter_register_5]] = (void*)((unsigned long long)regs[(unsigned char)(unsigned long long)regs[BM0__rt__instruction_parameter_register_3]] * (unsigned long long)regs[(unsigned char)(unsigned long long)regs[BM0__rt__instruction_parameter_register_4]]);
                    break;
                case BM0__ot__integer__divide:
                    if ((unsigned long long)regs[(unsigned char)(unsigned long long)regs[BM0__rt__instruction_parameter_register_4]] != 0) {
                        regs[(unsigned char)(unsigned long long)regs[BM0__rt__instruction_parameter_register_5]] = (void*)((unsigned long long)regs[(unsigned char)(unsigned long long)regs[BM0__rt__instruction_parameter_register_3]] / (unsigned long long)regs[(unsigned char)(unsigned long long)regs[BM0__rt__instruction_parameter_register_4]]);
                    } else {
                        regs[(unsigned char)(unsigned long long)regs[BM0__rt__instruction_error_code_register_register]] = (void*)(unsigned long long)(unsigned short)BM0__et__division_by_zero_attempted;
                    }

                    break;
                case BM0__ot__integer__modulous:
                    if ((unsigned long long)regs[(unsigned char)(unsigned long long)regs[BM0__rt__instruction_parameter_register_4]] != 0) {
                        regs[(unsigned char)(unsigned long long)regs[BM0__rt__instruction_parameter_register_5]] = (void*)((unsigned long long)regs[(unsigned char)(unsigned long long)regs[BM0__rt__instruction_parameter_register_3]] % (unsigned long long)regs[(unsigned char)(unsigned long long)regs[BM0__rt__instruction_parameter_register_4]]);
                    } else {
                        regs[(unsigned char)(unsigned long long)regs[BM0__rt__instruction_error_code_register_register]] = (void*)(unsigned long long)(unsigned short)BM0__et__modulus_by_zero_attempted;
                    }

                    break;
                case BM0__ot__comparison__less_than:
                    regs[(unsigned char)(unsigned long long)regs[BM0__rt__instruction_parameter_register_5]] = (void*)(unsigned long long)((unsigned long long)regs[(unsigned char)(unsigned long long)regs[BM0__rt__instruction_parameter_register_3]] < (unsigned long long)regs[(unsigned char)(unsigned long long)regs[BM0__rt__instruction_parameter_register_4]]);
                    break;
                case BM0__ot__comparison__equal_to:
                    regs[(unsigned char)(unsigned long long)regs[BM0__rt__instruction_parameter_register_5]] = (void*)(unsigned long long)((unsigned long long)regs[(unsigned char)(unsigned long long)regs[BM0__rt__instruction_parameter_register_3]] == (unsigned long long)regs[(unsigned char)(unsigned long long)regs[BM0__rt__instruction_parameter_register_4]]);
                    break;
                case BM0__ot__comparison__not_equal_to:
                    regs[(unsigned char)(unsigned long long)regs[BM0__rt__instruction_parameter_register_5]] = (void*)(unsigned long long)((unsigned long long)regs[(unsigned char)(unsigned long long)regs[BM0__rt__instruction_parameter_register_3]] != (unsigned long long)regs[(unsigned char)(unsigned long long)regs[BM0__rt__instruction_parameter_register_4]]);
                    break;
                case BM0__ot__comparison__greater_than:
                    regs[(unsigned char)(unsigned long long)regs[BM0__rt__instruction_parameter_register_5]] = (void*)(unsigned long long)((unsigned long long)regs[(unsigned char)(unsigned long long)regs[BM0__rt__instruction_parameter_register_3]] > (unsigned long long)regs[(unsigned char)(unsigned long long)regs[BM0__rt__instruction_parameter_register_4]]);
                    break;
                // in case there is an invalid / unimplemented operation ID
                default:
                    *error = BM0__et__unimplemented_operation;

                    return output;
                }
            }

            // change instruction index
            BM0__change_void_pointer_in_place(&regs[BM0__rt__instruction_pointer_register], BM0__ilt__operate);

            break;
        case BM0__it__do_x86_64_linux_syscall_limited:
            // read parameters
            BM0__copy_bytes(regs[BM0__rt__instruction_pointer_register] + 2, 1, &regs[BM0__rt__instruction_parameter_register_0]);
            BM0__copy_bytes(regs[BM0__rt__instruction_pointer_register] + 3, 1, &regs[BM0__rt__instruction_parameter_register_1]);
            BM0__copy_bytes(regs[BM0__rt__instruction_pointer_register] + 4, 1, &regs[BM0__rt__instruction_parameter_register_2]);
            BM0__copy_bytes(regs[BM0__rt__instruction_pointer_register] + 5, 1, &regs[BM0__rt__instruction_parameter_register_3]);
            BM0__copy_bytes(regs[BM0__rt__instruction_pointer_register] + 6, 1, &regs[BM0__rt__instruction_parameter_register_4]);
            BM0__copy_bytes(regs[BM0__rt__instruction_pointer_register] + 7, 1, &regs[BM0__rt__instruction_parameter_register_5]);
            BM0__copy_bytes(regs[BM0__rt__instruction_pointer_register] + 8, 1, &regs[BM0__rt__instruction_parameter_register_6]);
            BM0__copy_bytes(regs[BM0__rt__instruction_pointer_register] + 9, 1, &regs[BM0__rt__instruction_parameter_register_7]);
            
            // perform action
            switch ((BM0__ot)(unsigned short)(unsigned long long)regs[BM0__rt__instruction_parameter_register_0]) {
            case BM0__st__read:
                regs[(unsigned char)(unsigned long long)regs[BM0__rt__instruction_parameter_register_7]] = (void*)(unsigned long long)read((unsigned int)(unsigned long long)regs[(unsigned char)(unsigned long long)regs[BM0__rt__instruction_parameter_register_1]], regs[(unsigned char)(unsigned long long)regs[BM0__rt__instruction_parameter_register_2]], (size_t)(unsigned long long)regs[(unsigned char)(unsigned long long)regs[BM0__rt__instruction_parameter_register_3]]);

                break;
            case BM0__st__write:
                regs[(unsigned char)(unsigned long long)regs[BM0__rt__instruction_parameter_register_7]] = (void*)(unsigned long long)write((unsigned int)(unsigned long long)regs[(unsigned char)(unsigned long long)regs[BM0__rt__instruction_parameter_register_1]], regs[(unsigned char)(unsigned long long)regs[BM0__rt__instruction_parameter_register_2]], (size_t)(unsigned long long)regs[(unsigned char)(unsigned long long)regs[BM0__rt__instruction_parameter_register_3]]);

                break;
            case BM0__st__open:
                regs[(unsigned char)(unsigned long long)regs[BM0__rt__instruction_parameter_register_7]] = (void*)(unsigned long long)open((const char*)regs[(unsigned char)(unsigned long long)regs[BM0__rt__instruction_parameter_register_1]], (int)(unsigned long long)regs[(unsigned char)(unsigned long long)regs[BM0__rt__instruction_parameter_register_2]]);

                break;
            case BM0__st__close:
                regs[(unsigned char)(unsigned long long)regs[BM0__rt__instruction_parameter_register_7]] = (void*)(unsigned long long)close((int)(unsigned long long)regs[(unsigned char)(unsigned long long)regs[BM0__rt__instruction_parameter_register_1]]);

                break;
            case BM0__st__stat:
                regs[BM0__rt__instruction_parameter_register_7] = (void*)(unsigned long long)stat((const char*)regs[(unsigned char)(unsigned long long)regs[BM0__rt__instruction_parameter_register_1]], &stat_temporary);

                regs[(unsigned char)(unsigned long long)regs[BM0__rt__instruction_parameter_register_2]] = (void*)(unsigned long long)stat_temporary.st_size;
                regs[(unsigned char)(unsigned long long)regs[BM0__rt__instruction_parameter_register_3]] = (void*)(unsigned long long)stat_temporary.st_mode;
                
                break;
            case BM0__st__fstat:
                regs[BM0__rt__instruction_parameter_register_7] = (void*)(unsigned long long)fstat((int)(unsigned long long)regs[(unsigned char)(unsigned long long)regs[BM0__rt__instruction_parameter_register_1]], &stat_temporary);

                regs[(unsigned char)(unsigned long long)regs[BM0__rt__instruction_parameter_register_2]] = (void*)(unsigned long long)stat_temporary.st_size;
                regs[(unsigned char)(unsigned long long)regs[BM0__rt__instruction_parameter_register_3]] = (void*)(unsigned long long)stat_temporary.st_mode;
                
                break;
            default:
                *error = BM0__et__unimplemented_syscall;

                return output;
            }

            // change instruction index
            BM0__change_void_pointer_in_place(&regs[BM0__rt__instruction_pointer_register], BM0__ilt__do_x86_64_linux_syscall_limited);

            break;
        // in case no instruction is matched
        default:
            *error = BM0__et__unimplemented_instruction_ID;

            return output;
        }
    }

    // unreachable, but makes compiler happy
    return output;
}

/* Create Instructions & Data */
unsigned long long BM0__write_instruction__get_instruction_ilt(BM0__it opcode) {
    switch (opcode) {
    case BM0__it__quit:
        return BM0__ilt__quit;
    case BM0__it__write_register:
        return BM0__ilt__write_register;
    case BM0__it__allocate:
        return BM0__ilt__allocate;
    case BM0__it__deallocate:
        return BM0__ilt__deallocate;
    case BM0__it__buffer_to_register:
        return BM0__ilt__buffer_to_register;
    case BM0__it__register_to_register:
        return BM0__ilt__register_to_register;
    case BM0__it__register_to_buffer:
        return BM0__ilt__register_to_buffer;
    case BM0__it__operate:
        return BM0__ilt__operate;
    case BM0__it__do_x86_64_linux_syscall_limited:
        return BM0__ilt__do_x86_64_linux_syscall_limited;
    default:
        return 0;
    }
}

void* BM0__write_instruction__write_buffer_to_program(void* destination, BM0__buffer data) {
    BM0__copy_bytes(data.p_data, data.p_length, destination);

    return destination + data.p_length;
}

void* BM0__write_instruction__quit(void* destination, unsigned char buffer_pointer_source_register, unsigned char buffer_length_source_register) {
    unsigned short opcode = BM0__it__quit;

    BM0__copy_bytes(&opcode, 2, destination);
    BM0__copy_bytes(&buffer_pointer_source_register, 1, destination + 2);
    BM0__copy_bytes(&buffer_length_source_register, 1, destination + 3);

    return destination + (unsigned long long)BM0__ilt__quit;
}

void* BM0__write_instruction__write_register(void* destination, unsigned char destination_register, unsigned long long value) {
    unsigned short opcode = BM0__it__write_register;

    BM0__copy_bytes(&opcode, 2, destination);
    BM0__copy_bytes(&destination_register, 1, destination + 2);
    BM0__copy_bytes(&value, sizeof(unsigned long long), destination + 3);

    return destination + (unsigned long long)BM0__ilt__write_register;
}

void* BM0__write_instruction__allocate(void* destination, unsigned char allocation_size_register, unsigned char handle_destination_register, unsigned char pointer_destination_register, unsigned char length_destination_register) {
    unsigned short opcode = BM0__it__allocate;

    BM0__copy_bytes(&opcode, 2, destination);
    BM0__copy_bytes(&allocation_size_register, 1, destination + 2);
    BM0__copy_bytes(&handle_destination_register, 1, destination + 3);
    BM0__copy_bytes(&pointer_destination_register, 1, destination + 4);
    BM0__copy_bytes(&length_destination_register, 1, destination + 5);

    return destination + (unsigned long long)BM0__ilt__allocate;
}

void* BM0__write_instruction__deallocate(void* destination, unsigned char handle_register) {
    unsigned short opcode = BM0__it__deallocate;

    BM0__copy_bytes(&opcode, 2, destination);
    BM0__copy_bytes(&handle_register, 1, destination + 2);

    return destination + (unsigned long long)BM0__ilt__deallocate;
}

void* BM0__write_instruction__buffer_to_register(void* destination, unsigned char source_pointer_register, unsigned char byte_count, unsigned char destination_register) {
    unsigned short opcode = BM0__it__buffer_to_register;

    BM0__copy_bytes(&opcode, 2, destination);
    BM0__copy_bytes(&source_pointer_register, 1, destination + 2);
    BM0__copy_bytes(&byte_count, 1, destination + 3);
    BM0__copy_bytes(&destination_register, 1, destination + 4);

    return destination + (unsigned long long)BM0__ilt__buffer_to_register;
}

void* BM0__write_instruction__register_to_register(void* destination, unsigned char source_register, unsigned char destination_register) {
    unsigned short opcode = BM0__it__register_to_register;

    BM0__copy_bytes(&opcode, 2, destination);
    BM0__copy_bytes(&source_register, 1, destination + 2);
    BM0__copy_bytes(&destination_register, 1, destination + 3);

    return destination + (unsigned long long)BM0__ilt__register_to_register;
}

void* BM0__write_instruction__register_to_buffer(void* destination, unsigned char source_register, unsigned char byte_count, unsigned char destination_pointer_register) {
    unsigned short opcode = BM0__it__register_to_buffer;

    BM0__copy_bytes(&opcode, 2, destination);
    BM0__copy_bytes(&source_register, 1, destination + 2);
    BM0__copy_bytes(&byte_count, 1, destination + 3);
    BM0__copy_bytes(&destination_pointer_register, 1, destination + 4);

    return destination + (unsigned long long)BM0__ilt__register_to_buffer;
}

void* BM0__write_instruction__operate(void* destination, unsigned char flags_register_number, unsigned char required_flag_bit, unsigned char operation, unsigned char source_register_1, unsigned char source_register_2, unsigned char destination_register_1) {
    unsigned short opcode = BM0__it__operate;

    BM0__copy_bytes(&opcode, 2, destination);
    BM0__copy_bytes(&flags_register_number, 1, destination + 2);
    BM0__copy_bytes(&required_flag_bit, 1, destination + 3);
    BM0__copy_bytes(&operation, 1, destination + 4);
    BM0__copy_bytes(&source_register_1, 1, destination + 5);
    BM0__copy_bytes(&source_register_2, 1, destination + 6);
    BM0__copy_bytes(&destination_register_1, 1, destination + 7);

    return destination + (unsigned long long)BM0__ilt__operate;
}

void* BM0__write_instruction__do_x86_64_linux_syscall_limited(void* destination, unsigned char syscall_number, unsigned char argument_1, unsigned char argument_2, unsigned char argument_3, unsigned char argument_4, unsigned char argument_5, unsigned char argument_6, unsigned char return_value_destination_register) {
    unsigned short opcode = BM0__it__do_x86_64_linux_syscall_limited;

    BM0__copy_bytes(&opcode, 2, destination);
    BM0__copy_bytes(&syscall_number, 1, destination + 2);
    BM0__copy_bytes(&argument_1, 1, destination + 3);
    BM0__copy_bytes(&argument_2, 1, destination + 4);
    BM0__copy_bytes(&argument_3, 1, destination + 5);
    BM0__copy_bytes(&argument_4, 1, destination + 6);
    BM0__copy_bytes(&argument_5, 1, destination + 7);
    BM0__copy_bytes(&argument_6, 1, destination + 8);
    BM0__copy_bytes(&return_value_destination_register, 1, destination + 9);
    
    return destination + (unsigned long long)BM0__ilt__do_x86_64_linux_syscall_limited;
}

#endif
