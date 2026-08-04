#version 460
#extension GL_EXT_descriptor_heap : require
#extension GL_EXT_nonuniform_qualifier : require

struct RmlVertex {
    vec2 position;
    uint colorPacked;
    float _pad;
    vec2 texcoord;
};

layout(push_constant) uniform PushData {
    uint vertexBufferIndex;
    uint indexBufferIndex;
    uint textureIndex;
    uint samplerIndex;
    vec2 translate;
    vec2 resolution;
} pushData;

layout(std430, descriptor_heap) buffer VertexBuffer {
    RmlVertex vertices[];
} vertexHeaps[];

layout(std430, descriptor_heap) buffer IndexBuffer {
    uint indices[];
} indexHeaps[];

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec2 outTexCoord;

void main() {
    // Programmable pulling: fetch index, then fetch vertex
    uint vertexIndex = indexHeaps[pushData.indexBufferIndex].indices[gl_VertexIndex];
    RmlVertex v = vertexHeaps[pushData.vertexBufferIndex].vertices[vertexIndex];

    // Unpack color (RGBA packed as uint)
    uint c = v.colorPacked;
    vec4 color = vec4(
        float(c & 0xFF) / 255.0,
        float((c >> 8) & 0xFF) / 255.0,
        float((c >> 16) & 0xFF) / 255.0,
        float((c >> 24) & 0xFF) / 255.0
    );

    // Apply translation
    vec2 pos = v.position + pushData.translate;

    // Convert to NDC
    // RmlUi coordinates: (0,0) is top-left, (resolution.x, resolution.y) is bottom-right
    // Vulkan NDC: (-1,-1) is top-left, (1,1) is bottom-right
    vec2 ndcPos = (pos / pushData.resolution) * 2.0 - 1.0;
    
    gl_Position = vec4(ndcPos, 0.0, 1.0);
    outColor = color;
    outTexCoord = v.texcoord;
}
