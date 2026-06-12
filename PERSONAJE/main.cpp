#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <iostream>
#include <iomanip>
#include <filesystem>
#include <sstream>

#ifndef GLAD_GL_IMPLEMENTATION
    #define GLAD_GL_IMPLEMENTATION
    #include <glad/gl.h>
#endif

#ifndef GLFW_INCLUDE_NONE
    #define GLFW_INCLUDE_NONE
    #include <GLFW/glfw3.h>
#endif

#ifndef STB_IMAGE_IMPLEMENTATION
    #define STB_IMAGE_IMPLEMENTATION
    #include "stb_image.h"
#endif

#ifndef INCLUDE_STB_IMAGE_WRITE_H
    #define STB_IMAGE_WRITE_IMPLEMENTATION
    #include "stb_image_write.h"
#endif

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace fs = std::filesystem;

// =========================================================================
// DEFINICIÓN DE MEMORIA GLOBAL (Asignación real requerida por geometry3.h)
// =========================================================================
int mouseoldx, mouseoldy; 
int windowWidth = 500, windowHeight = 500; 

GLdouble eyeloc = 2.0; 
float mannequinRotationY = 0.0f;
float mannequinRotationX = 0.0f;
GLuint vertexshader, fragmentshader, shaderprogram;
GLuint projectionPos, modelviewPos, colorPos;
glm::mat4 projection, modelview;

GLuint islight;
GLuint light0dirn, light0color;
GLuint light1posn, light1color;
GLuint ambient, diffuse, specular, shininess;

// Instanciación de variables de geometría del header
GLuint VAOs[6], teapotVAO; 
GLuint buffers[11], teapotbuffers[3]; 
GLuint objects[2]; 
GLenum PrimType[2];
GLsizei NumElems[2];

GLubyte woodtexture[256][256][3]; 
GLuint texNames[1]; 
GLuint istex; 

// =========================================================================
// INCLUSIONES DEL PROYECTO
// =========================================================================
#include "shaders.h"
#include "geometry3.h"

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);

void printHelp() {
    std::cout << "--- Controles ---\nArrastrar Click Izquierdo: Rotar Camara\nClick Derecho: Resetear Camara\nESC: Salir\n";
}

void transformvec(const GLfloat input[4], GLfloat output[4]) 
{
    glm::vec4 inputvec(input[0], input[1], input[2], input[3]);
    glm::vec4 outputvec = modelview * inputvec;
    output[0] = outputvec[0]; output[1] = outputvec[1];
    output[2] = outputvec[2]; output[3] = outputvec[3];
}

void deleteBuffers() 
{
    glDeleteVertexArrays(numobjects + ncolors, VAOs);
    glDeleteVertexArrays(1, &teapotVAO); 
    glDeleteBuffers(numperobj * numobjects + ncolors, buffers);
    glDeleteBuffers(3, teapotbuffers);
}

void display()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    const GLfloat one[] = {1.0f, 1.0f, 1.0f, 1.0f};
    const GLfloat ambientValue[] = {0.4f, 0.4f, 0.4f, 1.0f};
    const GLfloat diffuseValue[] = {1.0f, 1.0f, 1.0f, 1.0f};
    const GLfloat high[] = {30.0f};
    const GLfloat lightDir[] = {0.5f, 0.5f, 1.0f, 0.0f};
	
    GLfloat transformedLight[4];
    transformvec(lightDir, transformedLight);

    glUniform3fv(light0dirn, 1, transformedLight);
    glUniform4fv(ambient, 1, ambientValue);
    glUniform4fv(diffuse, 1, diffuseValue);
    glUniform4fv(specular, 1, one);
    glUniform1fv(shininess, 1, high);
    glUniform1i(islight, 1);

    // Mapeo básico de textura activo
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texNames[0]);
    if (istex != (GLuint)-1) glUniform1i(istex, 1); 

	glm::mat4 model = modelview;

	model = glm::rotate(
		model,
		glm::radians(mannequinRotationY),
		glm::vec3(0.0f, 0.0f, 1.0f)
	);

	model = glm::rotate(
		model,
		glm::radians(mannequinRotationX),
		glm::vec3(1.0f, 0.0f, 0.0f)
	);

	model = glm::rotate(
		model,
		glm::pi<float>() / 2.0f,
		glm::vec3(1.0f, 0.0f, 0.0f)
	);

    float size = 0.3f;
    model = glm::scale(model, glm::vec3(size, size, size));

    glUniformMatrix4fv(modelviewPos, 1, GL_FALSE, &model[0][0]);

    drawteapot(); 
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{    
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) 
    {
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);
        mouseoldx = xpos; mouseoldy = ypos;
    }
    else if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
    { 
        eyeloc = 2.0;
        modelview = glm::lookAt(glm::vec3(0.0f, -eyeloc, eyeloc), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 1.0f));
        glUniformMatrix4fv(modelviewPos, 1, GL_FALSE, &modelview[0][0]);
    }
}

void mousedrag(int x, int y) 
{
    int yloc = y - mouseoldy;
    eyeloc += 0.005 * yloc;
    if (eyeloc < 0.1) eyeloc = 0.1;
    mouseoldy = y;

    modelview = glm::lookAt(glm::vec3(0.0f, -eyeloc, eyeloc), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 1.0f));
    glUniformMatrix4fv(modelviewPos, 1, GL_FALSE, &modelview[0][0]);
}

