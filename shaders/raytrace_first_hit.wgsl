use camera.wgsl;
use geometry.wgsl;
use material.wgsl;
use raytrace_common.wgsl;

@group(0) @binding(0) var<uniform> camera : Camera;

@group(1) @binding(0) var<storage, read> vertexPositions : array<vec4f>;
@group(1) @binding(1) var<storage, read> vertexAttributes : array<Vertex>;
@group(1) @binding(2) var<storage, read> bvhNodes : array<BVHNode>;
@group(1) @binding(3) var<storage, read> emissiveTriangles : TriangleArray;
@group(1) @binding(4) var<storage, read> emissiveAliasTable : array<vec2u>;
@group(1) @binding(5) var<storage, read> emissiveBvhNodes : array<BVHNode>;

@group(2) @binding(0) var<storage, read> materials : array<Material>;

@group(3) @binding(0) var accumulationTexture : texture_storage_2d<rgba32float, read_write>;

use bvh_traverse.wgsl;

fn raytraceFirstHit(ray : Ray) -> vec3f {
	let intersection = intersectScene(ray);

	if (intersection.intersects) {
		var normal = normalize(cross(intersection.vertices[1] - intersection.vertices[0], intersection.vertices[2] - intersection.vertices[0]));
		if (dot(normal, ray.direction) > 0.0) {
			normal = -normal;
		}

		let lightDirection = normalize(vec3f(1.0, 3.0, 2.0));
		let material = materials[vertexAttributes[3 * intersection.triangleID].materialID];

		return 0.5 * material.baseColorFactorAndTransmission.rgb * (0.5 + 0.5 * dot(normal, lightDirection)) + material.emissiveFactor.rgb;
	} else {
		return vec3f(0.0);
	}
}

@compute @workgroup_size(8, 8)
fn computeMain(@builtin(global_invocation_id) id: vec3<u32>) {
	let screenPosition = 2.0 * vec2f(id.xy) / vec2f(camera.screenSize) - vec2f(1.0);

	let cameraRay = computeCameraRay(camera.position, camera.viewProjectionInverseMatrix, screenPosition * vec2f(1.0, -1.0));

	let color = raytraceFirstHit(cameraRay);

	if (id.x < camera.screenSize.x && id.y < camera.screenSize.y) {
		textureStore(accumulationTexture, id.xy, vec4f(color, 1.0));
	}
}
