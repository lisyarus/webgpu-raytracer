use camera.wgsl;
use geometry.wgsl;
use material.wgsl;
use raytrace_common.wgsl;
use random.wgsl;
use brdf.wgsl;

@group(0) @binding(0) var<uniform> camera : Camera;

@group(1) @binding(0) var<storage, read> vertexPositions : array<vec4f>;
@group(1) @binding(1) var<storage, read> vertexAttributes : array<Vertex>;
@group(1) @binding(2) var<storage, read> bvhNodes : array<BVHNode>;
@group(1) @binding(3) var<storage, read> emissiveTriangles : TriangleArray;
@group(1) @binding(4) var<storage, read> emissiveBvhNodes : array<BVHNode>;

@group(2) @binding(0) var<storage, read> materials : array<Material>;

@group(3) @binding(0) var accumulationTexture : texture_storage_2d<rgba32float, read_write>;

use bvh_traverse.wgsl;

fn raytraceMonteCarlo(ray : Ray, randomState : ptr<function, RandomState>) -> vec3f {
	var accumulatedColor = vec3f(0.0);
	var colorFactor = vec3f(1.0);

	var currentRay = ray;

	for (var rayDepth = 0u; rayDepth < 8u; rayDepth += 1u) {
		let intersection = intersectScene(currentRay);

		if (intersection.intersects) {
			let intersectionPoint = currentRay.origin + currentRay.direction * intersection.distance;

			let v0 = vertexAttributes[3 * intersection.triangleID + 0u];
			let v1 = vertexAttributes[3 * intersection.triangleID + 1u];
			let v2 = vertexAttributes[3 * intersection.triangleID + 2u];

			let material = materials[v0.materialID];

			let baseColor = material.baseColorFactorAndTransmission.rgb;
			let metallic = material.metallicRoughnessFactorAndIor.b;
			let roughness = max(0.05, material.metallicRoughnessFactorAndIor.g);
			var ior = material.metallicRoughnessFactorAndIor.a;
			let transmission = material.baseColorFactorAndTransmission.a;

			var geometryNormal = normalize(cross(intersection.vertices[1] - intersection.vertices[0], intersection.vertices[2] - intersection.vertices[0]));

			var shadingNormal = normalize(v0.normal + intersection.uv.x * (v1.normal - v0.normal) + intersection.uv.y * (v2.normal - v0.normal));

			// Invert the normals if we're looking at the surface from the inside
			if (dot(geometryNormal, currentRay.direction) > 0.0) {
				geometryNormal = -geometryNormal;
				shadingNormal = -shadingNormal;
				ior = 1.0 / ior;
			}

			var newRay = Ray(intersectionPoint, vec3f(0.0));

			// MIS weights empirically chosen depending on what works better for which materials:
			//     roughness = 0, metallic = 0 : vndf + cosine + light
			//     roughness = 0, metallic = 1 : vndf
			//     roughness = 1, metallic = 0 : cosine + light
			//     roughness = 1, metallic = 1 : vndf
			//                transmission = 1 : vndf + transmission vndf

			var cosineSamplingWeight = (1.0 - metallic) * (1.0 - transmission);
			var lightSamplingWeight = (1.0 - metallic) * (1.0 - transmission) * select(0.0, 1.0, emissiveTriangles.count > 0u);
			var vndfSamplingWeight = 1.0 - (1.0 - metallic) * roughness;
			var vndfTransmissionWeight = transmission;

			var sumSamplingWeights = cosineSamplingWeight + lightSamplingWeight + vndfSamplingWeight + vndfTransmissionWeight;

			cosineSamplingWeight /= sumSamplingWeights;
			lightSamplingWeight /= sumSamplingWeights;
			vndfSamplingWeight /= sumSamplingWeights;
			vndfTransmissionWeight /= sumSamplingWeights;

			let strategyPick = uniformFloat(randomState);

			if (strategyPick < cosineSamplingWeight) {
				newRay.direction = cosineHemisphere(randomState, shadingNormal);
			} else if (strategyPick < cosineSamplingWeight + vndfSamplingWeight) {
				newRay.direction = sampleVNDF(randomState, shadingNormal, -currentRay.direction, roughness);
			} else if (strategyPick < cosineSamplingWeight + vndfSamplingWeight + vndfTransmissionWeight) {
				newRay.direction = sampleTransmissionVNDF(randomState, shadingNormal, -currentRay.direction, roughness);
			} else {
				let lightTriangleIndex = uniformUint(randomState, emissiveTriangles.count);
				let lightTriangle = emissiveTriangles.triangles[lightTriangleIndex];

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
			let vndfSamplingProbability = probabilityVNDF(shadingNormal, -currentRay.direction, newRay.direction, roughness);
			let vndfTransmissionProbability = probabilityTransmissionVNDF(shadingNormal, -currentRay.direction, newRay.direction, roughness);
			let directLightSamplingProbability = lightSamplingProbability(newRay);

			// To properly apply MIS, one needs to compute the total probability of generating a reflected direction
			// using _all_possible_strategies_, see https://lisyarus.github.io/blog/posts/multiple-importance-sampling.html
			var totalMISProbability = cosineHemisphereProbability * cosineSamplingWeight
				+ vndfSamplingProbability * vndfSamplingWeight
				+ vndfTransmissionProbability * vndfTransmissionWeight
				+ directLightSamplingProbability * lightSamplingWeight;


			// totalMISProbability = 1.0 / (4.0 * PI);
			// newRay.direction = uniformSphere(randomState);

			accumulatedColor += material.emissiveFactor.rgb * colorFactor;

			let ndotr = dot(shadingNormal, newRay.direction);

			if (transmission > 0.0 || ndotr > 0.0) {
				let brdf = cookTorranceGGX(shadingNormal, newRay.direction, -currentRay.direction, baseColor, metallic, roughness, ior, transmission);

				colorFactor *= brdf * abs(ndotr) / max(1e-8, totalMISProbability);

				// Offset ray origin to side of the surface where new ray direction is pointing to,
				// to prevent self-intersection artifacts
				newRay.origin += sign(dot(newRay.direction, geometryNormal)) * geometryNormal * 1e-4;

				currentRay = newRay;
			}
			else
			{
				// Non-transmissive material and the new ray points inside the object
				// => brdf would return zero, colorFactor would be zero, and all
				// further recursive rays will be useless
				// Instead, just ignore this ray altogether
				break;
			}
		} else {
			accumulatedColor += colorFactor * camera.backgroundColor;
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
