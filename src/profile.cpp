/*
usage:
    PROFILE(name) {
        some_expensive_code();
    }

    void target_function() {
        PROFILE_FUNC_START
        ...
        PROFILE_FUNC_END
    }

*/

#include "profile.h"

void profile_begin(const char *name) {
    size_t length = strlen(name);
    ProfileInfo *info = 0;
    for (int32 i = 0; i < profile_info_count; ++i) {
        if (strncmp(profile_info[i].name, name, length) == 0) {
            info = &profile_info[i];
            break;
        }
    }

    if (!info) {
        if((profile_info_count + 1) < PROFILE_INFO_MAX_SIZE) {
            size_t new_index = profile_info_count++;
            info = (profile_info + new_index);
            info->name = name;
            info->cycle_count = __rdtsc();
        } else {
            return;
        }
    }

    info->is_started = 1;
    return;
}

void profile_end(const char *name) {
    uint64 end = __rdtsc();
    size_t length = strlen(name);
    ProfileInfo *info = NULL;

    for (int32 i = 0; i < profile_info_count; ++i) {
        if (strncmp(profile_info[i].name, name, length) == 0) {
            info = (profile_info + i);
            break;
        }
    }

    if (info) {
        info->is_started = 0;
        info->cycle_elapsed = end - info->cycle_count;
        info->cycle_count = end;
    }
}
