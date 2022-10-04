#version 420

#define NUM_WAVES 9

layout (location = 0) in vec3 vertex;

uniform mat4 vertex_model_to_world;
uniform mat4 vertex_world_to_clip;

uniform float elapsed_time_s;
uniform float wave_sharpness;

uniform float width;
uniform float height;

struct Wave {
    float amplitude;
    vec2 direction;
    float frequency;
    float phase;
};

Wave waves[NUM_WAVES] = {
    {0.2, {0.7, 0.7}, 0.1, 0.5},
    {0.15, {0.4, 0.5}, 0.2, 3.5},
    {0.21, {0.55, 0.7}, 0.3, 1.5},
    {0.25, {0.65, 0.35}, 0.4, 2.5},
    {0.4, {1.0 / 3.0, 0.44}, 0.5, 1.5},
    {0.11, {0.1, 0.3}, 0.6, 4.5},
    {1.00, {1.0, 1.0}, 0.10, 2.5},
    {0.15, {0.1, 0.7}, 0.1, 1.0},
    {0.15, {0.7, 0.1}, 0.2, 1.3},
    //{0.22, {0.2, 0.2}, 2, 1.5},
};

float
sharpness_term(in int i) {
    return wave_sharpness /
        (waves[i].frequency * waves[i].amplitude * NUM_WAVES);
}

float
trig_term(in int i) {
    float phaseTime = waves[i].phase * elapsed_time_s;
    float dirTerm = dot(waves[i].frequency * waves[i].direction, vertex.xz); 

    return dirTerm + phaseTime;
}

out VS_OUT {
    mat3 BNT;
    vec3 pos;
    vec2 texCoords;
} vs_out;

#define B vs_out.BNT[0]
#define N vs_out.BNT[1]
#define T vs_out.BNT[2]

void main()
{
    vec3 pos = vertex;
    B = vec3(1.0, 0.0, 0.0);
    N = vec3(0.0, 1.0, 0.0);
    T = vec3(0.0, 0.0, 1.0);

    for(int i = 0; i < NUM_WAVES; ++i) {
        float trig = trig_term(i);
        float sharp = sharpness_term(i);

        float _common = sharp * cos(trig) * waves[i].amplitude;

        pos.x += _common * waves[i].direction.x;
        pos.z += _common * waves[i].direction.y;
        pos.y -= waves[i].amplitude * sin(trig);

        float wa = waves[i].frequency * waves[i].amplitude;

        float phaseTime = waves[i].phase * elapsed_time_s;
        float s = sin(
            waves[i].frequency *
            dot(waves[i].direction, pos.xz) +
            phaseTime
        );

        float qwas = sharp * wa * s;

        float c = cos(
            waves[i].frequency *
            dot(waves[i].direction, pos.xz) +
            phaseTime
        );

        float wac = wa * c;

        float xy = waves[i].direction.x * waves[i].direction.y;
        float xx = pow(waves[i].direction.x, 2.0);
        float yy = pow(waves[i].direction.y, 2.0);

        float xwac = waves[i].direction.x * wac;
        float ywac = waves[i].direction.y * wac;

        B.x -= qwas * xx;
        B.z -= qwas * yy;
        B.y += xwac;
        B = normalize(B);

        N.x -= xwac;
        N.z -= ywac;
        N.y -= qwas;
        N = normalize(N);

        T.x -= qwas * xy;
        T.z -= qwas * yy;
        T.y += ywac;
        T = normalize(T);
    }

    vs_out.texCoords = vec2(pos.x / width, pos.z / height);
    vec4 worldPos = vertex_model_to_world * vec4(pos, 1.0);

    vs_out.pos = worldPos.xyz;
    gl_Position =
        vertex_world_to_clip * worldPos;
}
