use camera.wgsl;
use geometry.wgsl;
use material.wgsl;
use raytrace_common.wgsl;
use random.wgsl;

@group(0) @binding(0) var<uniform> camera : Camera;

@group(1) @binding(0) var<storage, read> vertices : array<Vertex>;
@group(1) @binding(1) var<storage, read> indices : array<u32>;

@group(2) @binding(0) var<storage, read> materials : array<Material>;

@group(3) @binding(0) var accumulationTexture : texture_storage_2d<rgba32float, read_write>;

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

const backgroundColor = vec3(0.0);

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
			accumulatedColor += colorFactor * backgroundColor;
			break;
		}
	}

	return accumulatedColor;
}

@compute @workgroup_size(8, 8)
fn computeMain(@builtin(global_invocation_id) id: vec3<u32>) {
	var randomState = RandomState(0);
	initRandom(&randomState, id.x);
	initRandom(&randomState, id.y);
	initRandom(&randomState, camera.frameID);

	let screenPosition = 2.0 * vec2f(f32(id.x) + uniformFloat(&randomState), f32(id.y) + uniformFloat(&randomState)) / vec2f(camera.screenSize) - vec2f(1.0);

	let cameraRay = computeCameraRay(camera.viewProjectionInverseMatrix, screenPosition * vec2f(1.0, -1.0));

	let color = raytraceMonteCarlo(cameraRay, &randomState);
	let alpha = 1.0 / (f32(camera.frameID) + 1.0);

	if (id.x < camera.screenSize.x && id.y < camera.screenSize.y) {
		let accumulatedColor = textureLoad(accumulationTexture, id.xy);
		let storedColor = mix(accumulatedColor, vec4f(color, 1.0), alpha);
		textureStore(accumulationTexture, id.xy, storedColor);
	}
}
