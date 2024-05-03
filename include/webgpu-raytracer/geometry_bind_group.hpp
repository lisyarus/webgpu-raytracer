#pragma once

#include <webgpu.h>

WGPUBindGroupLayout createGeometryBindGroupLayout(WGPUDevice device);

WGPUBindGroup createGeometryBindGroup(WGPUDevice device, WGPUBindGroupLayout bindGroupLayout, WGPUBuffer vertexPositionsBuffer,
    WGPUBuffer vertexAttributesBuffer, WGPUBuffer bvhNodesBuffer, WGPUBuffer emissiveTrianglesBuffer, WGPUBuffer emissiveBvhNodesBuffer);
