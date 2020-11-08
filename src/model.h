#ifndef _K_MODEL_H
#define _K_MODEL_H

typedef struct Model {
    fVector3 *vertices;
    iVector3 *indexes;

    size_t   num_vertices;
    size_t   num_indexes;
} Model;

typedef  fVector3 ModelVertex[3];

void   free_model(Model *model);
bool32 load_model_vertices(Model *model, size_t triangle_index, fVector3 *out);
bool32 parse_obj(char *obj_file, size_t obj_file_length, Model *model);

#endif
