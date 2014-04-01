#version 300 es
precision highp float;
// https://github.com/dghost/glslRiftDistort

uniform sampler2D uTexSampler0;

uniform vec2 sLensCenter;
uniform vec2 sScreenCenter;

// Oculus specific uniforms
uniform vec2 uScale;
uniform vec2 uScaleIn;
uniform vec4 uHmdWarpParam;
uniform vec4 uChromAbParam;

uniform float uDistortionScale;

in vec2 vTexCoord;

//layout(location = 0) out vec4 outColor; // GLSL 3.30 or higher only

out vec4 sOutColour; // GLSL 1.50 or higher

// Performs Barrel distortion and Chromatic Abberation correction

void main(void)
{
  //ivec2 tex_sizei = textureSize(uTexSampler0,0);
  vec2 tex_size;
  //tex_size.x = float(tex_sizei.x);
  //tex_size.y = float(tex_sizei.y); 
  vec2 tc = vTexCoord;
  tc.y = 1.0 - tc.y;

  vec2 theta = (tc - sLensCenter) * uScaleIn; // uScales to [-1, 1]
  float rSq = theta.x * theta.x + theta.y * theta.y;
  vec2 rvector= theta * ( uHmdWarpParam.x + uHmdWarpParam.y * rSq +
    uHmdWarpParam.z * rSq * rSq 
    + uHmdWarpParam.w * rSq * rSq * rSq
    );

  vec2 theta_blue = rvector * (uChromAbParam.z + uChromAbParam.w * rSq);
  vec2 tc_blue = sLensCenter + uScale * uDistortionScale * theta_blue;
  if (!all(equal(clamp(tc_blue, sScreenCenter-vec2(0.25,0.5), sScreenCenter+vec2(0.25,0.5)), tc_blue))) {
    sOutColour = vec4(0);
    return;
  }
  

  float blue = texture(uTexSampler0, tc_blue).b;

  vec2  tc_green = sLensCenter + uScale * uDistortionScale * rvector;
  vec4  center = texture(uTexSampler0, tc_green);

  vec2  theta_red = rvector * (uChromAbParam.x + uChromAbParam.y * rSq);
  vec2  tc_red = sLensCenter + uScale * uDistortionScale * theta_red;
  float red = texture(uTexSampler0, tc_red).r;


  sOutColour = vec4(red, center.g, blue, center.a);

  //sOutColour = texture(uTexSampler0, vTexCoord);

  //sOutColour = vec4(1.0,1.0,0.0,1.0);
}
