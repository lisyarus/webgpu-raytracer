use math.wgsl;

struct Ray
{
	origin : vec3f,
	direction : vec3f,
}

fn computeCameraRay(cameraPosition : vec3f, viewProjectionInverseMatrix : mat4x4f, screenSpacePosition : vec2f) -> Ray {
	let p = perspectiveDivide(viewProjectionInverseMatrix * vec4f(screenSpacePosition, 0.0, 1.0));

	return Ray(
		cameraPosition,
		normalize(p - cameraPosition)
	);
}

struct TriangleHit
{
	distance : f32,
	uv : vec2f,
	intersects : bool,
}

fn intersectRayTriangle(ray : Ray, p0 : vec3f, p1 : vec3f, p2 : vec3f) -> TriangleHit {
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

struct AABBHit
{
	distance : f32,
	intersects : bool,
}

fn intersectRayAABB(ray : Ray, aabbMin : vec3f, aabbMax : vec3f) -> AABBHit {
	let tMin = (aabbMin - ray.origin) / ray.direction;
	let tMax = (aabbMax - ray.origin) / ray.direction;

	let tX = vec2f(min(tMin.x, tMax.x), max(tMin.x, tMax.x));
	let tY = vec2f(min(tMin.y, tMax.y), max(tMin.y, tMax.y));
	let tZ = vec2f(min(tMin.z, tMax.z), max(tMin.z, tMax.z));

	let t0 = max(tX.x, max(tY.x, tZ.x));
	let t1 = min(tX.y, min(tY.y, tZ.y));

	return AABBHit(max(t0, 0.0), t1 >= t0 && t1 >= 0.0);
}
