import Payload;

struct PointLight {
    float3 position;
    float quadraticAttenuation;
    float4 color;
};

struct GlobalUbo {
    float4x4 projectionMatrix;
    float4x4 viewMatrix;
    float4x4 inverseViewMatrix;
    float4 ambientLightColor;
    PointLight pointLights[8];
    int numLights;
};

struct PushConstant {
    float4x4 modelMatrix;
    float4x4 normalMatrix;
};

[[vk::binding(0, 0)]] ConstantBuffer<GlobalUbo> globUbo;

[[vk::binding(0, 1)]] RaytracingAccelerationStructure tlas;
[[vk::binding(1, 1)]] RWTexture2D<float4> outputImage;

[[vk::push_constant]] PushConstant push;

[shader("raygeneration")]
void main()
{

    float2 launchID = (float2)DispatchRaysIndex().xy;
    float2 launchSize = (float2)DispatchRaysDimensions().xy;

    const uint rayFlags = 0;
    const float2 clipCoords = launchID / launchSize * 2.0 - 1.0;
    const float3 viewCoords = normalize(float3(
        clipCoords.x / globUbo.projectionMatrix[0][0],
        clipCoords.y / globUbo.projectionMatrix[1][1],
        1.f));
    //const float4 viewCoords = mul(float4(clipCoords, 1.0, 1.0), globUbo.projectionMatrix); // REPLACE BY INVERSE PROJ

    RayDesc ray;
    ray.Origin = mul(float4(0.0, 0.0, 0.0, 1.0), globUbo.inverseViewMatrix).xyz;
    ray.Direction = mul(float4(normalize(viewCoords.xyz), 0.0), globUbo.inverseViewMatrix).xyz;
    ray.TMin = 0.001;
    ray.TMax = 1000.f;

    HitPayload payload;
    payload.color = float3(0, 0, 0);

    TraceRay(tlas, rayFlags, 0xff, 0, 0, 0, ray, payload);

    float3 color = payload.color;

    // Write the result to the output image
    outputImage[int2(launchID)] = float4(color, 1.0);
}