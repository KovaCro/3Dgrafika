#include <bits/stdc++.h>
#include <cstdlib>
#include <stdio.h>
#include <iostream>
#include "glad/include/glad.h"
#include "GL/include/glfw3.h"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define MAX_PARTICLES 20000
// Windows: g++ main.cpp glad\glad.c GL\lib-mingw-w64\libglfw3.a -l gdi32 -l opengl32 -o out -std=c++17

using namespace std;
using namespace glm;

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
  glViewport(0, 0, width, height);
}

unsigned int loadTexture(char const * path) {
  unsigned int textureID;
  glGenTextures(1, &textureID);
  int width, height, nrComponents;
  unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);
  glBindTexture(GL_TEXTURE_2D, textureID);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
  glGenerateMipmap(GL_TEXTURE_2D);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  stbi_image_free(data);
  return textureID;
}

const char *vertexShaderSource = "#version 330 core\n"
  "layout (location = 0) in vec3 vPos;\n"
  "layout (location = 1) in vec3 pPos;\n"
  "layout (location = 2) in vec3 pColor;\n"
  "uniform mat4 projection;\n"
  "out vec2 TexCoords;\n"
  "out vec3 fColor;\n"
  "void main()\n"
  "{\n"
  "  vec3 sum = pPos + vPos;\n"
  "  gl_Position = projection * vec4(sum, 1.0f);\n"
  "  fColor = pColor;\n"
  "  TexCoords = vec2(vPos.x + 0.5f, vPos.y + 0.5f);\n"
  "}\0";

const char *fragmentShaderSource = "#version 330 core\n"
  "in vec3 fColor;\n"
  "in vec2 TexCoords;\n"
  "uniform sampler2D pTexture;\n"
  "out vec4 FragColor;\n"
  "void main()\n"
  "{\n"
  "  vec4 tex = texture(pTexture, TexCoords);\n"
  "  FragColor = vec4(fColor, (tex.x + tex.y + tex.z) / 3);\n"
  "}\0";

struct particle {
  vec3 pos, col, dir, force;
  float life, fade;
  bool operator<(particle& that){
    return this->pos.z > that.pos.z;
  }
};

static const GLfloat g_vertex_buffer_data[] = {
  -0.5f, -0.5f, 0.0f,
  0.5f, -0.5f, 0.0f,
  -0.5f, 0.5f, 0.0f,
  0.5f, 0.5f, 0.0f,
};

struct Generator {
  particle particles[MAX_PARTICLES];
  unsigned int particleCount = MAX_PARTICLES;
  unsigned int particleTex = loadTexture("./particle.jpg");
  GLuint billboard_vertex_buffer;
  GLuint particles_position_buffer;
  GLuint particles_color_buffer;
  float *g_particule_position_data = new float[MAX_PARTICLES * 3];
  float *g_particule_color_data = new float[MAX_PARTICLES * 3];

