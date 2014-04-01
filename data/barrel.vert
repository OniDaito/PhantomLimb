
#version 300 es
precision highp float;

out vec2 vTexCoord;

layout (location = 0) in vec3 aVertPosition; 
layout (location = 3) in vec2 aVertTexCoord;

// Defaults set by Seburo
uniform mat4 uModelMatrix;
uniform mat4 uViewMatrix;
uniform mat4 uProjectionMatrix;

void main() {            
  gl_Position = uProjectionMatrix * uViewMatrix * uModelMatrix * vec4(aVertPosition,1.0f);
  vTexCoord = aVertTexCoord;
} 
