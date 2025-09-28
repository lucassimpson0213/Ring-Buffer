# Ring Buffer (C)

An implementation of a circular (ring) buffer in C. A ring buffer is a fixed-size data structure that uses a single, contiguous block of memory and wraps around when the end is reached. This makes it efficient for scenarios where data is produced and consumed at different rates.

## Features
- Fixed-size circular buffer
- Constant-time enqueue (write) and dequeue (read)
- Overwrite mode or blocking mode (configurable)
- Simple API for buffer operations

## Tech Stack
- C (pointers, arrays)
- Standard Library for testing (stdio, assert)

## Usage
```c
#include "ring_buffer.h"

#define SIZE 8

int main() {
    ring_buffer_t rb;
    rb_init(&rb, SIZE);

    rb_write(&rb, 42);
    rb_write(&rb, 84);

    int value;
    rb_read(&rb, &value);
    printf("Read value: %d\n", value); // Output: 42

    rb_free(&rb);
    return 0;
}
```
## Compile and run locally
gcc ring_buffer.c main.c -o ring_buffer
./ring_buffer
