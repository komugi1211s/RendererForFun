#ifndef _K_MEMORY_H
#define _K_MEMORY_H
/*
 * =========================================
 * NOTE(fuzzy):
 * 内部でヒープメモリ確保関数を呼ばないので、自分で確保したバッキングバッファを与えるように。
 *
 * Arena
 * リニアなアリーナ。内部で16アラインを行うので確保したメモリ容量よりもデータが収まらない可能性がある。
 * 仕様として、こいつから確保したメモリをアリーナに返す事は出来ない（Free不可）。
 * ライフタイムが決まっている物をまとめて放り込むのに使う。
 *
 * TemporaryMemory
 * 親となるアリーナから派生するアリーナ。さくっと作ってさくっと解放するのに使う。
 * Arenaがフリーを行えない代わりに、TemporaryMemoryを作ってそのメモリ領域で作業をするのに便利。
 * 複数作成した場合は、最も新しいものから順に解放していく必要がある。
 *
 * FixedArray<T>
 * 固定長の配列。何故固定長なのかと言うと上記のバッキングバッファ云々参照。
 * 添字でアクセス可能。Pushに失敗すると0を返す。
 * TemporaryMemoryからメモリを拾ってきて一時的な配列として、若しくはヒープ確保したバッファを叩き込んで普通の配列として使う。
 * 確保したメモリは .data の値をそのままfreeしよう。
 * =========================================
 * */

global_variable const size_t MEMORY_ALIGNMENT = 16;

uptr align_pointer_forward(uptr unaligned) {
    uptr modulo = (unaligned & (MEMORY_ALIGNMENT - 1));
    uptr aligned = unaligned;

    if (modulo != 0) {
        aligned += MEMORY_ALIGNMENT - modulo;
    }

    return aligned;
}

typedef struct TemporaryMemory TemporaryMemory;
typedef struct Arena {
    size_t capacity;
    size_t used;
    uint8  *data;

    int32  temp_count;

    void             initialize(uint8 *backing_buffer, size_t buffer_capacity);
    uint8           *alloc(size_t allocation_size);
    TemporaryMemory  begin_temporary(size_t temparena_size);
    void             end_temporary(TemporaryMemory *child);
} Arena;

typedef struct TemporaryMemory {
    size_t  capacity;
    size_t  used;
    uint8   *data;
    Arena   *derived_from;

    uint8 *alloc(size_t allocation_size);
    void close();
} TemporaryMemory;

template<typename T>
struct FixedArray {
    T *data;
    size_t capacity; // NOTE(fuzzy): Arena系ではここはバイトサイズだが、Arrayでは許容できる数。
    size_t count;

    void initialize(T *backing_buffer, size_t buffer_capacity_count) {
        data = backing_buffer;
        capacity = buffer_capacity_count;
        count = 0;
    }

    bool32 push(T newitem) {
        if (capacity <= count) return 0;
        data[count++] = newitem;
        return 1;
    }

    bool32  pop(T *out) {
        if(count == 0) return 0;
        *out = data[--count];
        return 1;
    }

    bool32 replace(T newitem, size_t index, T *olditem) {
        if (count <= index) return 0;
        if (olditem) *olditem = data[index];
        data[index] = newitem;
        return 1;
    }

    bool32 replace(T newitem, size_t index) {
        return replace(newitem, index, NULL);
    }

    void clear() { count = 0; }

    T operator [](size_t index) {
        return data[index];
    }
};

#endif // _K_MEMORY_H