  void init() {
    for(int i = 0; i < MAX_PARTICLES; i++) {
      newParticle(i);
	  }
    glGenBuffers(1, &billboard_vertex_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, billboard_vertex_buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(g_vertex_buffer_data), g_vertex_buffer_data, GL_STATIC_DRAW);

    glGenBuffers(1, &particles_position_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, particles_position_buffer);
    glBufferData(GL_ARRAY_BUFFER, MAX_PARTICLES * 3 * sizeof(GLfloat), NULL, GL_STREAM_DRAW);

    glGenBuffers(1, &particles_color_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, particles_color_buffer);
    glBufferData(GL_ARRAY_BUFFER, MAX_PARTICLES * 3 * sizeof(GLfloat), NULL, GL_STREAM_DRAW);
  }
  void newParticle(unsigned int i){
    particles[i].pos = vec3(0.f, 0.f, 0.f);
    particles[i].life = 1.0f;
    particles[i].fade = float(rand()%100)/5000.0f+0.02f;
    particles[i].col = vec3(1.0f, 1.0f, 1.0f);
    particles[i].dir = 0.02f * vec3(float((rand()%50)-25.f), float((rand()%20)), float((rand()%50)-25.f));
    particles[i].force = 0.05f * vec3(0.0, 0.8, 0.0);
  }
  void updateParticles() {
    for(int i = 0; i < particleCount; i++){
      particles[i].life -= particles[i].fade;
      if (particles[i].life > 0.0f){
        particles[i].force.x += (particles[i].pos.x < 0) ? 0.0008 : -0.0008;
        particles[i].force.z += (particles[i].pos.z < 0) ? 0.0008 : -0.0008;

        particles[i].dir += particles[i].force;
        particles[i].pos += particles[i].dir;

        /*if(particles[i].life < 0.4f){
          float interpolation = ((particles[i].life) * 2.5);
          particles[i].col = vec3(1.0f, 0.65f, 0.0f) * interpolation + vec3(1.0f, 0.0f, 0.0f) * (1 - interpolation);
        } else if (particles[i].life < 0.6){
          float interpolation = ((particles[i].life - 0.4) * 5);
          particles[i].col = vec3(1.0f, 0.85f, 0.098f) * interpolation + vec3(1.0f, 0.65f, 0.0f) * (1 - interpolation);
        } else if (particles[i].life < 0.8){
          float interpolation = ((particles[i].life - 0.6) * 5);
          particles[i].col = vec3(0.5f, 0.93f, 0.95f) * interpolation + vec3(1.0f, 0.85f, 0.098f) * (1 - interpolation);
        } else {
          float interpolation = ((particles[i].life - 0.8) * 5);
          particles[i].col = vec3(1.0f, 1.0f, 1.0f) * interpolation + vec3(0.5f, 0.93f, 0.95f) * (1 - interpolation);
        }*/
        if(particles[i].life < 0.4f){
          particles[i].col = vec3(1.0f, 0.0f, 0.0f);
        } else if (particles[i].life < 0.6){
          particles[i].col = vec3(1.0f, 0.65f, 0.0f);
        } else if (particles[i].life < 0.8){
          particles[i].col = vec3(1.0f, 0.85f, 0.098f);
        } else if (particles[i].life < 0.9){
          particles[i].col = vec3(0.5f, 0.93f, 0.95f);
        } else {
          particles[i].col = vec3(1.0f, 1.0f, 1.0f);
        }
      } else {
        newParticle(i);
      }
    }
    sort(&particles[0], &particles[MAX_PARTICLES]);
  }
  void updateBuffers() {
    for(int i = 0; i < particleCount; i++){
      g_particule_position_data[3*i+0] = particles[i].pos.x;
      g_particule_position_data[3*i+1] = particles[i].pos.y;
      g_particule_position_data[3*i+2] = particles[i].pos.z;
      g_particule_color_data[3*i+0] = particles[i].col.x;
      g_particule_color_data[3*i+1] = particles[i].col.y;
      g_particule_color_data[3*i+2] = particles[i].col.z;
    }
    glBindBuffer(GL_ARRAY_BUFFER, particles_position_buffer);
    glBufferData(GL_ARRAY_BUFFER, MAX_PARTICLES * 3 * sizeof(GLfloat), NULL, GL_STREAM_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, particleCount * sizeof(GLfloat) * 3, g_particule_position_data);

    glBindBuffer(GL_ARRAY_BUFFER, particles_color_buffer);
    glBufferData(GL_ARRAY_BUFFER, MAX_PARTICLES * 3 * sizeof(GLfloat), NULL, GL_STREAM_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, particleCount * sizeof(GLfloat) * 3, g_particule_color_data);
  }
  void render() {
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, particleTex); 

    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, billboard_vertex_buffer);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, particles_position_buffer);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

    glEnableVertexAttribArray(2);
    glBindBuffer(GL_ARRAY_BUFFER, particles_color_buffer);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
      
    glVertexAttribDivisor(0, 0);
    glVertexAttribDivisor(1, 1);
    glVertexAttribDivisor(2, 1);

    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, particleCount);
  }
};

int main() {
  glfwInit();

  GLFWwindow* window = glfwCreateWindow(800, 800, "OpenGL", NULL, NULL);
  glfwMakeContextCurrent(window);
  glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
  gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  unsigned int particleVertexShader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(particleVertexShader, 1, &vertexShaderSource, NULL);
  glCompileShader(particleVertexShader);
  unsigned int particleFragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(particleFragmentShader, 1, &fragmentShaderSource, NULL);
  glCompileShader(particleFragmentShader);
  unsigned int shaderProgram = glCreateProgram();
  glAttachShader(shaderProgram, particleVertexShader);
  glAttachShader(shaderProgram, particleFragmentShader);
  glLinkProgram(shaderProgram);
  glUseProgram(shaderProgram);

  mat4 projMat = ortho(-20.f, 20.f, -10.f, 50.f, -20.f, 20.f);
  glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, &projMat[0][0]);
  glUniform1i(glGetUniformLocation(shaderProgram, "pTexture"), 0);

  Generator gen;
  gen.init();

  while (!glfwWindowShouldClose(window)) {
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    gen.updateParticles();
    gen.updateBuffers();
    gen.render();
    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  glfwTerminate();
  return 0;
}