#version 300 es
precision highp float;

in vec4 vVertexPosition;
in vec4 vColour;
in vec2 vTexCoord;

out vec4 fragColour;

uniform sampler2D uBaseTex;

void main() {
  vec2 fTexCoord =  vTexCoord;
  fTexCoord.y = (1.0 - vTexCoord.y);
	vec4 texcolour = texture(uBaseTex,fTexCoord);
	fragColour = vec4(texcolour.rgb,1.0);
}
