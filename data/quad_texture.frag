#version 330
precision highp float;

in vec4 vVertexPosition;
in vec4 vColour;
in vec2 vTexCoord;

out vec4 fragColour;

///\todo choose between both in the uber shader
// At present rect works well but we are sending normalised coordinates
uniform sampler2DRect uBaseTex;
//uniform sampler2D uBaseTex;

void main() {
  vec2 texsize = textureSize(uBaseTex); 
  vec2 fTexCoord =  vTexCoord;
  fTexCoord.y = 1.0 - vTexCoord.y;
	vec4 texcolour = texture(uBaseTex,fTexCoord * texsize);
	fragColour = vec4(texcolour.rgb,1.0);
}
