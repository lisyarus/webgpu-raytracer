use camera.wgsl;
use geometry.wgsl;
use material.wgsl;
use raytrace_common.wgsl;
use random.wgsl;

@group(0) @binding(0) var<uniform> camera : Camera;

@group(1) @binding(0) var<storage, read> vertices : array<Vertex>;
@group(1) @binding(1) var<storage, read> indices : array<u32>;

@group(2) @binding(0) var<storage, read> materials : array<Material>;

struct SceneIntersection
{
	intersects : bool,
	distance : f32,
	vertices : array<Vertex, 3>,
	uv : vec2f,
}

fn intersectScene(ray : Ray) -> SceneIntersection {
	var result = SceneIntersection(
		false,
		1e30,
		array<Vertex, 3>(
			defaultVertex(),
			defaultVertex(),
			defaultVertex()
		),
		vec2f(0.0)
	);

	for (var i = 0u; i < arrayLength(&indices); i += 3u) {
		let v0 = vertices[indices[i + 0u]];
		let v1 = vertices[indices[i + 1u]];
		let v2 = vertices[indices[i + 2u]];

		let hit = intersectRayTriangle(ray, v0.position, v1.position, v2.position);
		if (hit.intersects && hit.distance < result.distance) {
			result.intersects = true;
			result.distance = hit.distance;
			result.vertices[0] = v0;
			result.vertices[1] = v1;
			result.vertices[2] = v2;
			result.uv = hit.uv;
		}
	}

	return result;
}

fn raytraceMonteCarlo(ray : Ray, randomState : ptr<function, RandomState>) -> vec3f {
	var accumulatedColor = vec3f(0.0);
	var colorFactor = vec3f(1.0);

	var currentRay = ray;

	for (var rayDepth = 0u; rayDepth < 8u; rayDepth += 1u) {
		let intersection = intersectScene(currentRay);
		if (intersection.intersects) {
			let material = materials[intersection.vertices[0].materialID];

			var normal = normalize(cross(intersection.vertices[1].position - intersection.vertices[0].position, intersection.vertices[2].position - intersection.vertices[0].position));
			if (dot(normal, currentRay.direction) > 0.0) {
				normal = -normal;
			}

			let reflectedDirection = cosineHemisphere(randomState, normal);

			accumulatedColor += material.emissiveFactor.rgb * colorFactor;

			colorFactor *= material.baseColorFactor.rgb;

			let intersectionPoint = currentRay.origin + currentRay.direction * intersection.distance;
			currentRay = Ray(intersectionPoint + reflectedDirection * 1e-4, reflectedDirection);
		} else {
			break;
		}
	}

	return accumulatedColor;
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
	var randomState = RandomState(0);
	initRandom(&randomState, u32(in.position.x));
	initRandom(&randomState, u32(in.position.y));
	initRandom(&randomState, camera.frameID);

	let screenPosition = in.screenSpacePosition + vec2f(uniformFloat(&randomState), uniformFloat(&randomState)) / vec2f(camera.screenSize);

	let cameraRay = computeCameraRay(camera.viewProjectionInverseMatrix, screenPosition);

	return vec4f(raytraceMonteCarlo(cameraRay, &randomState), 1.0 / (f32(camera.frameID) + 1.0));
}
