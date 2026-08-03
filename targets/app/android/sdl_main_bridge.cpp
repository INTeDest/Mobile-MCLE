#include <unistd.h>
#include <cstdio>
#include <android/log.h>

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "MAIN", __VA_ARGS__)

extern "C" int main(int argc, const char* argv[]);

extern "C" __attribute__((visibility("default"))) int SDL_main(int argc, char* argv[]) {
    LOGI("SDL_main bridge called!");
    if (chdir("/data/data/x.intedest.mlce/files") != 0) {
        chdir("/data/user/0/x.intedest.mlce/files");
    }
    LOGI("Current working directory set to app files.");

    LOGI("Calling main()...");
    int res = main(argc, (const char**)argv);
    LOGI("main() returned %d", res);
    return res;
}