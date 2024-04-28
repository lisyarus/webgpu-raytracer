#pragma once

#include <webgpu.h>

WGPUBindGroupLayout createGeometryBindGroupLayout(WGPUDevice device);

WGPUBindGroup createGeometryBindGroup(WGPUDevice device, WGPUBindGroupLayout bindGroupLayout, WGPUBuffer vertexBuffer, WGPUBuffer indexBuffer);
