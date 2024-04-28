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
