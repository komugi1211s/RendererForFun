/*
 *
 * the workflow that I want
 *
    Model model;
    if(parse_obj(obj_file, obj_file_length, &model)) {
        iVector3 triangle[3];

        for (int i = 0; i < model.num_triangles; ++i) {
            if(load_model_vertexes(&model, i, &triangle))
                draw_triangle(triangle[0], triangle[1], triangle[2]);
        }
    }
*/

#include "maths.h"
#include "model.h"

bool32 Model::load_face(size_t index, Face *output) {
    ASSERT(this->num_face_indices > index);
    FaceId indexes = this->face_indices[index];

    // NOTE(fuzzy):
    // ここはASSERTで強制失敗するべきか、それともロードできない体を伝えるべきか？
    // モデルの正常なロードに失敗しているのであればAssertで強制失敗させても良い気もする。

    ASSERT((indexes.vertex.x - 1) < this->num_vertices);
    ASSERT((indexes.vertex.y - 1) < this->num_vertices);
    ASSERT((indexes.vertex.z - 1) < this->num_vertices);

    output->vertices[0] = this->vertices[indexes.vertex.x - 1];
    output->vertices[1] = this->vertices[indexes.vertex.y - 1];
    output->vertices[2] = this->vertices[indexes.vertex.z - 1];

    if (indexes.texcoord.x != 0) { // NOTE(fuzzy): 0始まりは有り得ない
        ASSERT((indexes.texcoord.x - 1) < this->num_texcoords);
        ASSERT((indexes.texcoord.y - 1) < this->num_texcoords);
        ASSERT((indexes.texcoord.z - 1) < this->num_texcoords);

        output->texcoords[0] = this->texcoords[indexes.texcoord.x - 1];
        output->texcoords[1] = this->texcoords[indexes.texcoord.y - 1];
        output->texcoords[2] = this->texcoords[indexes.texcoord.z - 1];

        output->has_texcoords = 1;
    }

    if (indexes.normal.x != 0) {
        ASSERT((indexes.normal.x - 1) < this->num_normals);
        ASSERT((indexes.normal.y - 1) < this->num_normals);
        ASSERT((indexes.normal.z - 1) < this->num_normals);

        output->normals[0] = this->normals[indexes.normal.x - 1];
        output->normals[1] = this->normals[indexes.normal.y - 1];
        output->normals[2] = this->normals[indexes.normal.z - 1];

        output->has_normals = 1;
    }

    return 1;
}

char *parse_vertex_definitions(char *start, fVector3 *out, uint32 line_for_error_reporting) {
    ASSERT(out);
    char *stopped_at;
    char *current = start;

    out->x = strtof(current, &stopped_at);
    if (*stopped_at != ' ') {
        TRACE("Unexpected char `%c` at line %u", *stopped_at, line_for_error_reporting);
        return NULL;
    }
    current = (++stopped_at);

    // NOTE(fuzzy):
    // Y座標パース後のキャラクターは'\n'と' 'のどちらもあり得るが、
    // それがValidなのはテクスチャ頂点のみである。
    // 実際にValidな頂点であるかどうかは当関数の外で行うことにする。
    // TODO(fuzzy):
    // もし行末にコメントとかが入っていたらパースに失敗する
    out->y = strtof(current, &stopped_at);
    while (*stopped_at != ' ') {
        if (*stopped_at == '\n') {
            return stopped_at;
        }
        ++stopped_at;
    }
    current = (++stopped_at);

    out->z = strtof(current, &stopped_at);
    while (*stopped_at != '\n') {
        if (*stopped_at == '\0') {
            TRACE("Unexpected EOF at line %u", line_for_error_reporting);
            return NULL;
        }
        ++stopped_at;
    }
    current = stopped_at;
    return current;
}


