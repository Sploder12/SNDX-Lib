#version 330 core
out vec4 FragColor;

in vec2 TexCoords;
in vec3 Normal;
in vec3 FragPos;


uniform sampler2D texture_diffuse1;

void main()
{    
    vec4 color = texture(texture_diffuse1, TexCoords);
    //color = vec4(1.0);

    const float ambientStr = 0.1;
    const vec3 lightColor = vec3(1.0);
    const vec3 lightPos = vec3(-1.0, 1.0, 0.0);

    vec3 ambient = ambientStr * lightColor;

    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);

    vec3 diffuse = max(dot(norm, lightDir), 0.0) * lightColor;

    vec4 result = vec4(diffuse + ambient, 1.0) * color;
    FragColor = result;
}