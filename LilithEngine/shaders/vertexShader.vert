#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec2 uv;

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
	int textureId;
} goUbo;

layout(push_constant) uniform Push {
	mat4 modelMatrix;
	mat4 normalMatrix;
} push;

layout(location = 0) out vec3 fragPosWorld;
layout(location = 1) out vec3 fragColor;
layout(location = 2) out vec3 fragNormalWorld;
layout(location = 3) out vec2 fragTexCoord;


void main() {
	vec4 positionWorld = push.modelMatrix * vec4(position, 1.0);
	gl_Position = globUbo.projectionMatrix * globUbo.viewMatrix * positionWorld;

	fragPosWorld = positionWorld.xyz;
	fragColor = color;
	fragNormalWorld = normalize(mat3(push.normalMatrix) * normal);
	fragTexCoord = uv;
}