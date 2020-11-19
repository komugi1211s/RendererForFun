#ifndef _K_ENGINE_CPP
#define _K_ENGINE_CPP

#include "engine.h"
/*
 * =========================================
 * Memory Stuff.
 * =========================================
 * */

uint8 *Arena::alloc(size_t allocation_size) {
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

TempArena Arena::temparena(size_t temparena_size) {
    ASSERT(!this->temparena_is_present);
    ASSERT((this->used + temparena_size) < this->capacity);
    uptr pointer = (uptr)((this->data + this->capacity) - temparena_size);

    uint8 modulo = (pointer & (MEMORY_ALIGNMENT - 1));
    if (modulo > 0) {
        pointer -= modulo;
        temparena_size += modulo;
    }

    TempArena temparena;
    temparena.capacity = temparena_size;
    temparena.used = 0;
    temparena.data = (uint8 *)pointer;
    temparena.derived_from = this;

    this->capacity -= temparena_size;
    this->temparena_is_present = 1;
    return temparena;
}

uint8 *TempArena::alloc(size_t allocation_size) {
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

void TempArena::close() {
    this->derived_from->capacity += this->capacity;
    this->derived_from->temparena_is_present = 0;
    this->data = NULL;
    this->used = 0;
    this->capacity = 0;
}

/*
 * =========================================
 * Profiler Stuff.
 * =========================================
 * */

void profile_begin(const char *name) {
    size_t length = strlen(name);
    ProfileInfo *info = 0;
    for (int32 i = 0; i < profile_info_count; ++i) {
        if (strcmp(profile_info[i].name, name) == 0) {
            info = &profile_info[i];
            break;
        }
    }

    if (!info) {
        if((profile_info_count + 1) < PROFILE_INFO_MAX_SIZE) {
            size_t new_index = profile_info_count++;
            info = (profile_info + new_index);
            info->name = name;
            info->cycle_count = __rdtsc();
        } else {
            return;
        }
    }

    info->is_started = 1;
    return;
}

void profile_end(const char *name) {
    uint64 end = __rdtsc();
    size_t length = strlen(name);
    ProfileInfo *info = NULL;

    for (int32 i = 0; i < profile_info_count; ++i) {
        if (strcmp(profile_info[i].name, name) == 0) {
            info = &profile_info[i];
            break;
        }
    }

    if (info) {
        info->is_started = 0;
        info->cycle_elapsed = end - info->cycle_count;
        info->cycle_count = end;
    }
}

#endif // _K_ENGINE_CPP
