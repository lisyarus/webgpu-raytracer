#include <webgpu-raytracer/application.hpp>
#include <webgpu-raytracer/gltf_loader.hpp>
#include <webgpu-raytracer/scene_data.hpp>
#include <webgpu-raytracer/camera.hpp>
#include <webgpu-raytracer/shader_registry.hpp>
#include <webgpu-raytracer/renderer.hpp>
#include <stb_image.h>

#include <iostream>
#include <sstream>
#include <unordered_set>
#include <chrono>

static std::filesystem::path const projectRoot = PROJECT_ROOT;

int main(int argc, char ** argv) try
{
    if ((argc != 2 && argc != 3) || (argc == 2 && (argv[1] == std::string("-h") || argv[1] == std::string("--help"))))
    {
        std::cout << "Usage: " << argv[0] << " input [ background ]\n";
        std::cout << "    input        Path to a glTF file with the input scene\n";
        std::cout << "    background   Background emission color in R,G,B format (black \"0,0,0\" by default)\n";
        std::cout << "                 or path to an HDRI environment map\n";
        return 0;
    }

    Application application;
    ShaderRegistry shaderRegistry(projectRoot / "shaders", application.device());
    Renderer renderer(application.device(), application.queue(), application.surfaceFormat(), shaderRegistry);

    auto assetPath = std::filesystem::path(argv[1]);
    auto asset = glTF::load(assetPath);
    std::cout << "Loaded asset " << assetPath << '\n';

    HDRIData environmentMap
    {
        .width = 1,
        .height = 1,
        .pixels = {0.f, 0.f, 0.f, 0.f},
    };

    if (argc >= 3)
    {
        if (std::filesystem::exists(argv[2]))
        {
            // Try to parse an HDRI
            int width, height, channels;
            auto pixels = stbi_loadf(argv[2], &width, &height, &channels, 4);
            if (pixels)
            {
                environmentMap.width = width;
                environmentMap.height = height;
                environmentMap.pixels.resize(width * height * 4);
                std::copy(pixels, pixels + width * height * 4, environmentMap.pixels.data());
                stbi_image_free(pixels);
                std::cout << "Loaded HDRI from " << argv[2] << " (max intensity: " << *std::max_element(environmentMap.pixels.begin(), environmentMap.pixels.end()) << ")" << std::endl;
            }
            else
            {
                std::cout << "Failed to load HDRI from " << argv[2] << std::endl;
            }
        }
        else
        {
            // Try to parse R,G,B background color

            std::istringstream is(argv[2]);
            is >> environmentMap.pixels[0];
            is.get();
            is >> environmentMap.pixels[1];
            is.get();
            is >> environmentMap.pixels[2];
            if (!is)
            {
                std::cout << "Failed to parse background color \"" << argv[2] << "\"" << std::endl;
                environmentMap.pixels = {0.f, 0.f, 0.f, 0.f};
            }
        }
    }

    std::vector<std::uint32_t> cameraNodes;
    for (std::uint32_t i = 0; i < asset.nodes.size(); ++i)
        if (asset.nodes[i].camera)
            cameraNodes.push_back(i);

    Camera camera;
    if (!cameraNodes.empty())
        camera = Camera(asset, asset.nodes[cameraNodes.front()]);
    camera.setAspectRatio(application.width() * 1.f / application.height());

    SceneData sceneData(asset, environmentMap, application.device(), application.queue(),
        renderer.geometryBindGroupLayout(), renderer.materialBindGroupLayout());

    std::unordered_set<SDL_Scancode> keysDown;

    int frameId = 0;
    double time = 0.0;

    bool leftMouseButtonDown = false;

    auto lastFrameStart = std::chrono::high_resolution_clock::now();

    for (bool running = true; running;)
    {
        bool cameraMoved = false;
        bool screenResized = false;

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
                screenResized = true;
                break;
            }
            break;
        case SDL_MOUSEBUTTONDOWN:
            if (event->button.button == SDL_BUTTON_LEFT)
            {
                leftMouseButtonDown = true;
                application.setMouseHidden(true);
            }
            break;
        case SDL_MOUSEBUTTONUP:
            if (event->button.button == SDL_BUTTON_LEFT)
            {
                leftMouseButtonDown = false;
                application.setMouseHidden(false);
            }
            break;
        case SDL_MOUSEMOTION:
            if (leftMouseButtonDown)
            {
                float speed = 2.f / application.height();
                camera.rotateX(event->motion.xrel * speed);
                camera.rotateY(event->motion.yrel * speed);
                cameraMoved = true;
            }
            break;
        case SDL_KEYDOWN:
            keysDown.insert(event->key.keysym.scancode);
            if (event->key.keysym.scancode == SDL_SCANCODE_SPACE)
                renderer.setRenderMode(Renderer::Mode::RaytraceMonteCarlo);
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

        camera.setAspectRatio(application.width() * 1.f / application.height());

        auto thisFrameStart = std::chrono::high_resolution_clock::now();
        float const dt = std::chrono::duration_cast<std::chrono::duration<float>>(thisFrameStart - lastFrameStart).count();
        lastFrameStart = thisFrameStart;

        time += dt;

        {
            float rotationSpeed = 1.f;
            float movementSpeed = 10.f; // TODO: select based on scene size

            if (keysDown.contains(SDL_SCANCODE_Q))
            {
                camera.rotateZ(- rotationSpeed * dt);
                cameraMoved = true;
            }
            if (keysDown.contains(SDL_SCANCODE_E))
            {
                camera.rotateZ(  rotationSpeed * dt);
                cameraMoved = true;
            }

            if (keysDown.contains(SDL_SCANCODE_S))
            {
                camera.moveForward(- movementSpeed * dt);
                cameraMoved = true;
            }
            if (keysDown.contains(SDL_SCANCODE_W))
            {
                camera.moveForward(  movementSpeed * dt);
                cameraMoved = true;
            }

            if (keysDown.contains(SDL_SCANCODE_A))
            {
                camera.moveRight(- movementSpeed * dt);
                cameraMoved = true;
            }
            if (keysDown.contains(SDL_SCANCODE_D))
            {
                camera.moveRight(  movementSpeed * dt);
                cameraMoved = true;
            }
        }

        if (cameraMoved || screenResized)
            renderer.setRenderMode(Renderer::Mode::Preview);

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
