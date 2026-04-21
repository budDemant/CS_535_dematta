#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec4 color;
layout(location = 2) in vec3 normal;

layout(location = 0) out vec4 interColor;

layout(push_constant) uniform PushConstants {
    mat4 modelMat;
	float useNormalAsColor;
} pc;

void main()
{
	vec4 pos = vec4(position, 1.0);
	gl_Position = pc.modelMat * pos;

	vec3 world_normal = normalize(mat3(pc.modelMat) * normal);
	vec4 color_normal = vec4((world_normal + vec3(1.0)) / 2.0, 1.0);

	interColor = mix(color, color_normal, pc.useNormalAsColor);
}
