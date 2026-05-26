#version 330 core
out vec4 FragColor;

in vec3 ourColor;
in vec3 TexCoord;
in vec3 Normal;
in vec3 FragPos;

uniform sampler3D texture1;
uniform sampler1D texture2;

uniform vec3 lightPos;
uniform vec3 viewPos;

// function prototypes
vec3 PhongShading(vec3 gradient,vec3 color);

void main(){
    vec3 P = FragPos;
    vec3 Color = PhongShading(Normal, ourColor);
    FragColor = vec4(Color, 1.0f);
    // sol. 1
    //FragColor = vec4(PhongShading(Normal), 1.0f);
}

vec3 PhongShading(vec3 gradient,vec3 color){
    float Ka = 0.2, Kd = 0.8, Ks = 0.4;
    vec3 Ia = color;
    vec3 Id = color;
    vec3 Is = vec3(1.0, 1.0, 1.0);

    vec3 ambient = Ka * Ia;

    vec3 N = normalize(gradient);
    vec3 L = normalize(lightPos - FragPos);
    vec3 diffuse = Kd * Id * max(dot(N, L), 0.0);

    vec3 V = normalize(viewPos - FragPos);
    vec3 R = reflect(-L, N);
    vec3 specular = Ks * Is * pow(max(dot(V, R), 0.0), 256.0);

    return vec3((diffuse + specular) / (length(lightPos - FragPos) * 0.005f) + ambient);

}