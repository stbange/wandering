// MIT. Version directive is supplied for desktop GL or WebGL2.
layout(location=0) in vec3 position;
layout(location=1) in vec4 color;
layout(location=2) in vec2 uv;
uniform mat4 matrix;
uniform vec3 camera;
out vec4 tint;
out vec2 texcoord;
out float distanceToEye;
void main() {
    gl_Position=matrix*vec4(position,1.0);
    tint=color;
    texcoord=uv;
    distanceToEye=distance(position,camera);
}
