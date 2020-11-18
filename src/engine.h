#ifndef _K_ENGINE_H
#define _K_ENGINE_H

typedef struct MouseInput {
    real32 x, y;
    bool32 left, right;
} MouseInput;

typedef struct Input {
    bool32 forward, backward, left, right;
    MouseInput mouse;
    MouseInput prev_mouse;
    bool32 debug_menu_key;
} Input;

typedef struct FontData {
    stbtt_fontinfo font_info;
    int32          ascent;
    int32          descent;
    int32          line_gap;
    int32          base_line;
    real32         char_scale;
} FontData;

typedef struct Color {
    real32 r, g, b, a;

    uint32 to_uint32_argb() {
        uint8 u8r, u8g, u8b, u8a;

        u8a = (uint8)floor(fmax(0.0, fmin(a, 1.0)) * 255.0);
        u8r = (uint8)floor(fmax(0.0, fmin(r, 1.0)) * 255.0);
        u8g = (uint8)floor(fmax(0.0, fmin(g, 1.0)) * 255.0);
        u8b = (uint8)floor(fmax(0.0, fmin(b, 1.0)) * 255.0);

        uint32 result = ((u8a << 24) | (u8r << 16) | (u8g << 8) | u8b);
        return result;
    }

    static Color from_uint32_argb(uint32 from) {
        real32 ra, rg, rb, rr;

        ra = (real32)((from & 0xFF000000) >> 24) / 255.0;
        rr = (real32)((from & 0x00FF0000) >> 16) / 255.0;
        rg = (real32)((from & 0x0000FF00) >>  8) / 255.0;
        rb = (real32)((from & 0x000000FF)      ) / 255.0;

        Color result;
        result.a = ra;
        result.r = rr;
        result.g = rg;
        result.b = rb;
        return result;
    }

    Color blend(Color other) {
        /*
         * Typical Alpha multiply Blend
         * https://ja.wikipedia.org/wiki/%E3%82%A2%E3%83%AB%E3%83%95%E3%82%A1%E3%83%96%E3%83%AC%E3%83%B3%E3%83%89
         * */

        Color result = {0};
        result.a = a + other.a * (1.0 - a);
        result.r = r + other.r * (1.0 - a);
        result.g = g + other.g * (1.0 - a);
        result.b = b + other.b * (1.0 - a);

        return result;
    }
} Color;

Color rgba(real32 r, real32 g, real32 b, real32 a) {
    Color result;
    result.r = r;
    result.g = g;
    result.b = b;
    result.a = a;

    return result;
}

Color rgb_opaque(real32 r, real32 g, real32 b) {
    Color result;
    result.r = r;
    result.g = g;
    result.b = b;
    result.a = 1.0;

    return result;
}


typedef struct TempArena TempArena;

typedef struct Arena {
    size_t capacity;
    size_t used;
    uint8  *data;

    size_t original_capacity;

    Arena(uint8 *storage_ptr, size_t storage_capacity) {
        original_capacity = storage_capacity;
        capacity          = storage_capacity;
        data = storage_ptr;
        used = 0;
    }

    uint8    *alloc(size_t allocation_size);
    TempArena temparena(size_t temparena_size);
} Arena;

typedef struct TempArena {
    size_t  capacity;
    size_t  used;
    uint8   *data;

    Arena   *derived_from;

    uint8 *alloc(size_t allocation_size);
    void close();
} TempArena;

global_variable const size_t MEMORY_ALIGNMENT = 16;

uptr align_pointer_forward(uptr unaligned) {
    uptr modulo = (unaligned & (MEMORY_ALIGNMENT - 1));
    uptr aligned = unaligned;

    if (modulo != 0) {
        aligned += MEMORY_ALIGNMENT - modulo;
    }

    return aligned;
}

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
    this->data = NULL;
    this->used = 0;
    this->capacity = 0;
}

#endif
