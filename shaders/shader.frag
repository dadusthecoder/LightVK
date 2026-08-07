#version 460
#extension GL_EXT_descriptor_heap : require
#extension GL_EXT_nonuniform_qualifier : require

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragUV;

layout(location = 0) out vec4 outColor;
layout(descriptor_heap) uniform texture2D textures_heap[];
layout(descriptor_heap) uniform sampler samplers_heap[];

void main() {

    vec4 texColor = texture(sampler2D(textures_heap[256], samplers_heap[0]), fragUV);

    outColor = vec4(texColor.rgb, 1.0);
}
