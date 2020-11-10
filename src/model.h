#ifndef _K_MODEL_H
#define _K_MODEL_H

typedef struct ModelIndex {
    iVector3  vert_idx;
    iVector3  tex_idx;
    // iVector3  normal_idx;
} ModelIndex;

typedef struct Model {
    fVector3   *vertices;
    fVector3   *texcoords;

    ModelIndex *indexes;

    size_t   num_vertices;
    size_t   num_texcoords;

    size_t   num_indexes;
} Model;

typedef struct Texture {
    int32 width;
    int32 height;
    int32 channels;
    uint8 *data;
} Texture;

typedef  fVector3 ModelVertex[3]; // subject to change.

bool32 load_model_triangles(Model *model, size_t triangle_index, fVector3 *out_vert);
bool32 load_model_texcoords(Model *model, size_t triangle_index,   fVector3 *out_tex);
bool32 parse_obj(char *obj_file, size_t obj_file_length, Model *model);

#endif // _K_MODEL_H
