struct Vertex
{
	normal : vec3f,
	materialID : u32,
	texcoord : vec2f,
}

struct BVHNode
{
	// leftChildOrFirstTriangle is aabbMin.w as uint
	aabbMin : vec4f,
	// triangleCount is aabbMax.w as uint
	aabbMax : vec4f,
}

const MAX_BVH_DEPTH = 32u;
const MAX_BVH_STACK_SIZE = MAX_BVH_DEPTH + 1u;
const BVH_NODE_AXIS_MASK = 3u << 30u;
const BVH_NODE_AXIS_SHIFT = 30u;

struct TriangleArray {
	// count.y is unused
	count : vec2u,

	// triangle.x is triangle ID
	// triangle.y is triangle weight (sampling probability)
	triangles : array<vec2u>,
}
