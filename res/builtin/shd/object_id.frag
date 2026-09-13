#version 330 core

flat in uint v_object_id;

out uint f_color;

void main() {
	f_color = v_object_id;
}
