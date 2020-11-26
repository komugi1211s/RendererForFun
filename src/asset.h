#ifndef _K_ASSET_H
#define _K_ASSET_H

#include "general.h"

typedef struct FileObject FileObject;

// TODO(fuzzy):
// could cause overflow.
typedef struct FaceId {
    iVector3   vertex;
    iVector3   texcoord;
    iVector3   normal;
} FaceId;

typedef struct Model {
    size_t      generation;

    fVector3   *vertices;
    size_t      num_vertices;

    fVector3   *texcoords;
    size_t      num_texcoords;

    fVector3   *normals;
    size_t      num_normals;

    FaceId     *face_indices;
    size_t      num_face_indices;

    bool32      load_face(size_t index, Face *output);
} Model;

typedef struct ObjParser {
    FileObject  *reading_buffer;
    size_t       current_position;
    size_t       current_line;
} ObjPerser;

typedef struct Texture {
    size_t  generation;
    uint8  *data;

    int32   width;
    int32   height;
    int32   channels;
} Texture;

typedef struct ModelArray {
    Model  *models;
    size_t  count;
    size_t  capacity;
    size_t  generation;

    Model   *get(GenId id);
    Model   *alloc();
    bool32   extend(Platfrom *platfrom);
} ModelList;


typedef struct TextureArray {
    Texture  *textures;
    size_t    count;
    size_t    capacity;
    size_t    generation;

    Texture  *get(GenId id);
    Texture  *alloc();
    bool32    extend(Platfrom *platfrom);
} TextureList;


typedef struct AssetTable {
    ModelArray   model_array;
    TextureArray texture_array;
} AssetTable;

typedef struct GenId {
    size_t index;
    size_t generation;
} GenId;


typedef struct ModelParseWorkspace {
    TemporaryMemory vertices;
    TemporaryMemory texcoords;
    TemporaryMemory normals;
    TemporaryMemory face_indices;
} ModelParseWorkspace;

inline bool32
file_extension_matches(char *file_name, const char *expected_extension) {
    char *extension = strrchr(file_name, '.');
    if (extension && extension != file_name) {
        return (strcmp(extension+1, expected_extension) == 0);
    } else {
        return 0;
    }
}

bool32 parse_obj_file(ModelParseWorkspace *workspace, char *obj_file_buffer, size_t file_length);
bool32 load_model(Engine *engine, char *filename);
bool32 load_texture(Engine *engine, char *filename);

#endif // _K_ASSET_H
