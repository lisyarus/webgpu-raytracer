struct Camera
{
	viewProjectionMatrix : mat4x4f,
}

@group(0) @binding(0) var<uniform> camera: Camera;

struct VertexInput {
	@location(0) position : vec3f,
	@location(1) normal : vec3f,
}

struct VertexOutput {
	@builtin(position) position : vec4f,
	@location(0) normal : vec3f,
}

@vertex
fn vertexMain(in : VertexInput) -> VertexOutput {
	return VertexOutput(
		camera.viewProjectionMatrix * vec4f(in.position, 1.0),
		in.normal
	);
}

@fragment
fn fragmentMain(in : VertexOutput, @builtin(front_facing) front_facing : bool) -> @location(0) vec4f {
	return vec4f(select(-in.normal, in.normal, front_facing) * 0.5 + vec3f(0.5), 1.0);
}
