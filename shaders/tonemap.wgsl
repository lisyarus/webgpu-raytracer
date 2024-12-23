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
