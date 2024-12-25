// N.B.: this file expects that the following global arrays are defined:
//     vertexPositions
//     bvhNodes
//     emissiveTriangles
//     emissiveBvhNodes

struct SceneIntersection
{
	intersects : bool,
	distance : f32,
	triangleID : u32,
	vertices : array<vec3f, 3>,
	uv : vec2f,
	visitedNodeCount : u32,
	intersectedNodeCount : u32,
}

fn intersectScene(ray : Ray) -> SceneIntersection {
	var result = SceneIntersection(
		false,
		1e30,
		0u,
		array<vec3f, 3>(vec3f(0.0), vec3f(0.0), vec3f(0.0)),
		vec2f(0.0),
		0u,
		0u
	);

	var nodeStack = array<u32, MAX_BVH_STACK_SIZE>();
	var nodeStackSize = 0u;
	var currentNodeID = 0u;

	while (true) {
		result.visitedNodeCount += 1u;

		let node = bvhNodes[currentNodeID];

		let leftChildOrFirstTriangle = bitcast<u32>(node.aabbMin.w);
		let triangleCount = bitcast<u32>(node.aabbMax.w);

		let hit = intersectRayAABB(ray, node.aabbMin.xyz, node.aabbMax.xyz);

		if (!hit.intersects || hit.distance > result.distance) {
			if (nodeStackSize > 0u) {
				currentNodeID = nodeStack[nodeStackSize - 1u];
				nodeStackSize -= 1u;
				continue;
			} else {
				break;
			}
		}

		result.intersectedNodeCount += 1u;

		if (triangleCount > 0) {
			for (var i = 0u; i < triangleCount; i += 1u) {
				let triangleID = leftChildOrFirstTriangle + i;

				let v0 = vertexPositions[3 * triangleID + 0u].xyz;
				let v1 = vertexPositions[3 * triangleID + 1u].xyz;
				let v2 = vertexPositions[3 * triangleID + 2u].xyz;

				let hit = intersectRayTriangle(ray, v0, v1, v2);
				if (hit.intersects && hit.distance < result.distance) {
					result.intersects = true;
					result.distance = hit.distance;
					result.triangleID = triangleID;
					result.vertices[0] = v0;
					result.vertices[1] = v1;
					result.vertices[2] = v2;
					result.uv = hit.uv;
				}
			}

			if (nodeStackSize > 0u) {
				currentNodeID = nodeStack[nodeStackSize - 1u];
				nodeStackSize -= 1u;
				continue;
			} else {
				break;
			}
		} else {
			let firstChild = leftChildOrFirstTriangle & (~BVH_NODE_AXIS_MASK);

			// Ordered traversal: visit closer child first (i.e. put closer child higher on the stack)

			let nodeAxis = leftChildOrFirstTriangle >> BVH_NODE_AXIS_SHIFT;

			let positive = u32(ray.direction[nodeAxis] > 0.0);

			nodeStack[nodeStackSize] = firstChild + positive;
			currentNodeID = firstChild + (1u - positive);

			nodeStackSize += 1u;
		}
	}

	return result;
}

fn lightSamplingProbability(ray : Ray) -> f32 {
	if (emissiveTriangles.count.x == 0u) {
		return 0.0;
	}

	var result = 0.0;

	var nodeStack = array<u32, MAX_BVH_STACK_SIZE>();
	var nodeStackSize = 0u;
	var currentNodeID = 0u;

	while (true) {
		let node = emissiveBvhNodes[currentNodeID];

		let leftChildOrFirstTriangle = bitcast<u32>(node.aabbMin.w);
		let triangleCount = bitcast<u32>(node.aabbMax.w);

		let hit = intersectRayAABB(ray, node.aabbMin.xyz, node.aabbMax.xyz);

		if (!hit.intersects) {
			if (nodeStackSize > 0u) {
				currentNodeID = nodeStack[nodeStackSize - 1u];
				nodeStackSize -= 1u;
				continue;
			} else {
				break;
			}
		}

		if (triangleCount > 0) {
			for (var i = 0u; i < triangleCount; i += 1u) {
				let triangleIndex = leftChildOrFirstTriangle + i;
				let triangleData = emissiveTriangles.triangles[triangleIndex];
				let triangleID = triangleData.x;
				let triangleWeight = bitcast<f32>(triangleData.y);

				let v0 = vertexPositions[3 * triangleID + 0u].xyz;
				let v1 = vertexPositions[3 * triangleID + 1u].xyz;
				let v2 = vertexPositions[3 * triangleID + 2u].xyz;

				let hit = intersectRayTriangle(ray, v0, v1, v2);
				if (hit.intersects) {
					let c = cross(v1 - v0, v2 - v0);
					let l = length(c);
					let n = c / l;

					let area = l * 0.5;

					result += (1.0 / area) * hit.distance * hit.distance / max(1e-8, abs(dot(ray.direction, n))) * triangleWeight;
				}
			}

			if (nodeStackSize > 0u) {
				currentNodeID = nodeStack[nodeStackSize - 1u];
				nodeStackSize -= 1u;
				continue;
			} else {
				break;
			}
		} else {
			let firstChild = leftChildOrFirstTriangle & (~BVH_NODE_AXIS_MASK);

			// Ordered traversal: visit closer child first (i.e. put closer child higher on the stack)

			let nodeAxis = leftChildOrFirstTriangle >> BVH_NODE_AXIS_SHIFT;

			let positive = u32(ray.direction[nodeAxis] > 0.0);

			nodeStack[nodeStackSize] = firstChild + positive;
			currentNodeID = firstChild + (1u - positive);

			nodeStackSize += 1u;
		}
	}

	return result;
}
