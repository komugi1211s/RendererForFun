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

/*
 * =========================================
 * Profiler Stuff.
 * =========================================
 * */

void profile_begin(char *name) {
    size_t length = strlen(name);
    ProfileInfo *info = NULL;
    for (int32 i = 0; i < profile_info_count; ++i) {
        if (profile_info[i].name_length == length) {
            if (strcmp(profile_info[i].name, name) == 0) {
                info = &profile_info[i];
                break;
            }
        }
    }

    if (!info) {
        if((profile_info_count + 1) < PROFILE_INFO_MAX_SIZE) {
            size_t new_index = profile_info_count++;
            info = (profile_info + new_index);

            info->name          = name;
            info->name_length   = length;
            info->cycle_elapsed = 0;
            info->cycle_count   = __rdtsc();
        } else {
            return;
        }
    }
    return;
}

void profile_end(char *name) {
    uint64 end = __rdtsc();
    size_t length = strlen(name);
    ProfileInfo *info = NULL;

    for (int32 i = 0; i < profile_info_count; ++i) {
        if (profile_info[i].name_length == length) {
            if (strcmp(profile_info[i].name, name) == 0) {
                info = &profile_info[i];
                break;
            }
        }
    }

    if (info) {
        info->cycle_elapsed = end - info->cycle_count;
        info->cycle_count   = end;
    }
}

#endif // _K_ENGINE_CPP
