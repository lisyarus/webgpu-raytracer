const PI = 3.14159265358979323846;

// Heaviside step function
fn chiPlus(x : f32) -> f32 {
	return step(0.0, x);
}

fn completeBasis(Z : vec3f) -> mat3x3f {
	var X = select(vec3f(1.0, 0.0, 0.0), vec3f(0.0, 1.0, 0.0), abs(Z.x) > 0.5);
	X -= Z * dot(X, Z);
	X = normalize(X);
	let Y = cross(Z, X);
	return mat3x3f(X, Y, Z);
}
