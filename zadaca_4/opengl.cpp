#include <cstdlib>
#include <stdio.h>
#include <iostream>
#include <vector>
#include <string>
#include "glad/include/glad.h"
#include "GL/include/glfw3.h"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "obj_loader.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
// Windows: g++ opengl.cpp glad\glad.c GL\lib-mingw-w64\libglfw3.a -l gdi32 -l opengl32 -o out -std=c++17

using namespace std;
using namespace glm;

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow *window);

const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

const char *vertexShaderSource = "#version 330 core\n"
  "layout (location = 0) in vec3 aPos;\n"
  "layout (location = 1) in vec3 aNormal;\n"
  "layout (location = 2) in vec2 aTexCoords;\n"
  "uniform mat4 model;\n"
  "uniform mat4 view;\n"
  "uniform mat4 projection;\n"
  "out vec3 FragPos;\n"
  "out vec3 Normal;\n"
  "out vec2 TexCoords;\n"
  "void main()\n"
  "{\n"
  "  gl_Position = projection * view * model * vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
  "  FragPos = vec3(model * vec4(aPos, 1.0));\n"
  "  Normal = mat3(transpose(inverse(model))) * aNormal;\n"
  "  TexCoords = aTexCoords;\n"
  "}\0";

const char *fragmentShaderSource = "#version 330 core\n"
  "out vec4 FragColor;\n"
  "in vec3 FragPos;\n"
  "in vec3 Normal;\n"  
  "in vec2 TexCoords;\n"
  "uniform sampler2D diffuse;\n"
  "uniform sampler2D specular;\n"
  "uniform float shininess;\n"
  "uniform vec3 lightPos;\n"
  "uniform vec3 lightColor;\n"
  "uniform vec3 viewPos;\n"
  "void main()\n"
  "{\n"
  "  vec3 norm = normalize(Normal);\n"
  "  vec3 lightDir = normalize(lightPos - FragPos);\n"
  "  vec3 viewDir = normalize(viewPos - FragPos);\n"
  "  vec3 reflectDir = reflect(-lightDir, norm);\n"
  "  float diff = max(dot(norm, lightDir), 0.0);\n"
  "  float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);\n"
  "  vec3 a_res = vec3(0.2) * vec3(1.0, 1.0, 1.0) * vec3(texture(diffuse, TexCoords));\n"
  "  vec3 d_res = vec3(diff) * vec3(1.0, 1.0, 1.0) * vec3(texture(diffuse, TexCoords));\n"
  "  vec3 s_res = vec3(spec) * vec3(1.0, 1.0, 1.0) * vec3(texture(specular, TexCoords));\n"
  "  FragColor = vec4(a_res + d_res + s_res, 1.0);\n"
  "}\0";

const char *vLightShaderSource = "#version 330 core\n"
  "layout (location = 0) in vec3 aPos;\n"
  "uniform mat4 transformation;\n"
  "void main()\n"
  "{\n"
  "  gl_Position = transformation * vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
  "}\0";

const char *fLightShaderSource = "#version 330 core\n"
  "out vec4 FragColor;\n"
  "uniform vec3 lightColor;\n"
  "void main()\n"
  "{\n"
  "   FragColor = vec4(lightColor, 1.0f);\n"
  "}\0";

unsigned int loadTexture(char const * path) {
  unsigned int textureID;
  glGenTextures(1, &textureID);
  int width, height, nrComponents;
  unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);
  glBindTexture(GL_TEXTURE_2D, textureID);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
  glGenerateMipmap(GL_TEXTURE_2D);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  stbi_image_free(data);
  return textureID;
}

struct Mesh {
  vector<vec3> vertices, normals;
  vector<vec2> texCords;
  unsigned int specular, diffuse;
  float shininess;
  vector<unsigned int> indices;
  GLuint VAO, VBO, EBO;
  vec3 scale, translation;

  Mesh() {}
  Mesh(string filename, unsigned int diffuse, unsigned int specular, float shininess, vec3 scale, vec3 translation): 
  diffuse(diffuse), specular(specular), shininess(shininess), scale(scale), translation(translation) {
    objl::Loader loader;
    loader.LoadFile(filename);
    for(auto i:loader.LoadedMeshes[0].Indices){
      indices.push_back(i);
    }
    for(auto v: loader.LoadedMeshes[0].Vertices){
      vertices.push_back(vec3(v.Position.X, v.Position.Y, v.Position.Z));
      normals.push_back(vec3(v.Normal.X, v.Normal.Y, v.Normal.Z));
      texCords.push_back(vec2(v.TextureCoordinate.X, v.TextureCoordinate.Y));
    }
    setupMesh();
  }
  void setupMesh(){
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
  
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(vec3) +  normals.size() * sizeof(vec3) + texCords.size() * sizeof(vec2), NULL, GL_STATIC_DRAW);

    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vec3) * vertices.size(), &vertices[0]);
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(vec3) * vertices.size(), sizeof(vec3) * normals.size(), &normals[0]);
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(vec3) * vertices.size() + sizeof(vec3) * normals.size(), sizeof(vec2) * texCords.size(), &texCords[0]);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vec3), 0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(vec3), (void*)(sizeof(vec3)*vertices.size()));  
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(vec2), (void*)(sizeof(vec3)*vertices.size() + sizeof(vec3)*normals.size()));
    glEnableVertexAttribArray(2);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

    glBindVertexArray(0);
  }
  void drawMesh(){
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
  }
};

struct Light {
  vec3 color, pos;
  Mesh obj;
  Light (vec3 color, vec3 pos): color(color), pos(pos) {
    this->obj = Mesh("./sphere.obj", 0, 0, 0, vec3(0.2, 0.2, 0.2), pos);
  }
};

