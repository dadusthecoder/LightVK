#version 460
#extension GL_EXT_descriptor_heap : require
#extension GL_EXT_nonuniform_qualifier : require

struct Vertex {
    vec3 position;
    vec3 normal;
    vec2 uv;
    vec4 tangent;
};

layout(push_constant) uniform PushData {
    uint frameIndex;
    uint vertexBufferIndex;
    uint indexBufferIndex;
    uint materialIndex;
    mat4 transform;
}
pushData;

layout(std430, descriptor_heap) buffer VertexBuffer {
    Vertex vertices[];
}
vertexHeaps[];

layout(std430, descriptor_heap) buffer IndexBuffer {
    uint indices[];
}
indexHeaps[];

layout(std140, descriptor_heap) uniform UBO {
    vec4 viewProjCol0;
    vec4 viewProjCol1;
    vec4 viewProjCol2;
    vec4 viewProjCol3;
}
uniformHeaps[];

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragUV;

void main() {
    uint vertexIdx = indexHeaps[pushData.indexBufferIndex].indices[gl_VertexIndex];

    vec3 pos    = vertexHeaps[pushData.vertexBufferIndex].vertices[nonuniformEXT(vertexIdx)].position;
    
    // Transform normal into world space (assuming uniform scale for simplicity)
    vec3 normal = mat3(pushData.transform) * vertexHeaps[pushData.vertexBufferIndex].vertices[nonuniformEXT(vertexIdx)].normal;
    normal = normalize(normal);

    // Reconstruct the view-projection matrix column by column from the descriptor heap
    // to bypass the driver TDR bug reading mat4 directly from UBO arrays
    mat4 viewProj = mat4(
        uniformHeaps[pushData.frameIndex].viewProjCol0,
        uniformHeaps[pushData.frameIndex].viewProjCol1,
        uniformHeaps[pushData.frameIndex].viewProjCol2,
        uniformHeaps[pushData.frameIndex].viewProjCol3
    );

    gl_Position = viewProj * pushData.transform * vec4(pos, 1.0);

    // Basic directional lighting for visual feedback
    vec3 lightDir = normalize(vec3(0.5, 1.0, 0.3));
    float ndotl   = max(dot(normal, lightDir), 0.0);
    
    // Add a checkerboard pattern using the object-space position
    float checker = mod(floor(pos.x * 2.0) + floor(pos.y * 2.0) + floor(pos.z * 2.0), 2.0);
    vec3 baseColor = mix(vec3(0.8, 0.2, 0.2), vec3(0.2, 0.2, 0.8), checker); // Red and blue checkerboard

    fragColor     = baseColor * (ndotl * 0.7 + 0.3); // ambient + diffuse
}