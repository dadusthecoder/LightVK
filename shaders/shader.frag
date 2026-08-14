#version 460
#extension GL_EXT_descriptor_heap : require
#extension GL_EXT_nonuniform_qualifier : require

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
    uint isWireframe;
    uint padding;
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
    uint padding[2];
};

layout(std430, descriptor_heap) buffer MaterialBuffer {
    Material materials[];
}
materialHeaps[];

layout(location = 0) in vec2 fragUV;
layout(location = 1) in vec3 fragNormal;

void main() {
    if (pushData.isWireframe == 1) {
        outColor = vec4(0.2, 1.0, 0.2, 1.0); // Bright green
        return;
    }

    Material material = materialHeaps[pushData.materialBufferIndex].materials[pushData.materialIndex];

    vec4 texColor = material.baseColorFactor;
    if (material.baseColorTexture != 0xFFFFFFFFu && material.baseColorSampler != 0xFFFFFFFFu) {
        texColor *= texture(sampler2D(textures_heap[nonuniformEXT(material.baseColorTexture)],
                                       samplers_heap[nonuniformEXT(material.baseColorSampler)]),
                            fragUV);
    }

    vec3 lightDir = normalize(vec3(0.5, 1.0, 0.3));
    float diff = max(dot(normalize(fragNormal), lightDir), 0.0);
    vec3 ambient = vec3(0.2);
    vec3 lighting = ambient + diff * vec3(0.8);
    
    outColor = vec4(texColor.rgb * lighting + material.emissiveFactor.rgb * material.emissiveFactor.a, texColor.a);
}
