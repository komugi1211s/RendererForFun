#include "maths.h"
#include "renderer.h"

/* NOTE(fuzzy):
 *   レンダラのルール
 *    - プラットフォームレイヤーは仕様として「y = 0 == 左上」である
 *    - レンダラはこれに対して「 y = 0 == 左下 」である
 *    - なので現在viewportプロジェクションで上下をひっくり返すようにしている
 *
 * */

void swap_buffer(ScreenBuffer *buffer) {
    uint32 *visible_buffer = buffer->visible_buffer;
    buffer->visible_buffer = buffer->back_buffer;
    buffer->back_buffer = visible_buffer;
}

void clear_buffer(ScreenBuffer *buffer, uint32 clear_mode) {
    uint32 *buf = buffer->back_buffer;
    uint32 total_size = buffer->window_width * buffer->window_height;

    if ((clear_mode & CLEAR_COLOR_BUFFER)) {
        for (int32 i = 0; i < total_size; ++i) {
            buf[i] = 0xFF000000;
        }
    }

    if ((clear_mode & CLEAR_Z_BUFFER)) {
        // これ以上遠ければクリップされる
        real32 z_buffer_furthest = 255.0;
        for (int32 i = 0; i < total_size; ++i) {
            buffer->z_buffer[i] = z_buffer_furthest;
        }
    }
}

void draw_line(ScreenBuffer *buffer, int32 x0, int32 y0, int32 x1, int32 y1, Color color) {
    x0 = FP_MAX(0, FP_MIN(x0, buffer->window_width));
    x1 = FP_MAX(0, FP_MIN(x1, buffer->window_width));
    y0 = FP_MAX(0, FP_MIN(y0, buffer->window_height - 1));
    y1 = FP_MAX(0, FP_MIN(y1, buffer->window_height - 1));

    uint32 *buf = buffer->back_buffer;

    int32 x_start, x_end, y_start, y_end;
    bool32 transpose = abs(x1 - x0) < abs(y1 - y0);
    x_start = transpose ? y0 : x0;
    y_start = transpose ? x0 : y0;
    x_end   = transpose ? y1 : x1;
    y_end   = transpose ? x1 : y1;

    if (x_start > x_end) {
        SWAPVAR(int32, x_start, x_end);
        SWAPVAR(int32, y_start, y_end);
    }

    real32 delta = (real32)(x_end - x_start);
    ASSERT(delta >= 0);
    bool32 parallel_to_axis = y_end == y_start;

    uint32 c = color.to_uint32_argb();

    // NOTE(fuzzy): Testing optimization.
    if (parallel_to_axis) {
        for (int32 t = x_start; t <= x_end; ++t) {
            int32 x = t;
            int32 y = y_start;
            if (transpose) {
                x = y;
                y = t;
            }
            buf[(y * buffer->window_width) + x] = c;
        }
    } else {
        for (int32 t = x_start; t <= x_end; ++t) {
            real32 dt = (t - x_start) / delta;
            int32 x = t;
            int32 y = ((1.0 - dt) * y_start + dt * y_end);
            if (transpose) {
                x = y;
                y = t;
            }
            buf[(y * buffer->window_width) + x] = c;
        }
    }
}

void DEBUG_render_z_buffer(ScreenBuffer *buffer) {
    uint32 *buf = buffer->back_buffer;
    for (int32 y = 0; y < buffer->window_height; ++y) {
        for(int32 x = 0; x < buffer->window_width; ++x) {
            int32 z_buffer_position = x + y * buffer->window_width;
            uint8 z_val = static_cast<uint8>(ceil(FP_CLAMP(buffer->z_buffer[z_buffer_position], 0.0, 254.0)));

            uint32 result = (z_val << 16 | z_val << 8 | z_val);
            buf[z_buffer_position] = result;
        }
    }
}

internal inline void
draw_bounding_box(ScreenBuffer *buffer, BoundingBoxi2 box, Color color) {

    // from top left, goes right / bottom side
    draw_line(buffer, box.min_v.x, box.min_v.y, box.min_v.x, box.max_v.y, color);
    draw_line(buffer, box.min_v.x, box.min_v.y, box.max_v.x, box.min_v.y, color);

    // from bottom left, goes right side
    draw_line(buffer, box.min_v.x, box.max_v.y, box.max_v.x, box.max_v.y, color);

    // from top right,   goes bottom side
    draw_line(buffer, box.max_v.x, box.min_v.y, box.max_v.x, box.max_v.y, color);

}

