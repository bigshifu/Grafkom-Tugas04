#version 330 core

struct Material {
    sampler2D diffuse;
    sampler2D specular;
    float shininess;
};

uniform Material material;

void main()
{             
    //gl_FragDepth = gl_FragCoord.z; // no need to set depth explicitly
}