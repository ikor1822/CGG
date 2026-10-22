#version 450

layout(location = 0) in vec3 normal;
layout(location = 0) out vec4 out_color;

void main()
{
    vec3 light_direction = normalize(vec3(-0.5, 1.0, 0.7));
    float diffuse = max(dot(normalize(normal), light_direction), 0.0);
    float ambient = 0.25;
    float lighting = ambient + 0.75 * diffuse;
    vec3 base_color = vec3(0.1, 0.7, 0.9);
    out_color = vec4(base_color * lighting, 1.0);
}