// NOTE(fuzzy):
// 三角形の重心の計算については以下を参照
// 参考: https://shikihuiku.wordpress.com/2017/05/23/barycentric-coordinates%E3%81%AE%E8%A8%88%E7%AE%97%E3%81%A8perspective-correction-partial-derivative%E3%81%AB%E3%81%A4%E3%81%84%E3%81%A6/
// ryg先生の説明 https://fgiesen.wordpress.com/2013/02/06/the-barycentric-conspirac/

void draw_filled_triangle(ScreenBuffer *buffer, fVector3 vertices[3], Color color) {
    uint32 *buf = buffer->back_buffer;
    BoundingBoxf3 bd_box = BB_fV3(vertices[0], vertices[1], vertices[2]);
    uint32 c = color.to_uint32_argb();
    
    real32 det_t = fdeterminant_triangle_fv3(vertices[0], vertices[1], vertices[2]);

    bool32 triangle_is_degenerate = fabsf(det_t) < 1;

    if (!triangle_is_degenerate) {
        int32 y_min = (int32)FP_MAX(floor(bd_box.min_v.y), 0);
        int32 y_max = (int32)FP_MIN(ceil(bd_box.max_v.y), buffer->window_height-1);
        int32 x_min = (int32)FP_MAX(floor(bd_box.min_v.x), 0);
        int32 x_max = (int32)FP_MIN(ceil(bd_box.max_v.x), buffer->window_width-1);
        fVector3 P;
        real32 weight1, weight2, weight3;

        for(int32 y = y_min; y < y_max; ++y) {
            P.y = y;
            for(int32 x = x_min; x < x_max; ++x) {
                P.x = x;
                weight1 = fdeterminant_triangle_fv3(vertices[1], vertices[2], P) / det_t;
                weight2 = fdeterminant_triangle_fv3(vertices[2], vertices[0], P) / det_t;
                weight3 = fdeterminant_triangle_fv3(vertices[0], vertices[1], P) / det_t;

                // (1 - x - y)A + xB + yC = P' なので x+y+(1-x-y) = 1 とならなければならず
                // どれか1つの点が負数の場合三角形の外に点があることになる
                if (weight1 < 0 || weight2 < 0 || weight3 < 0) continue;

                // (1 - x - y)A + xB + yC として定義してあるので
                // 掛け算もそれに合わせて揃えないといけない
                real32 z_value = (vertices[0].z * weight1) + (vertices[1].z * weight2) + (vertices[2].z * weight3);
                int32 position = (y * buffer->window_width) + x;

                if (position > (buffer->window_width * buffer->window_height)) continue;
                if (position < 0) continue;

                if (buffer->z_buffer[position] > z_value) {
                    buffer->z_buffer[position] = z_value;
                    buf[position] = c;
                }
            }
        }
    }
}

