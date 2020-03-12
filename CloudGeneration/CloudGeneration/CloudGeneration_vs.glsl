#version 430            
layout(location = 2) uniform mat4 _CameraToWorld;
layout(location = 3) uniform mat4 _CameraInverseProjection;

layout(location = 0) in vec3 pos_attrib;
layout(location = 1) in vec2 tex_coord_attrib;
layout(location = 2) in vec3 normal_attrib;

out vec2 tex_coord;  

void main(void)
{
	gl_Position = vec4(pos_attrib, 1.0);
	tex_coord = 0.5*pos_attrib.xy + vec2(0.5);
}