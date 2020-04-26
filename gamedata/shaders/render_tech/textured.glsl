uniform mat4 MVP;
uniform sampler2D texture0;

interface vs_out {
    vec2 uv;
};

shader textured_vs(in vec3 position_ms, in vec2 uv, out vs_out o) {
    gl_Position = MVP * vec4(position_ms, 1.0);
    o.uv        = uv;
}

layout (location = 5) in mat4 mvp;

shader textured_instanced_vs(in vec3 position_ms, in vec2 uv, out vs_out o) {
    gl_Position = mvp * vec4(position_ms, 1.0);
    o.uv        = uv;
}

shader textured_fs(in vs_out i, out vec3 color) {
    color = texture(texture0, i.uv).rgb;
}

program textured
{
    vs(410) = textured_vs();
    fs(410) = textured_fs();
};

program textured_instanced
{
    vs(410) = textured_instanced_vs();
    fs(410) = textured_fs();
};