// draw_filled_triangleとほとんど同じ
void draw_textured_triangle(ScreenBuffer *buffer,
                            fVector3 vertices[3],
                            fVector3 texcoords[3],
                            fVector3 light_intensity, // TODO(fuzzy): Technical Debt.
                            Texture *texture)
{
    PROFILE_FUNC
    uint32 *buf = buffer->back_buffer;
    BoundingBoxf3 bd_box = BB_fV3(vertices[0], vertices[1], vertices[2]);
    // NOTE(fuzzy):
    // 数学を忘れないためにイチイチメモをする:
    //
    // det_T は三角形ABCの辺AB、ACから構成した行列式（三角形の面積もどき）で
    // 本来の三角形の二倍の面積を持っている（なので実際はdet_T/2が実際の三角形の面積）

    // NOTE(fuzzy):
    // det_Tが 1 より小さい場合（本来ならEPSとかと比べるべき）
    // その三角形は線形的に独立していない(Linearly dependant)とされる
    // これはつまり三角形がxy軸のうちどちらかに対して面のように平べったく広がっている/もしくは全部の点が全て同じ位置にある可能性がある
    // これが満たされる場合はその三角形を書くのも無駄なので（ほぼ線みたいになってるので書けないと思う）
    // 完全に無視する
    real32 det_t = fdeterminant_triangle_fv3(vertices[0], vertices[1], vertices[2]);

    bool32 triangle_is_degenerate = fabsf(det_t) < 1;
    if (!triangle_is_degenerate) {
        int32 y_min = (int32)FP_MAX(floor(bd_box.min_v.y), 0);
        int32 y_max = (int32)FP_MIN(ceil(bd_box.max_v.y), buffer->window_height-1);
        int32 x_min = (int32)FP_MAX(floor(bd_box.min_v.x), 0);
        int32 x_max = (int32)FP_MIN(ceil(bd_box.max_v.x), buffer->window_width-1);
        fVector3 P;

        real32 weight1, weight2, weight3;
        for(int32 y = y_min; y < y_max; ++y) {
            P.y = y;
            for(int32 x = x_min; x < x_max; ++x) {
                P.x = x;
                int32 position = (y * buffer->window_width) + x;

                weight1 = fdeterminant_triangle_fv3(vertices[1], vertices[2], P) / det_t;
                weight2 = fdeterminant_triangle_fv3(vertices[2], vertices[0], P) / det_t;
                weight3 = fdeterminant_triangle_fv3(vertices[0], vertices[1], P) / det_t;

                if (weight1 < 0|| weight2 < 0 || weight3 < 0) continue;

                real32 z_value = (vertices[0].z  * weight1) + (vertices[1].z  * weight2) + (vertices[2].z  * weight3);
                // TODO(fuzzy): ここはシェーダーの仕事
                real32 t_x     = (texcoords[0].x * weight1) + (texcoords[1].x * weight2) + (texcoords[2].x * weight3);
                real32 t_y     = (texcoords[0].y * weight1) + (texcoords[1].y * weight2) + (texcoords[2].y * weight3);

                real32 light = (light_intensity.x * weight1)
                               + (light_intensity.y * weight2)
                               + (light_intensity.z * weight3);

                int32 tex_x = t_x * texture->width;
                int32 tex_y = texture->height - (t_y * texture->height);
                int32 tex_p = (int32)(tex_x + (tex_y * texture->width)) * texture->channels;

                uint32 r, g, b;
                r = texture->data[tex_p]     * light;
                g = texture->data[tex_p + 1] * light;
                b = texture->data[tex_p + 2] * light;

                if (buffer->z_buffer[position] > z_value) {
                    buffer->z_buffer[position] = z_value;
                    buf[position] = (uint32) (r << 16 | g << 8 | b);
                }
            }
        }
    }
}

void draw_wire_triangle(ScreenBuffer *buffer, fVector3 vertices[3], Color color) {
    PROFILE_FUNC
    real32 det_T = fdeterminant_triangle_fv3(vertices[0], vertices[1], vertices[2]);
    bool32 triangle_is_degenerate = fabsf(det_T) < 1;

    if (!triangle_is_degenerate) {
        draw_line(buffer, floor(vertices[0].x), floor(vertices[0].y), ceil(vertices[1].x), ceil(vertices[1].y), color);
        draw_line(buffer, floor(vertices[0].x), floor(vertices[0].y), ceil(vertices[2].x), ceil(vertices[2].y), color);
        draw_line(buffer, floor(vertices[1].x), floor(vertices[1].y), ceil(vertices[2].x), ceil(vertices[2].y), color);
    }
}

void project_viewport(fVector3 world_space[3],
                      int32 window_width,
                      int32 window_height)
{
    const real32 arbitary_depth = 255;

    for (int i = 0; i < 3; ++i) {
        world_space[i].x = ((1.0 + world_space[i].x) * 0.5) * window_width;
        world_space[i].y = window_height - (((1.0 + world_space[i].y) * 0.5) * window_height); // NOTE(fuzzy): Yは上！！！
        world_space[i].z = ((1.0 + world_space[i].z) * 0.5) * arbitary_depth;
    }
}

void draw_filled_rectangle(ScreenBuffer *buffer, int32 x0, int32 y0, int32 x1, int32 y1, Color color) {
    uint32 *buf = buffer->back_buffer;

    if (x0 < 0 || y0 < 0) return;
    if (x1 > buffer->window_width || y1 > buffer->window_height) return;

    for (int32 y = y0; y < y1; ++y) {
        for (int32 x = x0; x < x1; ++x) {
            int32 position = (y * buffer->window_width) + x;
            uint32 current_color = buf[position];
            Color c = Color::from_uint32_argb(current_color);
            c = color.blend(c);
            buf[position] = c.to_uint32_argb();
        }
    }
}

