@shader

#version 150

@vertex

uniform mat4 display_matrix;
in vec3 position;

out vec4 surface_position;

void main () {
	surface_position = vec4(position, 1.0);	
	gl_Position = display_matrix * vec4(position, 1.0);
}

@fragment

uniform vec4 light_position;
uniform vec4 light_color;

in vec4 surface_position;
out vec4 fragment_color;

vec4 solid_light(vec4 position, vec4 color) {
	vec4 direction = position - surface_position;
	float distance = length(direction);
	
	return color / (distance * 0.2 + 1.0);
}

void main(void) {
	fragment_color = solid_light(light_position, light_color);
	fragment_color.w = 1.0;
}
