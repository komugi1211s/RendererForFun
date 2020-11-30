#ifndef _K_ASSET_H
#define _K_ASSET_H

#include "memory.h"
#include "general.h"

typedef struct FileObject FileObject;
typedef struct Engine Engine;

typedef struct GenId {
    size_t index;
    size_t generation;
} GenId;

typedef struct Face {
    fVector3 vertices[3];
    fVector3 texcoords[3];
    fVector3 normals[3];

    bool32 has_texcoords;
    bool32 has_normals;
} Face;

// TODO(fuzzy):
// could cause overflow.
typedef struct FaceId {
    iVector3   vertex;
    iVector3   texcoord;
    iVector3   normal;
} FaceId;

typedef struct ModelInfo {
    size_t vertex_count;
    size_t texcoord_count;
    size_t normal_count;
    size_t face_count;
} ModelInfo;

typedef struct Model {
    // NOTE(fuzzy): 下記FixedArray達はすべてこのmemoryから派生させる
    Arena memory_arena;

    FixedArray<fVector3> vertices;
    FixedArray<fVector3> texcoords;
    FixedArray<fVector3> normals;
    FixedArray<FaceId>   face_ids;

    bool32               load_face(size_t index, Face *output);
} Model;

typedef struct Texture {
    uint8  *data;

    int32   width;
    int32   height;
    int32   channels;
} Texture;

typedef struct AssetTable {
    FixedArray<Model>   models;
    FixedArray<Texture> textures;
} AssetTable;

inline bool32
file_extension_matches(char *file_name, const char *expected_extension) {
    char *extension = strrchr(file_name, '.');
    if (extension && extension != file_name) {
        return (strcmp(extension+1, expected_extension) == 0);
    } else {
        return 0;
    }
}

ModelInfo do_obj_element_counting(char *obj_file_buffer, size_t file_length);

bool32 parse_obj_file(Model *result, char *obj_file_buffer, size_t file_length);
bool32 load_model(Engine *engine, char *filename);
bool32 load_texture(Engine *engine, char *filename);

// TODO(fuzzy): 
// free A MODEL / A TEXTURE function, not this kind of bulk stuff.
void free_models(Engine *engine);
void free_textures(Engine *engine);

#endif // _K_ASSET_H
