use camera.wgsl;
use material.wgsl;

@group(0) @binding(0) var<uniform> camera : Camera;
@group(1) @binding(0) var<storage, read> materials : array<Material>;

struct VertexInput {
	@builtin(vertex_index) index : u32,
	@location(0) position : vec3f,
	@location(1) normal : vec3f,
	@location(2) materialID: u32,
}

struct VertexOutput {
	@builtin(position) position : vec4f,
	@location(0) normal : vec3f,
	@location(1) color : vec3f,
	@location(2) emission : vec3f,
}

@vertex
fn vertexMain(in : VertexInput) -> VertexOutput {
	let material = materials[in.materialID];
	return VertexOutput(
		camera.viewProjectionMatrix * vec4f(in.position, 1.0),
		in.normal,
		material.baseColorFactorAndTransmission.rgb,
		material.emissiveFactor.rgb,
	);
}

@fragment
fn fragmentMain(in : VertexOutput, @builtin(front_facing) front_facing : bool) -> @location(0) vec4f {
	let normal = normalize(select(-in.normal, in.normal, front_facing));

	let lightDirection = normalize(vec3f(1.0, 3.0, 2.0));

	let color = in.color * 0.5 * (0.5 + 0.5 * dot(normal, lightDirection)) + in.emission;

	return vec4f(color, 1.0);
}
