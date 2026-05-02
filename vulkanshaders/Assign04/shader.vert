#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec4 color;
layout(location = 2) in vec3 normal;

layout(location = 0) out vec4 interColor;
layout(location = 1) out vec3 interPos; 
layout(location = 2) out vec3 interNormal;

layout(push_constant) uniform PushConstants {
    mat4 modelMat;
	float useNormalAsColor;
} pc;

layout(set = 0, binding = 0) uniform UBOVertex { 
	mat4 viewMat; 
	mat4 projMat; 
} ubo;

void main()
{
	vec4 pos = vec4(position, 1.0); 
	pos = pc.modelMat*pos; 
	interPos = vec3(pos);
	gl_Position = ubo.projMat*ubo.viewMat*pos;

	vec3 world_normal = normalize(mat3(pc.modelMat) * normal);
	vec4 color_normal = vec4((world_normal + vec3(1.0)) / 2.0, 1.0);

	interColor = color;

	mat3 normalMat = transpose(inverse(mat3(pc.modelMat))); 
	interNormal = normalMat*normal;

}
