use camera.wgsl;
use geometry.wgsl;
use material.wgsl;
use raytrace_common.wgsl;

@group(0) @binding(0) var<uniform> camera : Camera;

@group(1) @binding(0) var<storage, read> vertices : array<Vertex>;
@group(1) @binding(2) var<storage, read> bvhNodes : array<BVHNode>;

@group(2) @binding(0) var<storage, read> materials : array<Material>;

use bvh_traverse.wgsl;

fn raytraceFirstHit(ray : Ray) -> vec3f {
	let intersection = intersectScene(ray);

	if (intersection.intersects) {
		var normal = normalize(cross(intersection.vertices[1].position - intersection.vertices[0].position, intersection.vertices[2].position - intersection.vertices[0].position));
		if (dot(normal, ray.direction) > 0.0) {
			normal = -normal;
		}

		let lightDirection = normalize(vec3f(1.0, 3.0, 2.0));
		let material = materials[intersection.vertices[0].materialID];

		return 0.5 * material.baseColorFactor.rgb * (0.5 + 0.5 * dot(normal, lightDirection)) + material.emissiveFactor.rgb;
	} else {
		return vec3f(0.0);
	}
}

struct VertexOut
{
	@builtin(position) position : vec4f,
	@location(0) screenSpacePosition : vec2f,
}

@vertex
fn vertexMain(@builtin(vertex_index) index : u32) -> VertexOut {

	// Hard-coded triangle that encloses the whole screen

	if (index == 0u) {
		return VertexOut(
			vec4f(-1.0, -1.0, 0.0, 1.0),
			vec2f(-1.0, -1.0),
		);
	} else if (index == 1u) {
		return VertexOut(
			vec4f( 3.0, -1.0, 0.0, 1.0),
			vec2f( 3.0, -1.0),
		);
	} else {
		return VertexOut(
			vec4f(-1.0,  3.0, 0.0, 1.0),
			vec2f(-1.0,  3.0),
		);
	}
}

@fragment
fn fragmentMain(in : VertexOut) -> @location(0) vec4f {

	let cameraRay = computeCameraRay(camera.viewProjectionInverseMatrix, in.screenSpacePosition);

	return vec4f(raytraceFirstHit(cameraRay), 1.0);
}
