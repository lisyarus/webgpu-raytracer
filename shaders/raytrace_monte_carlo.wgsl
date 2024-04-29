use camera.wgsl;
use geometry.wgsl;
use material.wgsl;
use raytrace_common.wgsl;
use random.wgsl;

@group(0) @binding(0) var<uniform> camera : Camera;

@group(1) @binding(0) var<storage, read> vertices : array<Vertex>;
@group(1) @binding(2) var<storage, read> bvhNodes : array<BVHNode>;

@group(2) @binding(0) var<storage, read> materials : array<Material>;

@group(3) @binding(0) var accumulationTexture : texture_storage_2d<rgba32float, read_write>;

struct SceneIntersection
{
	intersects : bool,
	distance : f32,
	vertices : array<Vertex, 3>,
	uv : vec2f,
	visitedNodeCount : u32,
	intersectedNodeCount : u32,
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
		vec2f(0.0),
		0u,
		0u
	);

	var nodeStack = array<u32, MAX_BVH_STACK_SIZE>();
	var nodeStackSize = 1u;
	nodeStack[0u] = 0u;

	while (nodeStackSize > 0u) {
		let nodeID = nodeStack[nodeStackSize - 1u];
		nodeStackSize -= 1u;

		result.visitedNodeCount += 1u;

		let node = bvhNodes[nodeID];

		let hit = intersectRayAABB(ray, vec3f(node.minX, node.minY, node.minZ), vec3f(node.maxX, node.maxY, node.maxZ));

		if (!hit.intersects || hit.distance > result.distance) {
			continue;
		}

		result.intersectedNodeCount += 1u;

		if (node.triangleCount > 0) {
			for (var i = 0u; i < node.triangleCount; i += 1u) {
				let triangleID = node.leftChildOrFirstTriangle + i;

				let v0 = vertices[3 * triangleID + 0u];
				let v1 = vertices[3 * triangleID + 1u];
				let v2 = vertices[3 * triangleID + 2u];

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
		} else {
			let firstChild = node.leftChildOrFirstTriangle & (~BVH_NODE_AXIS_MASK);

			// Ordered traversal: visit closer child first (i.e. put closer child higher on the stack)

			let nodeAxis = node.leftChildOrFirstTriangle >> BVH_NODE_AXIS_SHIFT;

			if (ray.direction[nodeAxis] > 0.0) {
				nodeStack[nodeStackSize] = firstChild + 1u;
				nodeStack[nodeStackSize + 1u] = firstChild;
			} else {
				nodeStack[nodeStackSize] = firstChild;
				nodeStack[nodeStackSize + 1u] = firstChild + 1u;
			}

			nodeStackSize += 2u;
		}
	}

	return result;
}

const backgroundColor = vec3(0.0);

fn raytraceMonteCarlo(ray : Ray, randomState : ptr<function, RandomState>) -> vec3f {
	var accumulatedColor = vec3f(0.0);
	var colorFactor = vec3f(1.0);

	var currentRay = ray;

	for (var rayDepth = 0u; rayDepth < 4u; rayDepth += 1u) {
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
