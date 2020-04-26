shader main_vs(in vec3 position_ms) {
    gl_Position = vec4(position_ms, 1.0);
}

shader main_fs(out vec3 color) {
    color = vec3(1, 0, 1);
}

program dummy_path {
    vs(410) = main_vs();
    fs(410) = main_fs();
};


uniform sampler2D screen_quad_texture;
uniform float time;

// SHADERTOY START
float rand(vec2 co){
    return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
}

const float range = 0.05;
const float noiseQuality = 250.0;
const float noiseIntensity = 0.0088;
const float offsetIntensity = 0.02;
const float colorOffsetIntensity = 1.3;

float verticalBar(float pos, float uvY, float offset)
{
    float edge0 = (pos - range);
    float edge1 = (pos + range);

    float x = smoothstep(edge0, pos, uvY) * offset;
    x -= smoothstep(pos, edge1, uvY) * offset;
    return x;
}

shader vhs1_fs(in vec2 uv, out vec4 color) {
    vec2 uv_op = uv;

    for (float i = 0.0; i < 0.71; i += 0.1313)
    {
        float d = mod(time * i, 1.7);
        float o = sin(1.0 - tan(time * 0.24 * i));
        o *= offsetIntensity;
        uv_op.x += verticalBar(d, uv_op.y, o);
    }

    float uvY = uv_op.y;
    uvY *= noiseQuality;
    uvY = float(int(uvY)) * (1.0 / noiseQuality);
    float noise = rand(vec2(time * 0.00001, uvY));
    uv_op.x += noise * noiseIntensity;

    vec2 offsetR = vec2(0.006 * sin(time), 0.0) * colorOffsetIntensity;
    vec2 offsetG = vec2(0.0073 * (cos(time * 0.97)), 0.0) * colorOffsetIntensity;

    float r = texture(screen_quad_texture, uv_op + offsetR).r;
    float g = texture(screen_quad_texture, uv_op + offsetG).g;
    float b = texture(screen_quad_texture, uv_op).b;

    color = vec4(r, g, b, 1.0);
}

shader vhs2_fs(in vec2 uv, out vec4 color)
{
    vec4 tex_color = vec4(0);
    // get position to sample
    vec2 samplePosition = uv;
    float whiteNoise = 9999.0;

    // Jitter each line left and right
    samplePosition.x = samplePosition.x + (rand(vec2(time, uv.y)) - 0.5) / 64.0;
    // Jitter the whole picture up and down
    samplePosition.y = samplePosition.y + (rand(vec2(time)) - 0.5) / 32.0;
    // Slightly add color noise to each line
    tex_color = tex_color + (vec4(-0.5) + vec4(rand(vec2(uv.y, time)), rand(vec2(uv.y, time + 1.0)), rand(vec2(uv.y, time + 2.0)), 0)) * 0.1;

    // Either sample the texture, or just make the pixel white (to get the staticy-bit at the bottom)
    whiteNoise = rand(vec2(floor(samplePosition.y * 80.0), floor(samplePosition.x * 50.0)) + vec2(time, 0));
    if (whiteNoise > 11.5 - 30.0 * samplePosition.y || whiteNoise < 1.5 - 5.0 * samplePosition.y) {
        // Sample the texture.
        samplePosition.y = samplePosition.y;
        tex_color = tex_color + texture(screen_quad_texture, samplePosition);
    } else {
        // Use white. (I'm adding here so the color noise still applies)
        tex_color = vec4(1);
    }
    color = tex_color;
}

// SHADERTOY END


shader passthrough_screen_quad_vs(in vec3 position_ms, out vec2 uv) {
    gl_Position = vec4(position_ms, 1.0);
    uv = (position_ms.xy + vec2(1.0, 1.0)) * 0.5;
}

shader passthrough_screen_quad_fs(in vec2 uv, out vec3 color) {
    color = texture(screen_quad_texture, uv).rgb;
    //color = texture(screen_quad_texture, uv + 0.005 * vec2(sin(time + 800.0 * uv.x), cos(time + 600.0 * uv.y))).xyz;
}

shader wobbly_texture_fs(in vec2 uv, out vec3 color) {
    color = texture(screen_quad_texture, uv + 0.005 * vec2(sin(time + 800.0 * uv.x), cos(time + 600.0 * uv.y))).xyz;
}

program passthrough_screen_quad {
    vs(410) = passthrough_screen_quad_vs();
    fs(410) = passthrough_screen_quad_fs();
};

program wobbly_texture {
    vs(410) = passthrough_screen_quad_vs();
    fs(410) = wobbly_texture_fs();
};

program vhs1_texture {
    vs(410) = passthrough_screen_quad_vs();
    fs(410) = vhs1_fs();
};

program vhs2_texture {
    vs(410) = passthrough_screen_quad_vs();
    fs(410) = vhs2_fs();
};