float rotationCoef = 1;

int main() {
  glfwInit();

  GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "OpenGL", NULL, NULL);
  glfwMakeContextCurrent(window);
  glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
  gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

  glEnable(GL_DEPTH_TEST);

  unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
  glCompileShader(vertexShader);
  unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
  glCompileShader(fragmentShader);
  unsigned int shaderProgram = glCreateProgram();
  glAttachShader(shaderProgram, vertexShader);
  glAttachShader(shaderProgram, fragmentShader);
  glLinkProgram(shaderProgram);

  unsigned int vLightShader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vLightShader, 1, &vLightShaderSource, NULL);
  glCompileShader(vLightShader);
  unsigned int fLightShader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fLightShader, 1, &fLightShaderSource, NULL);
  glCompileShader(fLightShader);
  unsigned int lightShaderProgram = glCreateProgram();
  glAttachShader(lightShaderProgram, vLightShader);
  glAttachShader(lightShaderProgram, fLightShader);
  glLinkProgram(lightShaderProgram);

  unsigned int diff_bricks = loadTexture("./bricks_diffuse.jpg");
  unsigned int spec_bricks = loadTexture("./bricks_specular.jpg");

  unsigned int diff_chest = loadTexture("./chest_diffuse.jpg");
  unsigned int spec_chest = loadTexture("./chest_specular.jpg");

  Light light(vec3(1.0, 1.0, 1.0), vec3(0.0, 0.0, 3.0));

  vector<Mesh*> meshes;
  // filename text_id -||- shininess scale position
  Mesh cube("./cube.obj", diff_chest, spec_chest, 1000.f, vec3(1.0, 1.0, 1.0), vec3(-3.0, -3.0, 0.0));
  Mesh cube2("./cube.obj", diff_bricks, spec_bricks, 1000.f, vec3(1.0, 1.0, 1.0), vec3(-3.0, 3.0, 0.0));
  Mesh pyramid("./pyramid.obj", diff_bricks, spec_bricks, 1000.f, vec3(1.0, 1.0, 1.0), vec3(3.0, 3.0, 0.0));
  Mesh pyramid2("./pyramid.obj", diff_chest, spec_chest, 1000.f, vec3(1.0, 1.0, 1.0), vec3(3.0, -3.0, 0.0));
  Mesh cube3("./cube.obj", diff_chest, spec_chest, 1000.f, vec3(0.77, 1.2, 1.0), vec3(-3.0, -3.0, 0.0));

  meshes.push_back(&cube);
  meshes.push_back(&cube2);
  meshes.push_back(&pyramid);
  meshes.push_back(&pyramid2);
  meshes.push_back(&cube3);

  vec3 cameraPos = vec3(0.0f, 0.0f, 2.0f);
  vec3 cameraTarget = vec3(0.0f, 0.0f, 0.0f);
  vec3 cameraDirection = normalize(cameraPos - cameraTarget);
  vec3 up = vec3(0.0f, 1.0f, 0.0f); 
  vec3 cameraRight = normalize(cross(up, cameraDirection));
  vec3 cameraUp = cross(cameraDirection, cameraRight);
  mat4 view = lookAt(cameraPos, cameraDirection, cameraUp);

  mat4 projection = ortho( -5.f, 5.f, -5.f, 5.f, -5.f, 5.f ); // l r b t n f

  while (!glfwWindowShouldClose(window)) {
    processInput(window);

    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUniform3f(glGetUniformLocation(shaderProgram, "lightPos"), light.pos.x, light.pos.y, light.pos.z);
    glUniform3f(glGetUniformLocation(shaderProgram, "lightColor"), light.color.x, light.color.y, light.color.z);
    glUniform3f(glGetUniformLocation(shaderProgram, "viewPos"), cameraPos.x, cameraPos.y, cameraPos.z);


    for(auto m:meshes){
      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, m->specular);
      glUniform1i(glGetUniformLocation(shaderProgram, "specular"), 0);
      glActiveTexture(GL_TEXTURE1);
      glBindTexture(GL_TEXTURE_2D, m->diffuse);
      glUniform1i(glGetUniformLocation(shaderProgram, "diffuse"), 1);
      glUniform1f(glGetUniformLocation(shaderProgram, "shininess"), m->shininess);

      mat4 model = mat4(1.0f);
      model = scale(model, m->scale);
      model = translate(model, m->translation);
      model = rotate(model, (float)glfwGetTime()*rotationCoef, vec3(0.0f, 1.0f, 0.0f));
      glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, &model[0][0]);
      glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, &view[0][0]);
      glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, &projection[0][0]);

      glUseProgram(shaderProgram);
      m->drawMesh();
    }

    glUseProgram(lightShaderProgram);
     glUniform3fv(glGetUniformLocation(lightShaderProgram, "lightColor"), 1, &light.color[0]);
    mat4 transformation = mat4(1.0f);
    transformation = translate(transformation, light.obj.translation);
    transformation = scale(transformation, light.obj.scale);
    transformation = projection * view * transformation;
    glUniformMatrix4fv(glGetUniformLocation(lightShaderProgram, "transformation"), 1, GL_FALSE, &transformation[0][0]);
    light.obj.drawMesh();

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  glfwTerminate();
  return 0;
}

void processInput(GLFWwindow *window) {
  if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) glfwSetWindowShouldClose(window, true);
  else if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) rotationCoef *= 4;
  else if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) rotationCoef /= 4;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
  glViewport(0, 0, width, height);
}