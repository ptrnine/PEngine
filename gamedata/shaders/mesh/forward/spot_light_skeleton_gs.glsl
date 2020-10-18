
layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

in vs_out {
    float depth;
    flat int spot_idx;
} gs_input[];

void main() {
    for (int i = 0; i < gl_in.length(); ++i) {
        gl_Position = gl_in[i].gl_Position;
        gl_Layer = gs_input[i].spot_idx;
        EmitVertex();
    }
    EndPrimitive();
}

