struct Camera
{
	viewProjectionMatrix : mat4x4f,
	viewProjectionInverseMatrix : mat4x4f,
	screenSize : vec2u,
	frameID : u32,
}