void init(std::string &out)
{
    printHelp();
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f); 

    projection = glm::mat4(1.0f); 
    modelview = glm::lookAt(glm::vec3(0.0f, -eyeloc, eyeloc), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 1.0f));

    std::string base_path_file = out + "\\glfw-master\\OwnProjects\\Final";
    std::string vertex_file = base_path_file + "\\shaders\\light.vert.glsl";
    std::string fragment_file = base_path_file + "\\shaders\\light.frag.glsl";
    
    // --- NUEVA RUTA: Apuntamos al JPG real dentro de la carpeta de modelos ---
    std::string texture_file = base_path_file + "\\models\\Material_baseColor.jpg";
    std::string path_to_model = base_path_file + "\\models\\manniquin.obj";

    vertexshader = initshaders(GL_VERTEX_SHADER, vertex_file.c_str());
    fragmentshader = initshaders(GL_FRAGMENT_SHADER, fragment_file.c_str());
    shaderprogram = initprogram(vertexshader, fragmentshader);

    islight     = glGetUniformLocation(shaderprogram, "islight");
    light0dirn  = glGetUniformLocation(shaderprogram, "light0dirn");
    ambient     = glGetUniformLocation(shaderprogram, "ambient");
    diffuse     = glGetUniformLocation(shaderprogram, "diffuse");
    specular    = glGetUniformLocation(shaderprogram, "specular");
    shininess   = glGetUniformLocation(shaderprogram, "shininess");
    
    projectionPos = glGetUniformLocation(shaderprogram, "projection");
    modelviewPos  = glGetUniformLocation(shaderprogram, "modelview");

    glGenVertexArrays(numobjects + ncolors, VAOs);
    glGenVertexArrays(1, &teapotVAO);
    glGenBuffers(numperobj * numobjects + ncolors + 1, buffers); 
    glGenBuffers(4, teapotbuffers);

    // =========================================================================
    // NUEVO CARGADOR DE TEXTURA JPG (Reemplaza a inittexture)
    // =========================================================================
    int width, height, nrChannels;
    // Forzamos la inversión vertical porque las texturas de OpenGL se cargan al revés
    stbi_set_flip_vertically_on_load(true); 
    unsigned char *data = stbi_load(texture_file.c_str(), &width, &height, &nrChannels, 0);
    
    if (data) {
        glGenTextures(1, texNames);
        glBindTexture(GL_TEXTURE_2D, texNames[0]);
        
        // Configuración de repetición y filtrado
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // Detectar si el canal es RGB o RGBA (por seguridad)
        GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;
        
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        
        stbi_image_free(data);
        std::cout << "Textura cargada exitosamente: " << width << "x" << height << std::endl;
    } else {
        std::cout << "Error critico al cargar la textura JPG en: " << texture_file << std::endl;
    }

    // Vinculamos los Uniforms correspondientes en el shader para texturas
    GLint texsampler = glGetUniformLocation(shaderprogram, "tex");
    glUseProgram(shaderprogram);
    if(texsampler != -1) glUniform1i(texsampler, 0);
    istex = glGetUniformLocation(shaderprogram, "istex");
    // =========================================================================

    loadteapot(path_to_model);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS); 
}

int main(int argc, char** argv)
{
    fs::path p = fs::current_path();
    fs::path p_current = p.parent_path().parent_path(); 

    std::stringstream ss;
    ss << std::quoted(p_current.string());
    std::string out;
    ss >> std::quoted(out);

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(windowWidth, windowHeight, "Maniquí 3D", NULL, NULL);
    if (window == NULL) {
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);

    if (!gladLoadGL(glfwGetProcAddress)) {
        return -1;
    }

    init(out); 
    
    projection = glm::perspective(30.0f / 180.0f * glm::pi<float>(), (GLfloat)windowWidth / (GLfloat)windowHeight, 1.0f, 10.0f);
    glUseProgram(shaderprogram);
    glUniformMatrix4fv(projectionPos, 1, GL_FALSE, &projection[0][0]);

    while (!glfwWindowShouldClose(window))
    {
        display();
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    deleteBuffers();
    glfwTerminate();
    return 0;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    windowWidth = width; windowHeight = height;
    glViewport(0, 0, width, height);
    projection = glm::perspective(30.0f / 180.0f * glm::pi<float>(), (GLfloat)width / (GLfloat)height, 1.0f, 10.0f);
    glUniformMatrix4fv(projectionPos, 1, GL_FALSE, &projection[0][0]);
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }
	if (key == GLFW_KEY_LEFT && action == GLFW_PRESS)
	{
		mannequinRotationY += 10.0f;
	}

	if (key == GLFW_KEY_RIGHT && action == GLFW_PRESS)
	{
		mannequinRotationY -= 10.0f;
	}

	if (key == GLFW_KEY_UP && action == GLFW_PRESS)
	{
		mannequinRotationX += 10.0f;
	}

	if (key == GLFW_KEY_DOWN && action == GLFW_PRESS)
	{
		mannequinRotationX -= 10.0f;
	}
}