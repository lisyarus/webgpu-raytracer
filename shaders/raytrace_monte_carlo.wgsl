use camera.wgsl;
use geometry.wgsl;
use material.wgsl;
use raytrace_common.wgsl;
use random.wgsl;

@group(0) @binding(0) var<uniform> camera : Camera;

@group(1) @binding(0) var<storage, read> vertexPositions : array<vec4f>;
@group(1) @binding(1) var<storage, read> vertexAttributes : array<Vertex>;
@group(1) @binding(2) var<storage, read> bvhNodes : array<BVHNode>;
@group(1) @binding(3) var<storage, read> emissiveTriangles : array<u32>;
@group(1) @binding(4) var<storage, read> emissiveBvhNodes : array<BVHNode>;

@group(2) @binding(0) var<storage, read> materials : array<Material>;

@group(3) @binding(0) var accumulationTexture : texture_storage_2d<rgba32float, read_write>;

use bvh_traverse.wgsl;

const backgroundColor = vec3(0.0);

fn raytraceMonteCarlo(ray : Ray, randomState : ptr<function, RandomState>) -> vec3f {
	var accumulatedColor = vec3f(0.0);
	var colorFactor = vec3f(1.0);

	var currentRay = ray;

	for (var rayDepth = 0u; rayDepth < 4u; rayDepth += 1u) {
		let intersection = intersectScene(currentRay);

		if (intersection.intersects) {
			let intersectionPoint = currentRay.origin + currentRay.direction * intersection.distance;

			let v0 = vertexAttributes[3 * intersection.triangleID + 0u];
			let v1 = vertexAttributes[3 * intersection.triangleID + 1u];
			let v2 = vertexAttributes[3 * intersection.triangleID + 2u];

			let material = materials[v0.materialID];

			var geometryNormal = normalize(cross(intersection.vertices[1] - intersection.vertices[0], intersection.vertices[2] - intersection.vertices[0]));

			var shadingNormal = normalize(v0.normal + intersection.uv.x * (v1.normal - v0.normal) + intersection.uv.y * (v2.normal - v0.normal));

			if (dot(geometryNormal, currentRay.direction) > 0.0) {
				geometryNormal = -geometryNormal;
				shadingNormal = -shadingNormal;
			}

			var newRay = Ray(intersectionPoint + geometryNormal * 1e-4, vec3f(0.0));

			let emissiveTriangleCount = arrayLength(&emissiveTriangles);

			if (uniformUint(randomState, 2u) == 0u) {
				newRay.direction = cosineHemisphere(randomState, shadingNormal);
			} else {
				let lightTriangleIndex = uniformUint(randomState, emissiveTriangleCount);
				let lightTriangle = emissiveTriangles[lightTriangleIndex];

				var lightUV = vec2f(uniformFloat(randomState), uniformFloat(randomState));
				if (dot(lightUV, vec2f(1.0)) > 1.0) {
					lightUV = vec2f(1.0) - lightUV;
				}

				let lightV0 = vertexPositions[3 * lightTriangle + 0u].xyz;
				let lightV1 = vertexPositions[3 * lightTriangle + 1u].xyz;
				let lightV2 = vertexPositions[3 * lightTriangle + 2u].xyz;

				let lightPoint = lightV0 * (1.0 - lightUV.x - lightUV.y) + lightV1 * lightUV.x + lightV2 * lightUV.y;

				newRay.direction = normalize(lightPoint - intersectionPoint);
			}

			let cosineHemisphereProbability = max(0.0, dot(newRay.direction, shadingNormal)) / PI;

			var directLightSamplingProbability = lightSamplingProbability(newRay);

			let totalMISProbability = cosineHemisphereProbability * 0.5 + directLightSamplingProbability * 0.5;

			accumulatedColor += material.emissiveFactor.rgb * colorFactor;

			colorFactor *= (material.baseColorFactor.rgb / PI) * max(0.0, dot(shadingNormal, newRay.direction)) / max(1e-8, totalMISProbability);

			currentRay = newRay;
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
	initRandom(&randomState, camera.frameID);
	initRandom(&randomState, id.x);
	initRandom(&randomState, id.y);

	let screenPosition = 2.0 * vec2f(f32(id.x) + uniformFloat(&randomState), f32(id.y) + uniformFloat(&randomState)) / vec2f(camera.screenSize) - vec2f(1.0);

	let cameraRay = computeCameraRay(camera.position, camera.viewProjectionInverseMatrix, screenPosition * vec2f(1.0, -1.0));

	let color = raytraceMonteCarlo(cameraRay, &randomState);
	let alpha = 1.0 / (f32(camera.frameID) + 1.0);

	if (id.x < camera.screenSize.x && id.y < camera.screenSize.y) {
		let accumulatedColor = textureLoad(accumulationTexture, id.xy);
		let storedColor = mix(accumulatedColor, vec4f(color, 1.0), alpha);
		textureStore(accumulationTexture, id.xy, storedColor);
	}
}