void draw_filled_model(ScreenBuffer *buffer, Model *model, Camera *camera, Property *property, Color color) {
    PROFILE_FUNC;
    fVector3 AB, AC;
    fVector3 surface_normal;

    fMat4x4 view = fMat4Lookat(camera->position, camera->target, camera->up);
    fMat4x4 projection = fMat4Perspective(camera->aspect_ratio, camera->fov, camera->z_near, camera->z_far);

    fMat4x4 mvp = fmul_fmat4x4(projection, view);
    for (size_t i = 0; i < model->num_face_indices; ++i) {
        // TODO(fuzzy): Face構造体そのもののポインターをロードできるのでは？
        Face face = {0};
        bool32 face_loaded = model->load_face(i, &face);

        if (face_loaded) {
            for (int v = 0; v < 3; ++v) {
                fVector4 fv4vert;
                fv4vert.xyz = face.vertices[v];
                fv4vert.w  = 1.0;
                fv4vert = fmul_fmat4x4_fv4(mvp, fv4vert);

                if ((fv4vert.w < 0) || (fv4vert.z > fv4vert.w) || (fv4vert.z < -fv4vert.w)) {
                    // TODO(fuzzy):
                    // 本来ならココでfor文を使わなければ避けられるGOTOなので
                    // 時間がある時に構造考え直すように
                    goto do_not_render_and_go_next_triangles_filled;
                }

                face.vertices[v].x = fv4vert.x / fv4vert.w;
                face.vertices[v].y = fv4vert.y / fv4vert.w;
                face.vertices[v].z = fv4vert.z / fv4vert.w;
            }
            AB = fsub_fv3(face.vertices[1], face.vertices[0]);
            AC = fsub_fv3(face.vertices[2], face.vertices[0]);

            // Back-face Culling
            // 既に頂点はView Spaceにある為、-V0.N < 0 を計算する
            // この際外積を(v2-v0)(v1-v0) と置いているので符号が違う。そのため0より小さいかどうか調べること
            surface_normal = fnormalize_fv3(fcross_fv3(AC, AB));
            if(-fdot_fv3(face.vertices[0], surface_normal) < 0) continue;

            project_viewport(face.vertices, buffer->window_width,
                                            buffer->window_height);

            draw_filled_triangle(buffer, face.vertices, color);
        }

do_not_render_and_go_next_triangles_filled:;
    }
}

void draw_textured_model(ScreenBuffer *buffer, Model *model, Camera *camera, Property *property, Texture *texture) {
    PROFILE_FUNC;
    fVector3 AB, AC;
    fVector3 surface_normal;
    real32 culling_result;
    fMat4x4 view = fMat4Lookat(camera->position, camera->target, camera->up);
    fMat4x4 projection = fMat4Perspective(camera->aspect_ratio, camera->fov, camera->z_near, camera->z_far);

    fMat4x4 mvp = fmul_fmat4x4(projection, view);
    for (size_t i = 0; i < model->num_face_indices; ++i) {
        Face face = {0};
        fVector3 light = { 1.0, 1.0, 1.0 };
        bool32 face_loaded = model->load_face(i, &face);
        if (face_loaded) {
            ASSERT(face.has_texcoords);
            for (int v = 0; v < 3; ++v) {
                // TODO(fuzzy): シェーダーの仕事
                fVector4 fv4vert;
                fv4vert.xyz = face.vertices[v];
                fv4vert.w = 1.0;
                fv4vert = fmul_fmat4x4_fv4(mvp, fv4vert);

                if ((fv4vert.w < 0) || (fv4vert.z > fv4vert.w) || (fv4vert.z < -fv4vert.w)) {
                    // TODO(fuzzy):
                    // 本来ならココでfor文を使わなければ避けられるGOTOなので
                    // 時間がある時に構造考え直すように
                    goto do_not_render_and_go_next_triangles_tex;
                } else {
                    face.vertices[v].x = fv4vert.x / fv4vert.w;
                    face.vertices[v].y = fv4vert.y / fv4vert.w;
                    face.vertices[v].z = fv4vert.z / fv4vert.w;
                }

            }
            AB = fsub_fv3(face.vertices[1], face.vertices[0]);
            AC = fsub_fv3(face.vertices[2], face.vertices[0]);

            // Back-face Culling
            // 既に頂点はView Spaceにある為、-V0.N >= 0 を計算する
            surface_normal = fnormalize_fv3(fcross_fv3(AC, AB));
            culling_result = -fdot_fv3(face.vertices[0], surface_normal);
            if(culling_result < 0) {
                continue;
            }

            if (face.has_normals) {
                light.x = FP_CLAMP(fdot_fv3(face.vertices[0], face.normals[0]), 0.0, 1.0);
                light.y = FP_CLAMP(fdot_fv3(face.vertices[1], face.normals[1]), 0.0, 1.0);
                light.z = FP_CLAMP(fdot_fv3(face.vertices[2], face.normals[2]), 0.0, 1.0);
            }

            project_viewport(face.vertices, buffer->window_width,
                                            buffer->window_height);

            draw_textured_triangle(buffer, face.vertices, face.texcoords, light, texture);

            // Triangleを構成するVertexのうち、どれか１つのZの位置がレンダリングできない範囲内にあるので
            // ここに飛んでTriangle自体のレンダリングを避ける
            // TODO(fuzzy):
            // 本来なら -1.0 < z < 1.0 の状況下でクリップが行われる際、
            // その三角形全体をクリップするのではなく、本来ならクリップされる位置を補う形の頂点を生成して
            // スムーズなクリッピングを実現するべき
            // どうやって調べれば情報が出てくるのか全然わからない…
            do_not_render_and_go_next_triangles_tex:;
        }
    }
}

