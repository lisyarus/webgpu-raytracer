use math.wgsl;
use camera.wgsl;
use material.wgsl;
use env_map.wgsl;

@group(0) @binding(0) var<uniform> camera : Camera;

@group(1) @binding(0) var<storage, read> materials : array<Material>;
@group(1) @binding(1) var environmentMap : texture_storage_2d<rgba32float, read>;
@group(1) @binding(2) var textureSampler : sampler;
@group(1) @binding(3) var albedoTexture : texture_2d_array<f32>;

struct VertexInput {
	@builtin(vertex_index) index : u32,
	@location(0) position : vec3f,
	@location(1) normal : vec3f,
	@location(2) materialID : u32,
	@location(3) texcoord : vec2f,
}

struct VertexOutput {
	@builtin(position) position : vec4f,
	@location(0) worldPosition : vec3f,
	@location(1) normal : vec3f,
	@location(2) texcoord : vec2f,
	@location(3) @interpolate(flat) materialID : u32,
}

@vertex
fn vertexMain(in : VertexInput) -> VertexOutput {
	return VertexOutput(
		camera.viewProjectionMatrix * vec4f(in.position, 1.0),
		in.position,
		in.normal,
		in.texcoord,
		in.materialID,
	);
}

@fragment
fn fragmentMain(in : VertexOutput, @builtin(front_facing) front_facing : bool) -> @location(0) vec4f {
	let material = materials[in.materialID];

	let normal = normalize(select(-in.normal, in.normal, front_facing));

	let lightDirection = normalize(vec3f(1.0, 3.0, 2.0));

	let albedoSample = textureSampleLevel(albedoTexture, textureSampler, in.texcoord, material.textureLayers.x, 0.0);

	if (albedoSample.a < 0.5) {
		discard;
	}

	let albedo = material.baseColorFactorAndTransmission.rgb * albedoSample.rgb;

	let litColor = albedo * (0.5 + 0.5 * dot(normal, lightDirection)) + material.emissiveFactor.rgb;

	let cameraDirection = normalize(camera.position - in.worldPosition);
	let reflectedDirection = 2.0 * normal * dot(normal, cameraDirection) - cameraDirection;

	let reflectedColor = sampleEnvMap(environmentMap, reflectedDirection) * albedo;

	let metallic = material.metallicRoughnessFactorAndIor.b;

	return vec4f(mix(litColor, reflectedColor, metallic), 1.0);
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
