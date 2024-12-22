use math.wgsl;

const MAX_ENV_MAP_INTENSITY = 100.0;

fn sampleEnvMap(environmentMap: texture_storage_2d<rgba32float, read>, direction : vec3f) -> vec3f {
	let dimensions = vec2f(textureDimensions(environmentMap));
	let x = atan2(direction.z, direction.x) / PI * 0.5 + 0.5;
	let y = -atan2(direction.y, length(direction.xz)) / PI + 0.5;

	return min(vec3f(MAX_ENV_MAP_INTENSITY), textureLoad(environmentMap, vec2u(dimensions * vec2f(x, y))).xyz);
}
