uniform sampler2D texture0;

in  vec2 uv;
out vec4 color;

void main() {
    color = texture(texture0, uv);
}

