#ifndef _K_ENGINE_CPP
#define _K_ENGINE_CPP

#include "engine.h"
/*
 * =========================================
 * Profiler Stuff.
 * =========================================
 * */

void profile_begin(char *name) {
    size_t length = strlen(name);
    ProfileInfo *info = NULL;
    for (int32 i = 0; i < profile_info_count; ++i) {
        if (profile_info[i].name_length == length) {
            if (strcmp(profile_info[i].name, name) == 0) {
                info = &profile_info[i];
                break;
            }
        }
    }

    if (!info) {
        if((profile_info_count + 1) < PROFILE_INFO_MAX_SIZE) {
            size_t new_index = profile_info_count++;
            info = (profile_info + new_index);

            info->name          = name;
            info->name_length   = length;
            info->cycle_elapsed = 0;
            info->cycle_count   = __rdtsc();
        } else {
            return;
        }
    }
    return;
}

void profile_end(char *name) {
    uint64 end = __rdtsc();
    size_t length = strlen(name);
    ProfileInfo *info = NULL;

    for (int32 i = 0; i < profile_info_count; ++i) {
        if (profile_info[i].name_length == length) {
            if (strcmp(profile_info[i].name, name) == 0) {
                info = &profile_info[i];
                break;
            }
        }
    }

    if (info) {
        info->cycle_elapsed = end - info->cycle_count;
        info->cycle_count   = end;
    }
}

#endif // _K_ENGINE_CPP
