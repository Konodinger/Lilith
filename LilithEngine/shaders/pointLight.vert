#version 450

const vec2 OFFSETS[6] = vec2[] (
	vec2(-1.0, -1.0),
	vec2(-1.0, 1.0),
	vec2(1.0, -1.0),
	vec2(1.0, -1.0),
	vec2(-1.0, 1.0),
	vec2(1.0, 1.0)
);

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
	vec4 position;
	vec4 color;
	float radius;
} push;

layout (location = 0) out vec2 fragOffset;


void main() {
	fragOffset = OFFSETS[gl_VertexIndex];
	vec4 positionCameraSpace = globUbo.viewMatrix * push.position;
	positionCameraSpace.xy += push.radius * fragOffset;

	gl_Position = globUbo.projectionMatrix * positionCameraSpace;
}