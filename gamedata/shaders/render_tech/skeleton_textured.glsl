const int MAX_BONES = 128;

uniform mat4 MVP;
uniform sampler2D texture0;
uniform mat4 bone_matrices[MAX_BONES];

interface vs_out {
    vec2 uv;
};

shader textured_vs(
    in vec3 position_ms,
    in vec2 uv,
    in ivec4 bone_ids,
    in vec4 weights,
    out vs_out o
) {
    mat4 bone_transform = bone_matrices[bone_ids[0]] * weights[0];
    bone_transform     += bone_matrices[bone_ids[1]] * weights[1];
    bone_transform     += bone_matrices[bone_ids[2]] * weights[2];
    bone_transform     += bone_matrices[bone_ids[3]] * weights[3];

    gl_Position = MVP * (bone_transform * vec4(position_ms, 1.0));
    o.uv        = uv;
}

//layout (location = 5) in mat4 mvp;

shader textured_instanced_vs(in vec3 position_ms, in vec2 uv, out vs_out o) {
    gl_Position = mvp * vec4(position_ms, 1.0);
    o.uv        = uv;
}

shader textured_fs(in vs_out i, out vec3 color) {
    color = texture(texture0, i.uv).rgb;
}

program skeleton_textured
{
    vs(410) = textured_vs();
    fs(410) = textured_fs();
};

program skeleton_textured_instanced
{
    vs(410) = textured_instanced_vs();
    fs(410) = textured_fs();
};