use math.wgsl;

fn pcg(n: u32) -> u32 {
    var h = n * 747796405u + 2891336453u;
    h = ((h >> ((h >> 28u) + 4u)) ^ h) * 277803737u;
    return (h >> 22u) ^ h;
}

struct RandomState {
	value : u32,
}

fn initRandom(state: ptr<function, RandomState>, value : u32) {
	(*state).value ^= value;
	(*state).value = pcg((*state).value);
}

fn uniformFloat(state: ptr<function, RandomState>) -> f32 {
	(*state).value = pcg((*state).value);
	return f32((*state).value) / 4294967295.0;
}

fn uniformHemisphere(state: ptr<function, RandomState>, normal : vec3f) -> vec3f {
	// See https://ameye.dev/notes/sampling-the-hemisphere/
	let theta = acos(uniformFloat(state));
	let phi = 2.0 * pi * uniformFloat(state);

	let basis = completeBasis(normal);
	return sin(theta) * (cos(phi) * basis[0] + sin(phi) * basis[1]) + cos(theta) * basis[2];
}


fn cosineHemisphere(state: ptr<function, RandomState>, normal : vec3f) -> vec3f {
	// See https://ameye.dev/notes/sampling-the-hemisphere/
	let theta = acos(sqrt(uniformFloat(state)));
	let phi = 2.0 * pi * uniformFloat(state);

	let basis = completeBasis(normal);
	return sin(theta) * (cos(phi) * basis[0] + sin(phi) * basis[1]) + cos(theta) * basis[2];
}
