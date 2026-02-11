import Payload;

[shader("miss")]
void main(inout HitPayload payload) {
    payload.color = float3(0.1f, 0.1f, 0.1f);
}