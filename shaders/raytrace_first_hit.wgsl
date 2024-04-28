use camera.wgsl;
use geometry.wgsl;
use material.wgsl;
use raytrace_common.wgsl;

@group(0) @binding(0) var<uniform> camera : Camera;

@group(1) @binding(0) var<storage, read> vertices : array<Vertex>;
@group(1) @binding(1) var<storage, read> indices : array<u32>;

@group(2) @binding(0) var<storage, read> materials : array<Material>;

fn raytraceFirstHit(ray : Ray) -> vec3f {
	var color = vec3f(0.0);
	var closestHit = TriangleHit(1e30, vec2f(0.0), false);

	for (var i = 0u; i < arrayLength(&indices); i += 3u) {
		let v0 = vertices[indices[i + 0u]];
		let v1 = vertices[indices[i + 1u]];
		let v2 = vertices[indices[i + 2u]];

		let hit = intersectRayTriangle(ray, v0.position, v1.position, v2.position);
		if (hit.intersects && hit.distance < closestHit.distance) {
			closestHit = hit;

			var normal = normalize(cross(v1.position - v0.position, v2.position - v0.position));
			if (dot(normal, ray.direction) > 0.0) {
				normal = -normal;
			}

			let lightDirection = normalize(vec3f(1.0, 3.0, 2.0));
			let material = materials[v0.materialID];

			color = 0.5 * material.baseColorFactor.rgb * (0.5 + 0.5 * dot(normal, lightDirection)) + material.emissiveFactor.rgb;
		}
	}

	return color;
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
