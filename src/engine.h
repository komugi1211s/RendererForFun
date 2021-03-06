#ifndef _K_ENGINE_H
#define _K_ENGINE_H

#include "memory.h"
#include "maths.h"
#include "asset.h"

/*
 * =========================================
 * Platform Stuff.
 *
 * NOTE(fuzzy): 重要。
 * レンダラは内部で下記３つの関数を必ず使うので
 * プログラム開始時に必ずplatform用の関数をセットアップする事。
 *
 * Platform.allocate_memory   :: AllocateMemoryFunc
 * 引数と戻り値はmallocと同じ。指定された量のメモリを確保し、失敗したらNULLを返す。
 *
 * Platform.deallocate_memory :: DeallocateMemoryFunc
 * freeと同じ。指定されたメモリを解放する。
 * 指定されたメモリがAllocateMemoryFuncで確保されてなければ未定義。
 *
 * Platform.open_and_read_file :: OpenAndReadEntireFileFunc
 * ファイル名が指定された際にそのファイルを読み込む。モードは読み込みバイナリで固定、テキストでは読めない。
 * ファイルが読めなかった場合はopenedが0になる。
 * 内部でAllocateMemoryFuncに指定された関数でメモリを確保し、DeallocateMemoryFuncでフリー出来る必要がある。
 *
 * NOTE(fuzzy):
 * やろうと思えばマクロでもイケるかなと思ったが、
 * マクロでやるならデフォルトの挙動を与えないといけないのでダメ（設定せずとも動く必要がある）。
 * =========================================
 * */

typedef struct FileObject {
    bool32 opened;
    size_t size;
    char   *content;
} FileObject;

typedef void *(*AllocateMemoryFunc)(size_t memory_size);
typedef void  (*DeallocateMemoryFunc)(void *ptr);
typedef FileObject (*OpenAndReadEntireFileFunc)(char *filename);

typedef struct Platform {
    AllocateMemoryFunc        allocate_memory;
    DeallocateMemoryFunc      deallocate_memory;
    OpenAndReadEntireFileFunc open_and_read_file;
} Platform;

/*
 * =========================================
 * Profiler Stuff.
 *
 * PROFILE(name)
 * for文を悪用したプロファイラマクロ。PROFILE(name) { ... } のようにして使える。
 *
 * PROFILE_FUNC
 * デストラクタを悪用したプロファイラマクロ。
 * 関数のトップレベルに放り込んで放置しておくだけで、スコープから抜けた際にプロファイルを行う。
 *
 * profile_begin(name)
 * profile_end(name)
 * 普通のプロファイル関数。同一の名前を指定すると、BeginからEndの間のサイクル数を計測する。
 * mallocとfree、fopenとfcloseのように対になってないとダメ。
 *
 * TODO(fuzzy):
 * プロファイラ自体が結構サイクルを使うので（本末転倒）そこを改善するように。
 *
 * =========================================
 * */

#define PROFILE_INFO_MAX_SIZE 255

/*
 * 一番頻繁に使うProfilerマクロ。
 *
 * 以下のようにしてブロックの計算時間を測るか
 * PROFILE("some expensive stuff") {
 *   ... expensive stuff ...
 * }
 *
 * 以下のようにして関数まるごと測れる
 * void func() {
 *   PROFILE_FUNC
 *   ... expensive computation stuff...
 * }
 * */

#define PROFILE(name) for(int i##__LINE__ = (profile_begin((char*)name), 0); i##__LINE__ < 1; i##__LINE__ = (profile_end((char*)name), 1))
#define PROFILE_FUNC  auto _profile_##__LINE__ = ProfileScope(__func__)

void profile_begin(char *name);
void profile_end(char *name);

typedef struct ProfileInfo {
    char   *name;
    size_t  name_length;
    uint64  cycle_count;
    uint64  cycle_elapsed;
} ProfileInfo;

global_variable ProfileInfo profile_info[PROFILE_INFO_MAX_SIZE] = {0};
global_variable size_t      profile_info_count = 0;

typedef struct ProfileScope {
    char *name;
    ProfileScope(const char *n) {
        name = (char *)n;
        profile_begin(name);
    }
    ~ProfileScope() {
        profile_end(name);
    }
} ProfileScope;

/*
 * =========================================
 * General Engine Stuff.
 *
 * 特筆することなし。
 * =========================================
 * */

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

// TODO(fuzzy):
// Float as color is a bad idea... seriously.
typedef struct Color {
    real32 r, g, b, a;

    uint32 to_uint32_argb() {
        uint8 u8r, u8g, u8b, u8a;

        u8a = (uint8)(FP_CLAMP(a * 255.0, 0.0, 255.0));
        u8r = (uint8)(FP_CLAMP(r * 255.0, 0.0, 255.0));
        u8g = (uint8)(FP_CLAMP(g * 255.0, 0.0, 255.0));
        u8b = (uint8)(FP_CLAMP(b * 255.0, 0.0, 255.0));

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

typedef struct Property {
    fVector3 position;
    fVector3 rotation; // TODO(fuzzy): Quaternionsの実装
    fVector3 scale;
} Property;

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

typedef struct Engine {
    Arena      core_memory;
    Platform   platform;
    AssetTable asset_table;
} Engine;
#endif
