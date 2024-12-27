use tonemap.wgsl;

struct ComposeUniforms
{
	exposure : f32,
}

@group(0) @binding(0) var accumulationTexture : texture_2d<f32>;
@group(1) @binding(0) var<uniform> composeUniforms : ComposeUniforms;

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
fn fragmentMain(@builtin(position) fragmentPosition : vec4f) -> @location(0) vec4f {
	let linearColor = textureLoad(accumulationTexture, vec2i(fragmentPosition.xy), 0).rgb;
	return vec4f(gammaCorrect(tonemap(linearColor * composeUniforms.exposure)), 1.0);
}
