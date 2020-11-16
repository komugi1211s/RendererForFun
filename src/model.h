#ifndef _K_MODEL_H
#define _K_MODEL_H

// TODO(fuzzy):
// could cause overflow.
typedef struct FaceId {
    iVector3  vertex;
    iVector3  texcoord;
    iVector3  normal;
} FaceId;

typedef struct Face {
    fVector3 vertices[3];
    fVector3 texcoords[3];
    fVector3 normals[3];

    bool32 has_texcoords;
    bool32 has_normals;
} Face;

typedef struct Model {
    fVector3   *vertices;
    fVector3   *texcoords;
    fVector3   *normals;

    size_t   num_vertices;
    size_t   num_texcoords;
    size_t   num_normals;

    FaceId  *face_indices;
    size_t   num_face_indices;

    bool32   load_face(size_t index, Face *output);
} Model;

typedef struct Texture {
    int32 width;
    int32 height;
    int32 channels;
    uint8 *data;
} Texture;

char *parse_vertex_definitions(char *start, fVector3 *out_vtx);
bool32 parse_obj(char *obj_file, size_t obj_file_length, Model *model);

#endif // _K_MODEL_H
