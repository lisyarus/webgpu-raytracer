// N.B.: this file expects that `vertices` and `bvhNodes` global arrays are defined

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
