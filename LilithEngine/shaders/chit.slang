import Payload;

[shader("closesthit")]
void main(inout HitPayload payload, in BuiltInTriangleIntersectionAttributes attr)
{
    payload.color = float3(1.f, 1.f, 1.f);
}