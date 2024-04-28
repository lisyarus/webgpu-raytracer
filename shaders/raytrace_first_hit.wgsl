@vertex
fn vertexMain(@builtin(vertex_index) index : u32) -> @builtin(position) vec4f {

	// Hard-coded triangle that encloses the whole screen

	if (index == 0u) {
		return vec4f(-1.0, -1.0, 0.0, 1.0);
	} else if (index == 1u) {
		return vec4f( 3.0, -1.0, 0.0, 1.0);
	} else {
		return vec4f(-1.0,  3.0, 0.0, 1.0);
	}
}

@fragment
fn fragmentMain() -> @location(0) vec4f {
	return vec4f(1.0, 0.0, 1.0, 0.001);
}
