#version 450
layout(push_constant) uniform PC {
    float cx;
    float cy;
    float radius;
    float r, g, b;
} pc;

layout(location = 0) in  vec2 fragUV;
layout(location = 0) out vec4 outColor;

void main()
{
    float dist = length(fragUV);

    float edge = 0.02 / pc.radius;          // ширина сглаживания (в нормализованных ед.)
    float borderWidth = 0.005 / pc.radius;   // ширина чёрной обводки

    // Внешняя граница круга (с AA)
    float outerAlpha = 1.0 - smoothstep(1.0 - edge, 1.0, dist);

    // Граница между заливкой и обводкой
    float innerEdge = 1.0 - borderWidth;
    float fillMask  = 1.0 - smoothstep(innerEdge - edge, innerEdge, dist);

    // Цвет: внутри — заданный, в полосе обводки — чёрный
    vec3 color = mix(vec3(0.0), vec3(pc.r, pc.g, pc.b), fillMask);

    if (outerAlpha < 0.001) discard;
    outColor = vec4(color, outerAlpha);
}