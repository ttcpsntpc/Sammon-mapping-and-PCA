#version 330 core
out vec4 FragColor;

in vec3 ourColor;
in vec3 TexCoord;
in vec3 Normal;
in vec3 FragPos;

// function prototypes
vec3 PhongShading(vec3 gradient,vec3 color);

void main(){
    FragColor = vec4(ourColor, 1.0f);
}
