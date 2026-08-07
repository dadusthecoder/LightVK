#version 460
#extension GL_EXT_descriptor_heap : require
#extension GL_EXT_nonuniform_qualifier : require

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragUV;

layout(location = 0) out vec4 outColor;
layout(descriptor_heap) uniform texture2D textures_heap[];
layout(descriptor_heap) uniform sampler samplers_heap[];

layout(push_constant) uniform PushData {
    uint frameIndex;
    uint vertexBufferIndex;
    uint indexBufferIndex;
    uint indexOffset;
    uint materialIndex;
    uint materialBufferIndex;
    mat4 transform;
}
pushData;

struct Material {
    vec4 baseColorFactor;
    vec4 emissiveFactor;
    vec4 scalarFactors;
    uint baseColorTexture;
    uint baseColorSampler;
    uint normalTexture;
    uint normalSampler;
    uint metallicRoughnessTexture;
    uint metallicRoughnessSampler;
    uint occlusionTexture;
    uint occlusionSampler;
    uint emissiveTexture;
    uint emissiveSampler;
};

layout(std430, descriptor_heap) buffer MaterialBuffer {
    Material materials[];
}
materialHeaps[];

void main() {
    Material material = materialHeaps[pushData.materialBufferIndex].materials[pushData.materialIndex];
    vec4 texColor = material.baseColorFactor;
    if (material.baseColorTexture != 0xFFFFFFFFu && material.baseColorSampler != 0xFFFFFFFFu) {
        texColor *= texture(sampler2D(textures_heap[nonuniformEXT(material.baseColorTexture)],
                                       samplers_heap[nonuniformEXT(material.baseColorSampler)]), fragUV);
    }

    outColor = vec4(texColor.rgb + material.emissiveFactor.rgb * material.emissiveFactor.a, texColor.a);
}
