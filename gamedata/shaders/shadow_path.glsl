uniform mat4 _MVP;

shader main_vs(in vec3 position_ms) {
    gl_Position = _MVP * vec4(position_ms, 1.0);
}

shader main_fs() {
}


program shadow_path {
    vs(410) = main_vs();
    fs(410) = main_fs();
};