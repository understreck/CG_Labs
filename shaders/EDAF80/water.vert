#version 420

#define NUM_WAVES 2

layout (location = 0) in vec3 vertex;

uniform mat4 vertex_model_to_world;
uniform mat4 vertex_world_to_clip;

uniform float elapsed_time_s;

uniform float width;
uniform float height;

struct Wave {
    float amplitude;
    vec2 direction;
    float frequency;
    float phase;
    float sharpness;
};

Wave waves[NUM_WAVES] = {
    {1.0, {-1.0, 0.0}, 0.2, 0.5, 2.0},
    {0.5, {-0.7, 0.7}, 0.4, 1.3, 2.0},
    //{0.5, {-0.5, 0.5}, 0.3, 1.0 / 3.0},
    //{0.25, {-0.6, -0.4}, 0.3, 3.0 / 5.0},
    //{0.35, {0.3, -0.1}, 0.3, 3.0 / 7.0},
    //{0.15, {0.4, 0.5}, 0.3, 0.4},
    //{0.8, {0.7, 0.7}, 0.1, 0.5},
    //{0.15, {0.4, 0.5}, 0.2, 3.5},
    //{0.21, {0.55, 0.7}, 0.3, 1.5},
    //{0.25, {0.65, 0.35}, 0.4, 2.5},
    //{0.4, {1.0 / 3.0, 0.44}, 0.5, 1.5},
    //{0.11, {0.1, 0.3}, 0.6, 4.5},
    //{1.00, {1.0, 1.0}, 0.10, 2.5},
    //{0.15, {0.1, 0.7}, 0.1, 1.0},
    //{0.15, {0.7, 0.1}, 0.2, 1.3},
    //{0.22, {0.2, 0.2}, 2, 1.5},
};

out VS_OUT {
    mat3 BTN;
    vec3 pos;
    vec2 normalCoord0;
    vec2 normalCoord1;
    vec2 normalCoord2;
} vs_out;

#define B vs_out.BTN[0]
#define T vs_out.BTN[1]
#define N vs_out.BTN[2]

float
common_term(in int i) {
    float a = dot(waves[i].direction, vertex.xz) * waves[i].frequency;
    float t = elapsed_time_s * waves[i].phase;

    return a + t;
}

void main()
{
    vec3 pos = vertex;
    B = vec3(1.0, 0.0, 0.0);
    T = vec3(0.0, 0.0, 1.0);
    N = vec3(0.0, 1.0, 0.0);

    for(int i = 0; i < NUM_WAVES; ++i) {
        float commonTerm = common_term(i);
        float alpha = sin(commonTerm) * 0.5 + 0.5;
        float h = waves[i].amplitude * pow(alpha, waves[i].sharpness);
        pos.y += h;

        float dCommonTerm = //derivatives CommonTerm
            0.5 * waves[i].sharpness * waves[i].frequency * waves[i].amplitude *
            pow(alpha, waves[i].sharpness - 1.0);
        float dhdx = dCommonTerm * cos(commonTerm) * waves[i].direction.x;
        float dhdz = dCommonTerm * cos(commonTerm) * waves[i].direction.y;

        B.y += dhdx;
        T.y += dhdz;
        N.x -= dhdx;
        N.z -= dhdz;
    }

    vec2 texCoord = vec2(pos.x / width, pos.z / height);
    vec2 texScale = vec2(8, 4);
    float normalTime = mod(elapsed_time_s, 100.0);
    vec2 normalSpeed = vec2(-0.05, 0.0);
    vs_out.normalCoord0.xy =
    texCoord.xy * texScale + normalTime * normalSpeed;
    vs_out.normalCoord1.xy =
    texCoord.xy * texScale * 2 + normalTime * normalSpeed * 4;
    vs_out.normalCoord2.xy =
    texCoord.xy * texScale * 4 + normalTime * normalSpeed * 8;

    vec4 worldPos = vertex_model_to_world * vec4(pos, 1.0);

    vs_out.pos = worldPos.xyz;
    gl_Position =
        vertex_world_to_clip * worldPos;
}
