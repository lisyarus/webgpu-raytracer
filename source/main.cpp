#include <webgpu-raytracer/application.hpp>

#include <iostream>
#include <unordered_set>
#include <chrono>

int main(int argc, char ** argv) try
{
    Application application;

    std::unordered_set<SDL_Scancode> keysDown;

    int frameId = 0;
    double time = 0.0;

    auto lastFrameStart = std::chrono::high_resolution_clock::now();

    for (bool running = true; running;)
    {
        bool resized = false;

        while (auto event = application.poll()) switch (event->type)
        {
        case SDL_QUIT:
            running = false;
            break;
        case SDL_WINDOWEVENT:
            switch (event->window.event)
            {
            case SDL_WINDOWEVENT_RESIZED:
                application.resize(event->window.data1, event->window.data2, false);
                resized = true;
                break;
            }
            break;
        case SDL_MOUSEMOTION:
            break;
        case SDL_KEYDOWN:
            keysDown.insert(event->key.keysym.scancode);
            break;
        case SDL_KEYUP:
            keysDown.erase(event->key.keysym.scancode);
            break;
        }

        auto surfaceTexture = application.nextSwapchainTexture();
        if (!surfaceTexture)
        {
            ++frameId;
            continue;
        }

        auto thisFrameStart = std::chrono::high_resolution_clock::now();
        float const dt = std::chrono::duration_cast<std::chrono::duration<float>>(thisFrameStart - lastFrameStart).count();
        lastFrameStart = thisFrameStart;

        application.present();

        wgpuTextureRelease(surfaceTexture);

        ++frameId;
    }
}
catch (std::exception const & e)
{
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
}
