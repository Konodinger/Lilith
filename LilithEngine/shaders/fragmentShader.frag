#version 450

layout (location = 0) in vec3 fragPosWorld;
layout (location = 1) in vec3 fragColor;
layout (location = 2) in vec3 fragNormalWorld;
layout (location = 3) in vec2 fragTexCoord;

layout (binding = 1) uniform sampler2D texSampler;

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
} ubo;

layout(push_constant) uniform Push {
	mat4 modelMatrix;
	mat4 normalMatrix;
} push;

layout (location = 0) out vec4 outColor;


void main() {

	vec3 ambientLight = ubo.ambientLightColor.xyz * ubo.ambientLightColor.w;
	vec3 diffuseLight = vec3(0.0);
	vec3 specularLight = vec3(0.0);
	vec3 surfaceNormal = normalize(fragNormalWorld);

	vec3 cameraPosWorld = ubo.inverseViewMatrix[3].xyz;
	vec3 viewDirection = normalize(cameraPosWorld - fragPosWorld);

	for (int i = 0; i < ubo.numLights; ++i) {
		PointLight light = ubo.pointLights[i];

		vec3 directionToLight = light.position - fragPosWorld;
		vec3 normalizedDirectionToLight = normalize(directionToLight);

		float lightIntensity = light.color.w / (1.0 + light.quadraticAttenuation * dot(directionToLight, directionToLight));
		vec3 lightIncoming = light.color.xyz * lightIntensity;
		diffuseLight += lightIncoming * max(0, dot(surfaceNormal, normalizedDirectionToLight));

		vec3 halfAngle = normalize(normalizedDirectionToLight + viewDirection);
		float specularReflection = clamp(dot(surfaceNormal, halfAngle), 0.0, 1.0);
		specularReflection = pow(specularReflection, 32.0);
		specularLight += lightIncoming * specularReflection;
	}



	outColor = vec4((ambientLight + diffuseLight + specularLight) * fragColor, 1.0);
	outColor = texture(texSampler, fragTexCoord);
}