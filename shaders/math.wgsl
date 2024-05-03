const PI = 3.14159265358979323846;

fn completeBasis(Z : vec3f) -> mat3x3f {
	var X = select(vec3f(1.0, 0.0, 0.0), vec3f(0.0, 1.0, 0.0), abs(Z.x) > 0.5);
	X -= Z * dot(X, Z);
	let Y = cross(Z, X);
	return mat3x3f(X, Y, Z);
}
