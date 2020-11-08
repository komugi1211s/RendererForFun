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

// IMPORTANT NOTE(fuzzy): This only works in IEEE-754 floating point format.
// if it does not work, well then good luck.

bool32 load_model_vertices(Model *model, size_t triangle_index, ModelVertex out) {
    if (model->num_indexes < (triangle_index - 1))  return 0;

    iVector3 indexes = model->indexes[triangle_index];
    if ((indexes.x - 1) > model->num_vertices) return 0;
    if ((indexes.y - 1) > model->num_vertices) return 0;
    if ((indexes.z - 1) > model->num_vertices) return 0;

    out[0] = model->vertices[indexes.x-1];
    out[1] = model->vertices[indexes.y-1];
    out[2] = model->vertices[indexes.z-1];
    
    return 1;
}

bool32 parse_obj(char *obj_file, size_t obj_file_length, Model *model) {
    char *start_position = obj_file;
    uint32 current_line = 0;

    model->num_vertices = 0;
    model->num_indexes  = 0;

    size_t maximum_idx = obj_file_length - 1;
    while(*obj_file) {
        switch(*obj_file) {
            // Group head, smooth shading option are
            // turned off right now.
            case 'g':
            case 's':
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
                    TRACE("Expected `usemtl` but did not match line %lu", current_line);
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
                    TRACE("Unexpected EOF while parsing Vertex at line %lu", current_line);
                    return 0;
                }

                if (next_char == 'p' || next_char == 't' || next_char == 'n') {
                    while (*obj_file != '\n' && *obj_file != '\0') {
                        obj_file++;
                    }
                    break;
                }

                if (next_char == ' ') { // valid vertex!
                    obj_file += 2;
                    fVector3 result;
                    ASSERT(model->num_vertices < maximum_idx);

                    char *skipped;
                    result.x = strtof(obj_file, &skipped);
                    if(*skipped != ' ') {
                        TRACE("Expected space separator, got `%c` at line %lu", *skipped, current_line);
                        return 0;
                    }
                    obj_file = ++skipped;

                    result.y = strtof(obj_file, &skipped);
                    if(*skipped != ' ') {
                        TRACE("Expected space separator, got `%c` at line %lu", *skipped, current_line);
                        return 0;
                    }
                    obj_file = ++skipped;

                    result.z = strtof(obj_file, &skipped);
                    obj_file = skipped;

                    model->vertices[model->num_vertices++] = result;
                } else {
                    TRACE("Expected space separator, got `%c` at line %lu", next_char, current_line);
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
                ASSERT(model->num_indexes < maximum_idx);
                char *skipped;
                iVector3 result;

                result.x = strtod(obj_file, &skipped);
                if (*skipped == '/')
                    while(*skipped != ' ') {
                        if (*skipped == '\0') {
                            TRACE("Unexpected EOF at line %lu", current_line);
                            return 0;
                        }
                        skipped++;
                    }

                obj_file = ++skipped;

                result.y = strtod(obj_file, &skipped);
                if (*skipped == '/')
                    while(*skipped != ' ') {
                        if (*skipped == '\0') {
                            TRACE("Unexpected EOF at line %lu", current_line);
                            return 0;
                        }
                        skipped++;
                    }

                obj_file = ++skipped;

                result.z = strtod(obj_file, &skipped);
                while(*skipped != '\n') {
                    skipped++;
                }
                obj_file = skipped;

                model->indexes[model->num_indexes++] = result;
            } break;

            default:
                TRACE("Unexpected Char `%c` at line %lu", *obj_file, current_line);
                return 0;
        }
    }
    return 1;
}

