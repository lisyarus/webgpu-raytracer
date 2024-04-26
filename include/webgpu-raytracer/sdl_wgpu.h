#pragma once

#ifdef __APPLE__
#include <SDL_video.h>
#else
#include <SDL2/SDL_video.h>
#endif
#include <webgpu.h>

#ifdef __cplusplus
extern "C" {
#endif

WGPUSurface SDL_WGPU_CreateSurface(WGPUInstance instance, SDL_Window * window);

#ifdef __cplusplus
}
#endif
