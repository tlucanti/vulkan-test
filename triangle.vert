
#version 450

vec3 positions[] = {
	vec3(0, -0.5, 0),
	vec3(0.5, 0.5, 0),
	vec3(-0.5, 0.5, 0),
};

void main()
{
	gl_Position = vec4(positions[gl_VertexIndex], 1);
}