void draw_wire_model(ScreenBuffer *buffer, Model *model, Camera *camera, Property *property, Color color) {
    PROFILE_FUNC;
    fMat4x4 view = fMat4Lookat(camera->position, camera->target, camera->up);
    fMat4x4 projection = fMat4Perspective(camera->aspect_ratio, camera->fov, camera->z_near, camera->z_far);

    fMat4x4 mvp = fmul_fmat4x4(projection, view);
    for (size_t i = 0; i < model->num_face_indices; ++i) {
        Face face = {0};
        bool32 face_loaded = model->load_face(i, &face);
        if (face_loaded) {
            for (int v = 0; v < 3; ++v) {
                fVector4 fv4vert;
                fv4vert.xyz = face.vertices[v];
                fv4vert.w = 1.0;
                fv4vert = fmul_fmat4x4_fv4(mvp, fv4vert);

                if ((fv4vert.w < 0) || (fv4vert.z > fv4vert.w) || (fv4vert.z < -fv4vert.w)) {
                    // TODO(fuzzy):
                    // 本来ならココでfor文を使わなければ避けられるGOTOなので
                    // 時間がある時に構造考え直すように
                    goto do_not_render_and_go_next_triangles_wireframe;
                }

               face.vertices[v].x = fv4vert.x / fv4vert.w;
               face.vertices[v].y = fv4vert.y / fv4vert.w;
               face.vertices[v].z = fv4vert.z / fv4vert.w;
            }

            project_viewport(face.vertices, buffer->window_width,
                                            buffer->window_height);

            draw_wire_triangle(buffer, face.vertices, color);
        } else {
            TRACE("failed to load model");
        }

        do_not_render_and_go_next_triangles_wireframe:;
    }
}

void draw_text(ScreenBuffer *buffer, FontData *font, int32 start_x, int32 start_y, char *text) {
    PROFILE_FUNC;

    int32 x = start_x;
    int32 y = start_y + (font->base_line + font->line_gap);
    uint32 *buf = buffer->back_buffer;

    for (char *t = text; *t; ++t) {
        if (*t == '\n') {
            y += (font->base_line + font->line_gap);
            continue;
        }
        char character = *t;
        int32 advance = 0;
        int32 lsb = 0;

        int32 x0, x1, y0, y1;
        stbtt_GetCodepointHMetrics(&font->font_info, character, &advance, &lsb);
        stbtt_GetCodepointBitmapBox(&font->font_info, character, font->char_scale, font->char_scale,
                                    &x0, &y0, &x1, &y1);

        DEBUGCharacterBitmap *bitmap;
        if (!bitmap_array[character].allocated) {
            DEBUGCharacterBitmap bitmap;
            bitmap.allocated = stbtt_GetCodepointBitmap(&font->font_info, font->char_scale, font->char_scale,
                                                        character, &bitmap.width, &bitmap.height, &bitmap.x_off, &bitmap.y_off);
            if (bitmap.allocated) {
                bitmap_array[character] = bitmap;
            } else {
                bitmap_array[character].allocated = NULL;
                bitmap_array[character].width  = -1;
                bitmap_array[character].height = -1;
            }
        }
        bitmap = &bitmap_array[character];

        int32 position = ((x + x0) + (y + y0) * buffer->window_width);

        if (bitmap->width < 0) {
            x += advance * font->char_scale;
            continue;
        }

        for (int i = 0; i < (y1 - y0); ++i) {
            for (int j = 0; j < (x1 - x0); ++j) {
                uint8 color = bitmap->allocated[j + (i * bitmap->width)];
                if (color > 0) {
                    buf[position + j + i * buffer->window_width] = (color << 24 | color << 16 | color << 8 | color);
                }
            }
        }

        x += advance * font->char_scale;
    }
}
