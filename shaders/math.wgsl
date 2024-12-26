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

fn perspectiveDivide(v : vec4f) -> vec3f {
	return v.xyz / v.w;
}

fn det(v0 : vec3f, v1 : vec3f, v2 : vec3f) -> f32 {
	return dot(v0, cross(v1, v2));
}

fn solve3x3Equation(matrix : mat3x3f, rhs : vec3f) -> vec3f {
	// Use Cramer's rule to solve the system
	let d = det(matrix[0], matrix[1], matrix[2]);
	let d0 = det(rhs, matrix[1], matrix[2]);
	let d1 = det(matrix[0], rhs, matrix[2]);
	let d2 = det(matrix[0], matrix[1], rhs);

	return vec3f(d0, d1, d2) / d;
}
