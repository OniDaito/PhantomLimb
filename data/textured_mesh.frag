#version 300 es
precision highp float;

in vec4 vVertexPosition;
in vec4 vColour;
in vec2 vTexCoord;

// Works with one light using the phong model

// Material
uniform vec4 uMaterialAmbient;
uniform vec4 uMaterialDiffuse;
uniform vec4 uMaterialSpecular;
uniform vec4 uMaterialEmissive;
uniform float uMaterialShininess;

///\todo choose between both in the uber shader
// At present rect works well but we are sending normalised coordinate
//uniform sampler2DRect uTexSampler0;
uniform sampler2D uTexSampler0;

out vec4 fragColour;

void main() {
  //vec3 n = normalize(vVertexNormal.xyz);
  //vec2 texsize = textureSize(uTexSampler0); 
  //vec4 texcolor = texture(uTexSampler0, vTexCoord * texsize);
  //vec4 texcolor = texture(uTexSampler0, vTexCoord);
  //fragColour = vec4(texcolor.rgb,1.0);

  fragColour = uMaterialDiffuse;

}
