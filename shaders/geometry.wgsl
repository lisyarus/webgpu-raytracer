struct Vertex
{
	position : vec3f,
	// ... 4-byte padding ...
	normal : vec3f,
	materialID : u32,
}

fn defaultVertex() -> Vertex {
	return Vertex(vec3f(0.0), vec3f(0.0), 0u);
}

struct BVHNode
{
	// Cannot declare min, max : vec3f
	// because of vec3f alignment
	minX : f32,
	minY : f32,
	minZ : f32,
	maxX : f32,
	maxY : f32,
	maxZ : f32,
	leftChildOrFirstTriangle : u32,
	triangleCount : u32,
}

const MAX_BVH_DEPTH = 16u;
const MAX_BVH_STACK_SIZE = MAX_BVH_DEPTH + 1u;
const BVH_NODE_AXIS_MASK = 3u << 30u;
const BVH_NODE_AXIS_SHIFT = 30u;
