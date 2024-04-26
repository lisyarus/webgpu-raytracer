#pragma once

#ifdef __APPLE__
#include <SDL.h>
#else
#include <SDL2/SDL.h>
#endif

#include <webgpu.h>

#include <optional>

struct Application
{
    Application();
    ~Application();

    SDL_Window * window() const { return window_; }

    WGPUSurface surface() const { return surface_; }

    WGPUTextureFormat surfaceFormat() const { return surfaceFormat_; }

    WGPUDevice device() const { return device_; }

    WGPUQueue queue() const { return queue_; }

    int width() const { return width_; }
    int height() const { return height_; }

    // Must to be called whenever the window is resized
    void resize(int width, int height, bool vsync);

    // Returns null if the request timed out
    // Throws on error
    // The returned texture view needs to be released after rendering to it
    WGPUTexture nextSwapchainTexture();

    // Must be called after rendering each frame
    void present();

    std::optional<SDL_Event> poll();

private:
    SDL_Window * window_;
    WGPUSurface surface_;
    WGPUTextureFormat surfaceFormat_;
    WGPUDevice device_;
    WGPUQueue queue_;

    int width_ = 1024;
    int height_ = 768;
};
