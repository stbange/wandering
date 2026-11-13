// MIT. Version header supplied by the platform adapter.
layout(location=0) in vec3 aPosition;
layout(location=1) in vec3 aColor;
layout(location=2) in vec3 aNormal;
layout(location=3) in vec2 aUV;
uniform mat4 uMatrix;
out vec3 vColor;
out vec3 vNormal;
out vec2 vUV;
out float vDistance;
void main() {
  gl_Position=uMatrix*vec4(aPosition,1.0);
  vColor=aColor;vNormal=aNormal;vUV=aUV;vDistance=length(aPosition);
}
