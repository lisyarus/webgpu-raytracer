fn AcesTonemap(color : vec3f) -> vec3f {
	let A = 2.51;
	let B = 0.03;
	let C = 2.43;
	let D = 0.59;
	let E = 0.14;

	return saturate((color * (A * color + B)) / (color * (C * color + D) + E));
}

fn Uncharted2TonemapBase(x : vec3f) -> vec3f
{
	let A = 0.15;
	let B = 0.50;
	let C = 0.10;
	let D = 0.20;
	let E = 0.02;
	let F = 0.30;
	let W = 11.2;

	return ((x*(A*x+C*B)+D*E)/(x*(A*x+B)+D*F))-E/F;
}

fn Uncharted2Tonemap(color : vec3f) -> vec3f
{
	let white = vec3f(10.0);
	return Uncharted2TonemapBase(color) / Uncharted2TonemapBase(white);
}

fn tonemap(color : vec3f) -> vec3f
{
	return AcesTonemap(color);
}

fn gammaCorrect(color : vec3f) -> vec3f {
	// No need to do gamma-correction since we're using sRGB surface format
	return color;
	//return pow(color, vec3f(1.0 / 2.2));
}
