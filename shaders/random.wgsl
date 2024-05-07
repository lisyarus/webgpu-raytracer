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

// Slightly biased if (2^32 % max) != 0
fn uniformUint(state: ptr<function, RandomState>, max : u32) -> u32 {
	(*state).value = pcg((*state).value);
	return ((*state).value % max);
}

fn uniformFloat(state: ptr<function, RandomState>) -> f32 {
	(*state).value = pcg((*state).value);
	return f32((*state).value) / 4294967295.0;
}

// Not actually used in sampling, can be used for debugging
// as ground-truth sampling with probability 1/(4*PI)
// (works really bad for low-roughness specular lobes, though)
fn uniformSphere(state: ptr<function, RandomState>) -> vec3f {
	let theta = acos(2.0 * uniformFloat(state) - 1.0);
	let phi = 2.0 * PI * uniformFloat(state);

	return sin(theta) * (cos(phi) * vec3f(1.0, 0.0, 0.0) + sin(phi) * vec3f(0.0, 1.0, 0.0)) + cos(theta) * vec3f(0.0, 0.0, 1.0);
}

// See https://ameye.dev/notes/sampling-the-hemisphere/
fn uniformHemisphere(state: ptr<function, RandomState>, normal : vec3f) -> vec3f {
	let theta = acos(uniformFloat(state));
	let phi = 2.0 * PI * uniformFloat(state);

	let basis = completeBasis(normal);
	return sin(theta) * (cos(phi) * basis[0] + sin(phi) * basis[1]) + cos(theta) * basis[2];
}

fn cosineHemisphere(state: ptr<function, RandomState>, normal : vec3f) -> vec3f {
	// See https://ameye.dev/notes/sampling-the-hemisphere/
	let theta = acos(sqrt(uniformFloat(state)));
	let phi = 2.0 * PI * uniformFloat(state);

	let basis = completeBasis(normal);
	return sin(theta) * (cos(phi) * basis[0] + sin(phi) * basis[1]) + cos(theta) * basis[2];
}
