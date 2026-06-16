#version 450

layout(push_constant) uniform PushConstants {
    int layer_count;
    int input_size;
    int output_size;
    int hidden_sizes[8];
    float resX;
    float resY;
} pc;

#define ACTIVATION(x) max(0.0, x)
float sigmoid(float x) { return 1.0 / (1.0 + exp(-x)); }
#define MAX_SIZE 64

layout(std430, set = 0, binding = 0) readonly buffer WeightsBuffer {
    float weights[];
} weightsBuffer;

layout(location = 0) in vec2 fragCoord;
layout(location = 0) out vec4 outColor;

void main() {
    // Фиксированные массивы максимального размера
    int sizes[10];
    sizes[0] = pc.input_size;
    for (int i = 0; i < pc.layer_count - 1; i++)
        sizes[i + 1] = pc.hidden_sizes[i];
    sizes[pc.layer_count] = pc.output_size;

    float cur[MAX_SIZE];
    float nxt[MAX_SIZE];

    vec2 uv = gl_FragCoord.xy;
    cur[0] = uv.x / pc.resX;
    cur[1] = uv.y / pc.resY;

    int offset = 0;

    for (int layer = 0; layer < pc.layer_count; layer++) {
        int inSize  = sizes[layer];
        int outSize = sizes[layer + 1];

        for (int j = 0; j < outSize; j++) {
            float sum = 0.0;
            for (int i = 0; i < inSize; i++)
                sum += cur[i] * weightsBuffer.weights[offset + i * outSize + j];
            sum += weightsBuffer.weights[offset + inSize * outSize + j];

            bool isLast = (layer == pc.layer_count - 1);
            nxt[j] = isLast ? sigmoid(sum) : ACTIVATION(sum);
        }

        offset += inSize * outSize + outSize;

        for (int j = 0; j < outSize; j++)
            cur[j] = nxt[j];
    }

    outColor = vec4(cur[0], cur[1], cur[2], 1.0);
}
