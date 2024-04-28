#pragma once

#include <webgpu.h>

WGPUBindGroupLayout createAccumulationBindGroupLayout(WGPUDevice device);

WGPUBindGroup createAccumulationBindGroup(WGPUDevice device, WGPUBindGroupLayout bindGroupLayout, WGPUTextureView accumulationTextureView);
