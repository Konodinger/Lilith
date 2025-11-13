#version 450

layout (location = 0) in vec2 fragOffset;
layout (location = 0) out vec4 outColor;

struct PointLight {
	vec3 position;
	float quadraticAttenuation;
	vec4 color;
};

layout(set = 0, binding = 0) uniform GlobalUbo {
	mat4 projectionMatrix;
	mat4 viewMatrix;
	mat4 inverseViewMatrix;
	vec4 ambientLightColor;
	PointLight pointLights[8];
	int numLights;
} globUbo;

layout(set = 1, binding = 0) uniform GameObjectUbo {
	bool usesColorTexture;
	uint textureId;
} goUbo;

layout(push_constant) uniform Push {
	vec4 position;
	vec4 color;
	float radius;
} push;

void main() {
	float dis = dot(fragOffset, fragOffset);
	if (dis >= 1.0) discard;
	outColor = vec4(push.color.xyz, 2.0 * (dis + 0.5)*(dis - 1.0)*(dis - 1.0));
}