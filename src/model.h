#ifndef _K_MODEL_H
#define _K_MODEL_H

typedef struct Model {
    fVector3 *vertices;
    fVector3 *texcoords;
    iVector3 *vert_idx;
    iVector3 *tex_idx;

    size_t   num_vertices;
    size_t   num_texcoords;
    size_t   num_vert_idx;
    size_t   num_tex_idx;
} Model;

typedef struct Texture {

} Texture;

typedef  fVector3 ModelVertex[3]; // subject to change.

void   free_model(Model *model);
bool32 load_model_vertices(Model *model, size_t triangle_index, fVector3 *out);
bool32 parse_obj(char *obj_file, size_t obj_file_length, Model *model);

#ifdef STB_IMAGE_IMPLEMENTATION
bool32 parse_texture(char *tex_file, size_t tex_file_length, Model *model);
#endif // STB_IMAGE_IMPLEMENTATION

#endif // _K_MODEL_H
