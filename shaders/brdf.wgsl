use math.wgsl;
use random.wgsl;

fn fresnel(f0 : vec3f, f90 : vec3f, VdotH : f32) -> vec3f {
	return f0 + (f90 - f0) * pow(max(0.0, 1.0 - VdotH), 5.0);
}

fn cookTorranceGGX(N : vec3f, L : vec3f, V : vec3f, baseColor : vec3f, metallic : f32, roughness : f32) -> vec3f {
	let H = normalize(L + V);

	let VdotN = dot(V, N);
	let VdotH = dot(V, H); // == LdotH
	let LdotN = dot(L, N);
	let NdotH = dot(N, H);

	let alpha = roughness * roughness;
	let alpha2 = alpha * alpha;

	// normal Distribution term
	let D = alpha2 / PI / pow(NdotH * NdotH * (alpha2 - 1.0) + 1.0, 2.0) * select(0.0, 1.0, NdotH > 0.0);

	// Visibility + Geometry term
	let visV = 1.0 / (abs(VdotN) + sqrt(alpha2 + (1.0 - alpha2) * VdotN * VdotN)) * select(0.0, 1.0, VdotN > 0.0);
	let visL = 1.0 / (abs(LdotN) + sqrt(alpha2 + (1.0 - alpha2) * LdotN * LdotN)) * select(0.0, 1.0, LdotN > 0.0);

	let Vis = visV * visL;

	let specularBrdf = vec3f(D * Vis);

	let metallicFresnel = fresnel(baseColor, vec3f(1.0), VdotH);
	let metallicBrdf = metallicFresnel * specularBrdf;

	let dielectricFresnel = fresnel(vec3f(0.04), vec3f(1.0), VdotH);
	let diffuseBrdf = baseColor / PI;
	let dielectricBrdf = mix(diffuseBrdf, specularBrdf, dielectricFresnel);

	let resultBrdf = mix(dielectricBrdf, metallicBrdf, metallic);

	return resultBrdf;
}

fn sampleVNDF(randomState : ptr<function, RandomState>, N : vec3f, V : vec3f, roughness : f32) -> vec3f {
	let localBasis = completeBasis(N);

	let toGlobal = localBasis;
	let toLocal = transpose(toGlobal);

	let alpha = roughness * roughness;

	let Vlocal = toLocal * V;

	let Vh = normalize(vec3f(alpha, alpha, 1.0) * Vlocal);

	let T1 = normalize(cross(vec3(0.0, 0.0, 1.0), Vh));
	let T2 = cross(Vh, T1);

	let r = sqrt(uniformFloat(randomState));
	let phi = 2.0 * PI * uniformFloat(randomState);
	let t1 = r * cos(phi);
	var t2 = r * sin(phi);
	let s = 0.5 * (1.f + Vh.z);
	t2 = (1.f - s) * sqrt(max(0.0, 1.0 - t1 * t1)) + s * t2;

	let Nh = t1 * T1 + t2 * T2 + sqrt(max(0.0, 1.0 - t1 * t1 - t2 * t2)) * Vh;

	let Ne = normalize(vec3(alpha * Nh.x, alpha * Nh.y, max(0.0, Nh.z)));

	let L = 2.0 * Ne * dot(Ne, Vlocal) - Vlocal;

	return toGlobal * L;
}

fn probabilityVNDF(N : vec3f, V : vec3f, L : vec3f, roughness : f32) -> f32 {
	let localBasis = completeBasis(N);

	let toGlobal = localBasis;
	let toLocal = transpose(toGlobal);

	let alpha = roughness * roughness;
	let alpha2 = alpha * alpha;

	let Vlocal = toLocal * V;
	let Llocal = toLocal * L;
	let H = normalize(Llocal + Vlocal);

	let D = alpha2 / PI / pow((alpha2 - 1.0) * H.z * H.z + 1.0, 2.0) * select(0.0, 1.0, H.z > 0.f);

	let visV = 2.0 * abs(Vlocal.z) / (abs(Vlocal.z) + sqrt(alpha2 + (1.0 - alpha2) * Vlocal.z * Vlocal.z)) * select(0.0, 1.0, Vlocal.z > 0.0);

	return D * visV / 4.0 / Vlocal.z;
}
