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
#ifndef _K_ASSET_CPP
#define _K_ASSET_CPP

#include "general.h"
#include "maths.h"
#include "asset.h"

bool32 Model::load_face(size_t index, Face *output) {
    ASSERT(this->face_ids.count > index);
    FaceId indexes = this->face_ids[index];

    // NOTE(fuzzy):
    // ここはASSERTで強制失敗するべきか、それともロードできない体を伝えるべきか？
    // モデルの正常なロードに失敗しているのであればAssertで強制失敗させても良い気もする。

    int32 v_max = FP_MAX_IN_3(indexes.vertex.x, indexes.vertex.y, indexes.vertex.z);
    ASSERT((v_max - 1) < this->vertices.count);

    output->vertices[0] = this->vertices[indexes.vertex.x - 1];
    output->vertices[1] = this->vertices[indexes.vertex.y - 1];
    output->vertices[2] = this->vertices[indexes.vertex.z - 1];

    if (indexes.texcoord.x != 0) { // NOTE(fuzzy): 0始まりは有り得ない
        int32 t_max = FP_MAX_IN_3(indexes.texcoord.x, indexes.texcoord.y, indexes.texcoord.z);
        ASSERT((t_max - 1) < this->texcoords.count);

        output->texcoords[0] = this->texcoords[indexes.texcoord.x - 1];
        output->texcoords[1] = this->texcoords[indexes.texcoord.y - 1];
        output->texcoords[2] = this->texcoords[indexes.texcoord.z - 1];

        output->has_texcoords = 1;
    }

    if (indexes.normal.x != 0) {
        int32 n_max = FP_MAX_IN_3(indexes.normal.x, indexes.normal.y, indexes.normal.z);
        ASSERT((n_max - 1) < this->normals.count);

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

char *eat_until_newline(char *buffer) {
    while (*buffer && *buffer != '\n') buffer++;
    return buffer;
}

ModelInfo do_obj_element_counting(char *obj_file_buffer, size_t file_length) {
    ModelInfo result    = {0};

    char *obj_file      = obj_file_buffer;
    uint32 current_line = 0;

    while(*obj_file) {
        switch(*obj_file) {
            case 'v':
            {
                if     (obj_file[1] == 't') result.texcoord_count += 1;
                else if(obj_file[1] == 'n') result.normal_count   += 1;
                else                        result.vertex_count   += 1;

                obj_file = eat_until_newline(obj_file);
            } break;

            case 'f':
            {
                result.face_count += 1;
                obj_file = eat_until_newline(obj_file);
            } break;

            case '\n':
                obj_file++;
                break;

            default:
                obj_file = eat_until_newline(obj_file);
        }
    }

    return result;
}

bool32 parse_obj_file(Model *workspace, char *obj_file_buffer, size_t file_length) {
    char *obj_file      = obj_file_buffer;
    uint32 current_line = 0;

    while(*obj_file) {
        switch(*obj_file) {
            // Group head, smooth shading option are
            // turned off right now.
            case 'g':
            case 's':
            case 'o':
            case 'm':
                obj_file = eat_until_newline(obj_file);
                break;

            // usemtl option is turned off right now.
            case 'u':
            {
                if (strncmp(obj_file, "usemtl", 6) == 0) {
                    obj_file = eat_until_newline(obj_file);
                    break;
                } else {
                    TRACE("Expected `usemtl` but did not match line %u", current_line);
                    return 0;
                }
            } break;

            case '#':
                obj_file = eat_until_newline(obj_file);
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
                FixedArray<fVector3> *out_array = NULL;

                if (next_char == 't') {
                    obj_file += 3; // Skip 'vt ' including spaces.
                    out_array = &workspace->texcoords;
                } else if (next_char == 'n') {
                    obj_file += 3; // Skip 'vn ' including spaces.
                    out_array = &workspace->normals;
                } else if (next_char == ' ') {
                    obj_file += 2; // Skip 'v ' including spaces.
                    out_array = &workspace->vertices;
                } else {
                    TRACE("Unexpected char when parsing v[?], `%c` at line %u", next_char, current_line);
                    return 0;
                }
                char *updated_obj_pointer = parse_vertex_definitions(obj_file, &result, current_line);
                if (updated_obj_pointer) {
                    ASSERT(out_array->push(result));
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
                ASSERT(workspace->face_ids.push(index_result));
            } break;

            default:
                TRACE("Unexpected Char `%c` at line %u", *obj_file, current_line);
                return 0;
        }
    }
    return 1;
}

bool32 load_model(Engine *engine, char *filename) {
    if(!file_extension_matches(filename, "obj")) return 0;
    // ファイルを読む
    FileObject file_object = engine->platform.open_and_read_file(filename);

    if(file_object.opened) {
        // ファイルが開けたら一時パスとして各エレメントの総数を数える
        ModelInfo model_info = do_obj_element_counting(file_object.content, file_object.size);

        // 専用のアロケーターを準備
        size_t vertex_buffer_size   = (model_info.vertex_count   * sizeof(fVector3));
        size_t texcoord_buffer_size = (model_info.texcoord_count * sizeof(fVector3));
        size_t normal_buffer_size   = (model_info.normal_count   * sizeof(fVector3));
        size_t faceid_buffer_size   = (model_info.face_count     * sizeof(FaceId));
        uint8 *model_buffer = (uint8 *)engine->platform.allocate_memory(vertex_buffer_size + texcoord_buffer_size + normal_buffer_size + faceid_buffer_size);
        if (!model_buffer) {
            engine->platform.deallocate_memory(file_object.content);
            TRACE("failed to allocate %zu memory for models.", vertex_buffer_size + texcoord_buffer_size + normal_buffer_size + faceid_buffer_size);
            return 0;
        }

        Arena model_memory;
        model_memory.initialize(model_buffer, vertex_buffer_size + texcoord_buffer_size + normal_buffer_size + faceid_buffer_size);

        Model model = {0};

        model.memory_arena = model_memory;
        model.vertices.initialize((fVector3*)model_memory.alloc(vertex_buffer_size), model_info.vertex_count);
        model.texcoords.initialize((fVector3*)model_memory.alloc(texcoord_buffer_size), model_info.texcoord_count);
        model.normals.initialize((fVector3*)model_memory.alloc(normal_buffer_size), model_info.normal_count);
        model.face_ids.initialize((FaceId*)model_memory.alloc(faceid_buffer_size), model_info.face_count);

        if(!parse_obj_file(&model, file_object.content, file_object.size)) {
            engine->platform.deallocate_memory(model_memory.data);
            engine->platform.deallocate_memory(file_object.content);
            TRACE("Failed to parse models.");
            return 0;
        }

        if(!engine->asset_table.models.push(model)) {
            engine->platform.deallocate_memory(model_memory.data);
            engine->platform.deallocate_memory(file_object.content);
            TRACE("Failed to push model.");
            return 0;
        }

        return 1;
    }
    TRACE("Could not open a file.");
    return 0;
}


#endif // _K_ASSET_CPP
