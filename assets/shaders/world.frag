// MIT. Subtle nearest-filtered pixel texture, directional light and distance fog.
in vec3 vColor;
in vec3 vNormal;
in vec2 vUV;
in float vDistance;
uniform sampler2D uTexture;
uniform vec3 uSky;
uniform float uDay;
uniform int uMode;
out vec4 fragColor;
void main() {
  if(uMode==1) { fragColor=vec4(vColor,1.0);return; }
  float light=0.58+0.42*max(0.0,dot(normalize(vNormal),normalize(vec3(-0.5,0.9,0.35))));
  float textureValue=texture(uTexture,vUV).r;
  vec3 color=vColor*light*(0.88+textureValue*0.22)*(0.42+uDay*0.58);
  float fog=smoothstep(85.0,340.0,vDistance);
  fragColor=vec4(mix(color,uSky,fog),1.0);
}
