use math.wgsl;
use random.wgsl;

// Schlick's approximation for Fresnel equations
fn fresnel(f0 : vec3f, f90 : vec3f, VdotH : f32) -> vec3f {
	return f0 + (f90 - f0) * pow(max(0.0, 1.0 - abs(VdotH)), 5.0);
}

// Cook-Torrance BRDF with GGX normal distribution & Smith geometry term + transmission
// See
//     https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#appendix-b-brdf-implementation
//     https://github.com/KhronosGroup/glTF/blob/main/extensions/2.0/Khronos/KHR_materials_transmission/README.md#implementation-notes
//     https://www.cs.cornell.edu/~srm/publications/EGSR07-btdf.pdf
//     https://dassaultsystemes-technology.github.io/EnterprisePBRShadingModel/spec-2022x.md.html
fn cookTorranceGGX(N : vec3f, L : vec3f, V : vec3f, baseColor : vec3f, metallic : f32, roughness : f32, ior : f32, transmission : f32) -> vec3f {
	let Lt = L - 2.0 * N * dot(L, N);

	let H = normalize(V + L);
	let Ht = normalize(V + Lt);

	let VdotN = dot(V, N); // always >= 0.0
	let VdotH = dot(V, H); // == LdotH
	let LdotN = dot(L, N);
	let NdotH = dot(N, H);

	let VdotHt = dot(V, Ht);
	let LdotHt = dot(L, Ht);
	let NdotHt = dot(N, Ht);

	let alpha = roughness * roughness;
	let alpha2 = alpha * alpha;

	// normal Distribution term
	let D = alpha2 / PI / pow(NdotH * NdotH * (alpha2 - 1.0) + 1.0, 2.0) * chiPlus(NdotH);
	let Dt = alpha2 / PI / pow(NdotHt * NdotHt * (alpha2 - 1.0) + 1.0, 2.0) * chiPlus(NdotHt);

	// Visibility x Geometry term
	let visV  = 1.0 / (abs(VdotN) + sqrt(alpha2 + (1.0 - alpha2) * VdotN * VdotN));
	let visL  = 1.0 / (abs(LdotN) + sqrt(alpha2 + (1.0 - alpha2) * LdotN * LdotN));

	let vis = visV * visL;

	let specularBrdf = vec3f(D * vis) * chiPlus(VdotN) * chiPlus(LdotN);

	let f0 = pow((1.0 - ior) / (1.0 + ior), 2.0);

	let transmissionFresnel = fresnel(vec3f(f0), vec3f(1.0), VdotHt);
	let transmissionBtdf = baseColor * Dt * vis * chiPlus(-LdotN) * chiPlus(VdotN);

	let metallicFresnel = fresnel(baseColor, vec3f(1.0), VdotH);
	let metallicBrdf = metallicFresnel * specularBrdf;

	let diffuseBrdf = baseColor / PI * chiPlus(LdotN);

	let dielectricFresnel = fresnel(vec3f(f0), vec3f(1.0), VdotH);

	let opaqueDielectricBrdf = mix(diffuseBrdf, specularBrdf, dielectricFresnel);
	let transparentDielectricBrdf = specularBrdf * dielectricFresnel + transmissionBtdf * (1.0 - transmissionFresnel);

	let dielectricBrdf = mix(opaqueDielectricBrdf, transparentDielectricBrdf, transmission);

	let resultBrdf = mix(dielectricBrdf, metallicBrdf, metallic);

	return resultBrdf;
}

// Reflected direction sampling algorithm well-suited for the Cook-Torrance specular term, see
//    Eric Heitz, Sampling the GGX Distribution of Visible Normals (2018)
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

fn sampleTransmissionVNDF(randomState : ptr<function, RandomState>, N : vec3f, V : vec3f, roughness : f32) -> vec3f {
	let reflectedDirection = sampleVNDF(randomState, N, V, roughness);
	return reflectedDirection - 2.0 * dot(reflectedDirection, N) * N;
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

	let D = alpha2 / PI / pow((alpha2 - 1.0) * H.z * H.z + 1.0, 2.0) * chiPlus(H.z);

	let visV = 2.0 * abs(Vlocal.z) / (abs(Vlocal.z) + sqrt(alpha2 + (1.0 - alpha2) * Vlocal.z * Vlocal.z)) * chiPlus(Vlocal.z);

	return D * visV / 4.0 / Vlocal.z;
}

fn probabilityTransmissionVNDF(N : vec3f, V : vec3f, L : vec3f, roughness : f32) -> f32 {
	let reflectedDirection = L - 2.0 * dot(L, N) * N;
	return probabilityVNDF(N, V, reflectedDirection, roughness);
}
