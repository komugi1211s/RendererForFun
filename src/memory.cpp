#ifndef _K_MEMORY_CPP
#define _K_MEMORY_CPP

#include "general.h"
#include "memory.h"

void Arena::initialize(uint8 *backing_buffer, size_t buffer_capacity) {
    this->data       = backing_buffer;
    this->capacity   = buffer_capacity;
    this->used       = 0;
    this->temp_count = 0;
}

uint8 *Arena::alloc(size_t allocation_size) {
    uptr pointer = (uptr)(this->data + this->used);
    uptr aligned_pointer = align_pointer_forward(pointer);

    // TODO(fuzzy): @Bug ちょっと違う？
    // 正常にアラインはされてはいるが、前回のアロケーションと今行っているアロケーションの合間に隙間が開く
    if (aligned_pointer > pointer) {
        allocation_size += (size_t)(aligned_pointer - pointer);
    }

    ASSERT((this->used + allocation_size) < this->capacity);

    uint8 *heap_memory = (uint8 *)aligned_pointer;
    this->used += allocation_size;

    return heap_memory;
}

TemporaryMemory Arena::begin_temporary(size_t temparena_size) {
    uptr pointer = (uptr)(this->data + this->used);
    uptr aligned_pointer = align_pointer_forward(pointer);

    if (aligned_pointer > pointer) {
        temparena_size += (size_t)(aligned_pointer - pointer);
    }

    ASSERT((this->used + temparena_size) < this->capacity);


    TemporaryMemory temparena;
    temparena.capacity = temparena_size;
    temparena.data     = (uint8 *)pointer;
    temparena.used     = 0;
    temparena.derived_from = this;

    this->temp_count += 1;
    return temparena;
}

uint8 *TemporaryMemory::alloc(size_t allocation_size) {
    uptr pointer = (uptr)(this->data + this->used);
    uptr aligned_pointer = align_pointer_forward(pointer);

    if (aligned_pointer > pointer) {
        allocation_size += (size_t)(aligned_pointer - pointer);
    }

    ASSERT((this->used + allocation_size) < this->capacity);

    uint8 *heap_memory = (uint8 *)aligned_pointer;
    this->used += allocation_size;

    return heap_memory;
}

void Arena::end_temporary(TemporaryMemory *child) {
    ASSERT(this->temp_count > 0);
    ASSERT(this == child->derived_from);

    this->used       -= child->capacity;
    this->temp_count -= 1;
}


#endif // _K_MEMORY_CPP