bool32 parse_obj(char *obj_file, size_t obj_file_length, Model *model) {
    uint32 current_line = 0;

    model->num_vertices = 0;
    model->num_texcoords = 0;
    model->num_normals = 0;
    model->num_face_indices = 0;

    size_t maximum_idx = obj_file_length - 1;
    while(*obj_file) {
        switch(*obj_file) {
            // Group head, smooth shading option are
            // turned off right now.
            case 'g':
            case 's':
            case 'o':
            case 'm':
                while (*obj_file != '\n' && *obj_file != '\0') {
                    obj_file++;
                }
                break;

            // usemtl option is turned off right now.
            case 'u':
            {
                if (strncmp(obj_file, "usemtl", 6) == 0) {
                    while (*obj_file != '\n' && *obj_file != '\0') {
                        obj_file++;
                    }
                    break;
                } else {
                    TRACE("Expected `usemtl` but did not match line %u", current_line);
                    return 0;
                }
            } break;


            case '#':
                while (*obj_file != '\n' && *obj_file != '\0') {
                    obj_file++;
                }
                break;

            case 'v':
            {
                char next_char = obj_file[1];
                if (next_char == '\0') {
                    TRACE("Unexpected EOF while parsing Vertex at line %u", current_line);
                    return 0;
                }

                if (next_char == 'p') {
                    while (*obj_file != '\n' && *obj_file != '\0') {
                        obj_file++;
                    }
                    break;
                }

                fVector3 result = {0};
                fVector3 *out_array = NULL;
                size_t next_position;

                if (next_char == 't') {
                    obj_file += 3; // Skip 'vt ' including spaces.
                    ASSERT(model->num_texcoords+1 < maximum_idx);

                    out_array = model->texcoords;
                    next_position = model->num_texcoords++;
                } else if (next_char == 'n') {
                    obj_file += 3; // Skip 'vn ' including spaces.
                    ASSERT(model->num_normals+1 < maximum_idx);

                    out_array = model->normals;
                    next_position = model->num_normals++;
                } else if (next_char == ' ') {
                    obj_file += 2; // Skip 'v ' including spaces.
                    ASSERT(model->num_vertices+1 < maximum_idx);

                    out_array = model->vertices;
                    next_position = model->num_vertices++;
                } else {
                    TRACE("Unexpected char when parsing v, `%c` at line %u", next_char, current_line);
                    return 0;
                }

                char *updated_obj_pointer = parse_vertex_definitions(obj_file, &result, current_line);
                if (updated_obj_pointer) {
                    out_array[next_position] = result;
                    obj_file = updated_obj_pointer;
                } else {
                    TRACE("Parse vertex defintions failed.");
                    return 0;
                }
            } break;

            case '\t':
            case '\r':
            case ' ':
                obj_file++;
                break;

            case '\n':
                obj_file++;
                current_line++;
                break;

            case 'f':
            {
                obj_file++;
                ASSERT(model->num_face_indices < maximum_idx);

                char *skipped;
                FaceId index_result = {0};
                index_result.vertex.x = strtod(obj_file, &skipped);
                if (*skipped == '/') {
                    obj_file = ++skipped;
                    index_result.texcoord.x = strtod(obj_file, &skipped);

                    if (*skipped == '/') {
                        obj_file = ++skipped;
                        index_result.normal.x = strtod(obj_file, &skipped);
                    }

                    while(*skipped != ' ') {
                        if (*skipped == '\0') {
                            TRACE("Unexpected EOF at line %u", current_line);
                            return 0;
                        }
                        skipped++;
                    }
                }

                obj_file = ++skipped;

                index_result.vertex.y = strtod(obj_file, &skipped);
                if (*skipped == '/') {
                    obj_file = ++skipped;
                    index_result.texcoord.y = strtod(obj_file, &skipped);

                    if (*skipped == '/') {
                        obj_file = ++skipped;
                        index_result.normal.y = strtod(obj_file, &skipped);
                    }

                    while(*skipped != ' ') {
                        if (*skipped == '\0') {
                            TRACE("Unexpected EOF at line %u", current_line);
                            return 0;
                        }
                        skipped++;
                    }
                }
                obj_file = ++skipped;

                index_result.vertex.z = strtod(obj_file, &skipped);
                if (*skipped == '/') {
                    obj_file = ++skipped;
                    index_result.texcoord.z = strtod(obj_file, &skipped);

                    if (*skipped == '/') {
                        obj_file = ++skipped;
                        index_result.normal.z = strtod(obj_file, &skipped);
                    }
                }

                while(*skipped != '\n' && *skipped != '\0') {
                    skipped++;
                }
                obj_file = skipped;

                model->face_indices[model->num_face_indices++] = index_result;
            } break;

            default:
                TRACE("Unexpected Char `%c` at line %u", *obj_file, current_line);
                return 0;
        }
    }
    return 1;
}

