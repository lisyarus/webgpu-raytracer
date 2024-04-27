#include <webgpu-raytracer/application.hpp>
#include <webgpu-raytracer/gltf_loader.hpp>
#include <webgpu-raytracer/scene_data.hpp>
#include <webgpu-raytracer/camera.hpp>
#include <webgpu-raytracer/shader_registry.hpp>
#include <webgpu-raytracer/renderer.hpp>

#include <iostream>
#include <unordered_set>
#include <chrono>

static std::filesystem::path const projectRoot = PROJECT_ROOT;

int main(int argc, char ** argv) try
{
    if (argc != 2)
    {
        std::cout << "Usage: " << argv[0] << " <gltf-file>\n";
        return 0;
    }

    Application application;
    ShaderRegistry shaderRegistry(projectRoot / "shaders", application.device());
    Renderer renderer(application.device(), application.queue(), application.surfaceFormat(), shaderRegistry);

    auto assetPath = std::filesystem::path(argv[1]);
    auto asset = glTF::load(assetPath);
    std::cout << "Loaded asset " << assetPath << '\n';

    std::vector<std::uint32_t> cameraNodes;
    for (std::uint32_t i = 0; i < asset.nodes.size(); ++i)
        if (asset.nodes[i].camera)
            cameraNodes.push_back(i);

    Camera camera;
    if (!cameraNodes.empty())
        camera = Camera(asset, asset.nodes[cameraNodes.front()]);
    camera.setAspectRatio(application.width() * 1.f / application.height());

    SceneData sceneData(asset, application.device(), application.queue());

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
                camera.setAspectRatio(application.width() * 1.f / application.height());
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

        renderer.renderFrame(surfaceTexture, camera, sceneData);
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
