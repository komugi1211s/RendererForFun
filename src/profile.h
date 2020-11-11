
#ifndef _K_PROFILE_H
#define _K_PROFILE_H

#ifdef _MSC_VER
#include <intrin.h>
#else
#include <x86intrin.h>
#endif

#define PROFILE(name) for(int i##__LINE__ = (profile_begin(name), 0); i##__LINE__ < 1; i##__LINE__ = (profile_end(name), 1))

#define PROFILE_INFO_MAX_SIZE 255

typedef struct ProfileInfo {
    const char  *name;
    uint64 cycle_count;
    uint64 cycle_elapsed;
    bool32 is_started;
} ProfileInfo;

global_variable ProfileInfo profile_info[PROFILE_INFO_MAX_SIZE] = {0};
global_variable size_t      profile_info_count = 0;

void profile_begin(const char *name);
void profile_end(const char *name);

typedef struct ProfileScope {
    const char *name;

    ProfileScope(const char *n): name(n) {
        profile_begin(n);
    }
    ~ProfileScope() {
        profile_end(name);
    }
} ProfileScope;

#define PROFILE_FUNC          ProfileScope _profile_##__LINE__(__func__);

#endif // _K_PROFILE_H
