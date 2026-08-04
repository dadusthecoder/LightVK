#version 460
#extension GL_EXT_descriptor_heap : require
#extension GL_EXT_nonuniform_qualifier : require

layout(push_constant) uniform PushData {
    uint vertexBufferIndex;
    uint indexBufferIndex;
    uint textureIndex;
    uint samplerIndex;
    vec2 translate;
    vec2 resolution;
} pushData;

layout(location = 0) in vec4 inColor;
layout(location = 1) in vec2 inTexCoord;

layout(location = 0) out vec4 outColor;

layout(descriptor_heap) uniform texture2D textures[];
layout(descriptor_heap) uniform sampler samplers[];

void main() {
    vec4 texColor = texture(sampler2D(textures[pushData.textureIndex], samplers[pushData.samplerIndex]), inTexCoord);
    outColor = inColor * texColor;
}
