// MIT. Negative vertex alpha selects the cutout vegetation tile.
in vec4 tint;
in vec2 texcoord;
in float distanceToEye;
uniform sampler2D atlas;
uniform vec3 fog;
uniform float daylight;
uniform int ui;
out vec4 pixel;
void main() {
    vec3 c=tint.rgb;
    if(ui==0) {
        vec2 uv=vec2(fract(texcoord.x)*0.5,fract(texcoord.y));
        if(tint.a<0.0) uv=vec2(0.5+texcoord.x*0.499,texcoord.y);
        vec4 texel=texture(atlas,uv);
        if(texel.a<0.5) discard;
        float light=max(daylight,0.13)+0.32*exp(-distanceToEye*0.075);
        c*=texel.rgb*light;
        c=mix(c,fog,smoothstep(90.0,310.0,distanceToEye));
        c=floor(c*32.0+0.5)/32.0;
    }
    pixel=vec4(c,abs(tint.a));
}
