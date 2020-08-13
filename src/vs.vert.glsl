// Compiled with:
// glslangValidator -V --vn vsSpv ./src/vs.vert.glsl

#version 450

layout (binding = 0) uniform UniformBuffer {
	float time;
} ubo;

layout (location = 0) out vec4 vertexColor;
layout (location = 1) out vec2 texCoord;

vec4 colors[6] = vec4[](
	vec4(1.0, 0.0, 0.0, 1.0),
	vec4(0.0, 1.0, 0.0, 1.0),
	vec4(0.0, 0.0, 1.0, 1.0),

	vec4(0.0, 0.0, 1.0, 1.0),
	vec4(1.0, 0.0, 1.0, 1.0),
	vec4(1.0, 0.0, 0.0, 1.0) 
);

vec2 texCoords[6] = vec2[](
	vec2(0.0, 1.0),
	vec2(1.0, 1.0),
	vec2(1.0, 0.0),

	vec2(1.0, 0.0),
	vec2(0.0, 0.0),
	vec2(0.0, 1.0)
);

vec2 positions[6] = vec2[](
	vec2(-1.0,  1.0),
	vec2( 1.0,  1.0),
	vec2( 1.0, -1.0),

	vec2( 1.0, -1.0),
	vec2(-1.0, -1.0),
	vec2(-1.0,  1.0)
);

void main()
{
	gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);

	float sT = abs(sin(ubo.time));
	if (sT < 0.07)
		sT = 0.07;

	vertexColor = colors[gl_VertexIndex] * vec4(vec3(sT), 1.0);
	texCoord = texCoords[gl_VertexIndex];
}
