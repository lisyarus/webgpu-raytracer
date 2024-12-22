use math.wgsl;
use camera.wgsl;
use material.wgsl;
use env_map.wgsl;

@group(0) @binding(0) var<uniform> camera : Camera;

@group(1) @binding(0) var<storage, read> materials : array<Material>;
@group(1) @binding(1) var environmentMap : texture_storage_2d<rgba32float, read>;

struct VertexInput {
	@builtin(vertex_index) index : u32,
	@location(0) position : vec3f,
	@location(1) normal : vec3f,
	@location(2) materialID: u32,
}

struct VertexOutput {
	@builtin(position) position : vec4f,
	@location(0) worldPosition : vec3f,
	@location(1) normal : vec3f,
	@location(2) color : vec3f,
	@location(3) emission : vec3f,
	@location(4) metallic : f32,
}

@vertex
fn vertexMain(in : VertexInput) -> VertexOutput {
	let material = materials[in.materialID];
	return VertexOutput(
		camera.viewProjectionMatrix * vec4f(in.position, 1.0),
		in.position,
		in.normal,
		material.baseColorFactorAndTransmission.rgb,
		material.emissiveFactor.rgb,
		material.metallicRoughnessFactorAndIor.b,
	);
}

@fragment
fn fragmentMain(in : VertexOutput, @builtin(front_facing) front_facing : bool) -> @location(0) vec4f {
	let normal = normalize(select(-in.normal, in.normal, front_facing));

	let lightDirection = normalize(vec3f(1.0, 3.0, 2.0));

	let color = in.color * 0.5 * (0.5 + 0.5 * dot(normal, lightDirection)) + in.emission;

	let cameraDirection = normalize(camera.position - in.worldPosition);
	let reflectedDirection = 2.0 * normal * dot(normal, cameraDirection) - cameraDirection;

	let reflectedColor = sampleEnvMap(environmentMap, reflectedDirection) * in.color;

	return vec4f(mix(color, reflectedColor, in.metallic), 1.0);
}

struct BackgroundVertexOutput
{
	@builtin(position) position : vec4f,
	@location(0) worldSpacePosition : vec3f,
}

@vertex
fn backgroundVertexMain(@builtin(vertex_index) index : u32) -> BackgroundVertexOutput
{
	var ndc = vec4f(-1.0, -1.0, 0.0, 1.0);
	if (index == 1u) {
		ndc = vec4f(3.0, -1.0, 0.0, 1.0);
	}
	else if (index == 2u) {
		ndc = vec4f(-1.0, 3.0, 0.0, 1.0);
	}

	return BackgroundVertexOutput(
		ndc,
		perspectiveDivide(camera.viewProjectionInverseMatrix * ndc)
	);
}

@fragment
fn backgroundFragmentMain(in : BackgroundVertexOutput) -> @location(0) vec4f
{
	let direction = in.worldSpacePosition - camera.position;
	return vec4f(sampleEnvMap(environmentMap, direction), 1.0);
}
