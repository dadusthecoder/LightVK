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
    uint indexOffset;
    uint materialIndex;
    uint materialBufferIndex;
    uint isWireframe;
    uint padding;
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

layout(location = 0) out vec2 fragUV;
layout(location = 1) out vec3 fragNormal;

void main() {
    uint vertexIdx = indexHeaps[pushData.indexBufferIndex].indices[pushData.indexOffset + gl_VertexIndex];

    Vertex vertex = vertexHeaps[pushData.vertexBufferIndex].vertices[nonuniformEXT(vertexIdx)];

    fragUV = vertex.uv;

    // Transform normal into world space (assuming uniform scale for simplicity)
    vec3 normal = mat3(pushData.transform) * vertex.normal;
    normal      = normalize(normal);
    fragNormal  = normal;

    // Reconstruct the view-projection matrix column by column from the descriptor heap
    // to bypass the driver TDR bug reading mat4 directly from UBO arrays
    mat4 viewProj = mat4(uniformHeaps[pushData.frameIndex].viewProjCol0,
                         uniformHeaps[pushData.frameIndex].viewProjCol1,
                         uniformHeaps[pushData.frameIndex].viewProjCol2,
                         uniformHeaps[pushData.frameIndex].viewProjCol3);

    gl_Position = viewProj * pushData.transform * vec4(vertex.position, 1.0);
}
