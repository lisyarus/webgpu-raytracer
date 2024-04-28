use camera.wgsl;
use geometry.wgsl;
use material.wgsl;

@group(0) @binding(0) var<uniform> camera : Camera;

@group(1) @binding(0) var<storage, read> vertices : array<Vertex>;
@group(1) @binding(1) var<storage, read> indices : array<u32>;

@group(2) @binding(0) var<storage, read> materials : array<Material>;

fn perspectiveDivide(v : vec4f) -> vec3f {
	return v.xyz / v.w;
}

struct Ray
{
	origin : vec3f,
	direction : vec3f,
}

fn computeCameraRay(viewProjectionInverseMatrix : mat4x4f, screenSpacePosition : vec2f) -> Ray {
	let p0 = perspectiveDivide(viewProjectionInverseMatrix * vec4f(screenSpacePosition, 0.0, 1.0));
	let p1 = perspectiveDivide(viewProjectionInverseMatrix * vec4f(screenSpacePosition, 1.0, 1.0));

	return Ray(
		p0,
		normalize(p1 - p0)
	);
}

struct TriangleHit
{
	distance : f32,
	uv : vec2f,
	intersects : bool,
}

fn det(v0 : vec3f, v1 : vec3f, v2 : vec3f) -> f32 {
	return dot(v0, cross(v1, v2));
}

fn solve3x3Equation(matrix : mat3x3f, rhs : vec3f) -> vec3f {
	// Use Cramer's rule to solve the system
	let det = det(matrix[0], matrix[1], matrix[2]);
	let d0 = det(rhs, matrix[1], matrix[2]);
	let d1 = det(matrix[0], rhs, matrix[2]);
	let d2 = det(matrix[0], matrix[1], rhs);

	return vec3f(d0, d1, d2) / det;
}

fn intersectRayTriangle(ray : Ray, p0 : vec3f, p1 : vec3f, p2 : vec3f) -> TriangleHit
{
	// Solve linear system:
	// origin + t * direction = p0 + u * (p1 - p0) + v * (p2 - p0)

	// Similar to Möller–Trumbore intersection algorithm

	let solution = solve3x3Equation(mat3x3f(ray.direction, p0 - p1, p0 - p2), p0 - ray.origin);

	let intersects = solution.x >= 0.0 && solution.y >= 0.0 && solution.z >= 0.0 && (solution.y + solution.z) <= 1.0;

	return TriangleHit(
		solution.x,
		solution.yz,
		intersects
	);
}

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
