#include <webgpu-raytracer/sdl_wgpu.h>

#ifdef __APPLE__
#include <SDL_syswm.h>
#else
#include <SDL2/SDL_syswm.h>
#endif

#ifdef SDL_VIDEO_DRIVER_COCOA
#include <Foundation/Foundation.h>
#include <QuartzCore/CAMetalLayer.h>
#include <Cocoa/Cocoa.h>
#endif

WGPUSurface SDL_WGPU_CreateSurface(WGPUInstance instance, SDL_Window * window)
{
    SDL_SysWMinfo info;
    SDL_VERSION(&info.version);
    SDL_GetWindowWMInfo(window, &info);

    switch (info.subsystem)
    {
#ifdef SDL_VIDEO_DRIVER_X11
    case SDL_SYSWM_X11:
        {
            WGPUSurfaceDescriptorFromXlibWindow surfaceDescriptorFromXlibWindow;
            surfaceDescriptorFromXlibWindow.chain.next = 0;
            surfaceDescriptorFromXlibWindow.chain.sType = WGPUSType_SurfaceDescriptorFromXlibWindow;
            surfaceDescriptorFromXlibWindow.window = info.info.x11.window;
            surfaceDescriptorFromXlibWindow.display = info.info.x11.display;

            WGPUSurfaceDescriptor surfaceDescriptor;
            surfaceDescriptor.label = 0;
            surfaceDescriptor.nextInChain = (const WGPUChainedStruct*)&surfaceDescriptorFromXlibWindow;

            return wgpuInstanceCreateSurface(instance, &surfaceDescriptor);
        }
#endif

#ifdef SDL_VIDEO_DRIVER_WAYLAND
    case SDL_SYSWM_WAYLAND:
        {
            WGPUSurfaceDescriptorFromWaylandSurface surfaceDescriptorFromWaylandSurface;
            surfaceDescriptorFromWaylandSurface.chain.next = 0;
            surfaceDescriptorFromWaylandSurface.chain.sType = WGPUSType_SurfaceDescriptorFromWaylandSurface;
            surfaceDescriptorFromWaylandSurface.display = info.info.wl.display;
            surfaceDescriptorFromWaylandSurface.surface = info.info.wl.surface;

            WGPUSurfaceDescriptor surfaceDescriptor;
            surfaceDescriptor.label = 0;
            surfaceDescriptor.nextInChain = (const WGPUChainedStruct*)&surfaceDescriptorFromWaylandSurface;

            return wgpuInstanceCreateSurface(instance, &surfaceDescriptor);
        }
#endif

#ifdef SDL_VIDEO_DRIVER_WINDOWS
    case SDL_SYSWM_WINDOWS:
        {
            WGPUSurfaceDescriptorFromWindowsHWND surfaceDescriptorFromWindowsHWND;
            surfaceDescriptorFromWindowsHWND.chain.next = 0;
            surfaceDescriptorFromWindowsHWND.chain.sType = WGPUSType_SurfaceDescriptorFromWindowsHWND;
            surfaceDescriptorFromWindowsHWND.hwnd = info.info.win.window;
            surfaceDescriptorFromWindowsHWND.hinstance = info.info.win.hinstance;

            WGPUSurfaceDescriptor surfaceDescriptor;
            surfaceDescriptor.label = 0;
            surfaceDescriptor.nextInChain = (const WGPUChainedStruct*)&surfaceDescriptorFromWindowsHWND;

            return wgpuInstanceCreateSurface(instance, &surfaceDescriptor);
        }
#endif

#ifdef SDL_VIDEO_DRIVER_COCOA
    case SDL_SYSWM_COCOA:
        {
            NSWindow * nsWindow = info.info.cocoa.window;
            [nsWindow.contentView setWantsLayer : YES];
            id metalLayer = [CAMetalLayer layer];
            [nsWindow.contentView setLayer : metalLayer];

            WGPUSurfaceDescriptorFromMetalLayer surfaceDescriptorFromMetalLayer;
            surfaceDescriptorFromMetalLayer.chain.next = 0;
            surfaceDescriptorFromMetalLayer.chain.sType = WGPUSType_SurfaceDescriptorFromMetalLayer;
            surfaceDescriptorFromMetalLayer.layer = metalLayer;

            WGPUSurfaceDescriptor surfaceDescriptor;
            surfaceDescriptor.label = 0;
            surfaceDescriptor.nextInChain = (const WGPUChainedStruct*)&surfaceDescriptorFromMetalLayer;

            return wgpuInstanceCreateSurface(instance, &surfaceDescriptor);
        }
#endif

    default:
        return 0;
    }

}
