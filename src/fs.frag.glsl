// Compiled with:
// glslangValidator -V --vn fsSpv ./src/fs.frag.glsl

#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (binding = 1) uniform sampler2D textureSampler;

layout (location = 0) in vec4 fragmentIn;
layout (location = 1) in vec2 texCoordIn;

layout (location = 0) out vec4 fragmentOut;

const float sobelMin = 200.0;
const float sobelMax = 300.0;

vec4 sobel(void)
{
	vec4 top         = texture(textureSampler, vec2(texCoordIn.x, texCoordIn.y + 1.0 / sobelMin));
	vec4 bottom      = texture(textureSampler, vec2(texCoordIn.x, texCoordIn.y - 1.0 / sobelMin));
	vec4 left        = texture(textureSampler, vec2(texCoordIn.x - 1.0 / sobelMax, texCoordIn.y));
	vec4 right       = texture(textureSampler, vec2(texCoordIn.x + 1.0 / sobelMax, texCoordIn.y));
	vec4 topLeft     = texture(textureSampler, vec2(texCoordIn.x - 1.0 / sobelMax, texCoordIn.y + 1.0 / sobelMin));
	vec4 topRight    = texture(textureSampler, vec2(texCoordIn.x + 1.0 / sobelMax, texCoordIn.y + 1.0 / sobelMin));
	vec4 bottomLeft  = texture(textureSampler, vec2(texCoordIn.x - 1.0 / sobelMax, texCoordIn.y - 1.0 / sobelMin));
	vec4 bottomRight = texture(textureSampler, vec2(texCoordIn.x + 1.0 / sobelMax, texCoordIn.y - 1.0 / sobelMin));
	vec4 sx = -topLeft - 2 * left - bottomLeft + topRight   + 2 * right  + bottomRight;
	vec4 sy = -topLeft - 2 * top  - topRight   + bottomLeft + 2 * bottom + bottomRight;
	vec4 sobel = sqrt(sx * sx + sy * sy);
	return sobel;
}

void main()
{
	fragmentOut = sobel() * fragmentIn;
}
