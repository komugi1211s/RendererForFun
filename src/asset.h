

typedef struct Model {
    fVector3   *vertices;
    fVector3   *texcoords;
    fVector3   *normals;
    FaceId     *face_indices;

    size_t   num_vertices;
    size_t   num_texcoords;
    size_t   num_normals;
    size_t   num_face_indices;

    bool32   load_face(size_t index, Face *output);
} Model;

typedef struct Texture {
    int32 width;
    int32 height;
    int32 channels;
    uint8 *data;
} Texture;

typedef struct ModelList {
    Model *models;
    size_t count;
    size_t capacity;
} ModelList;

typedef struct TextureList {
    Texture *textures;
    size_t count;
    size_t capacity;
} TextureList;

typedef struct AssetTable {
    ModelList   model_list;
    TextureList texture_list;
} AssetTable;
