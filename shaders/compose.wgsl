@group(0) @binding(0) var accumulationTexture : texture_2d<f32>;

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

fn acesToneMap(color : vec3f) -> vec3f {
	let A = 2.51;
	let B = 0.03;
	let C = 2.43;
	let D = 0.59;
	let E = 0.14;

	return saturate((color * (A * color + B)) / (color * (C * color + D) + E));
}

fn gammaCorrect(color : vec3f) -> vec3f {
	return pow(color, vec3f(1.0 / 2.2));
}

@fragment
fn fragmentMain(@builtin(position) fragmentPosition : vec4f) -> @location(0) vec4f {
	let linearColor = textureLoad(accumulationTexture, vec2i(fragmentPosition.xy), 0).rgb;
	return vec4f(gammaCorrect(acesToneMap(linearColor)), 1.0);
}
