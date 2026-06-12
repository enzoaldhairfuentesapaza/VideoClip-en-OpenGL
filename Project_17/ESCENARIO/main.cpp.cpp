#define GLAD_GL_IMPLEMENTATION
#include <glad/gl.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <iostream>
#include <vector>
#include <cmath>


#include <fstream>
#include <sstream>
#include <filesystem>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"   




const unsigned int SCR_WIDTH  = 1280;
const unsigned int SCR_HEIGHT = 720;

const float PI = 3.14159265359f;


const float CUARTO_ANCHO = 10.0f;   
const float CUARTO_ALTO  = 6.0f;    
const float CUARTO_LARGO = 24.0f;  

// pnales
const float TAM_PANEL    = 1.5f;
const int   FILAS_PANEL  = 3;       
const float PANEL_SEP    = 0.08f;   


const char* VERTEX_SHADER_SRC = R"(#version 330 core
// Do not use any version older than 330!

// Inputs
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal; // For this project, we will actually use normals now
layout (location = 2) in vec2 texCoords;

// Extra outputs, if any
out vec4 myvertex;
out vec3 mynormal;
out vec2 texcoord;

// Uniform variables
uniform mat4 projection;
uniform mat4 modelview;
uniform int istex ;

void main() {
    gl_Position = projection * modelview * vec4(position, 1.0f);
    mynormal = mat3(transpose(inverse(modelview))) * normal ;
    myvertex = modelview * vec4(position, 1.0f) ;
	texcoord = vec2 (0.0, 0.0); // Default value just to prevent errors
	if (istex != 0){
		texcoord = texCoords;
	}
}
)";

const char* FRAGMENT_SHADER_SRC = R"(#version 330 core
// Do not use any version older than 330!

// Inputs to the fragment shader are outputs of the same name of the vertex shader
in vec4 myvertex;
in vec3 mynormal;
in vec2 texcoord;

// Output the frag color
out vec4 fragColor;

uniform sampler2D tex ;
uniform int istex ;
uniform int islight ; // are we lighting.
uniform vec3 color;

// Assume light 0 is directional, light 1 is a point light.
// The actual light values are passed from the main OpenGL program.

uniform vec3 light0dirn ;
uniform vec4 light0color ;
uniform vec4 light1posn ;
uniform vec4 light1color ;

// Material parameters: ambient, diffuse, specular, shininess.
// The ambient is just additive and doesn't multiply the lights.

uniform vec4 ambient ;
uniform vec4 diffuse ;
uniform vec4 specular ;
uniform float shininess ;

vec4 ComputeLight (const in vec3 direction, const in vec4 lightcolor, const in vec3 normal, const in vec3 halfvec, const in vec4 mydiffuse, const in vec4 myspecular, const in float myshininess) {

        float nDotL = dot(normal, direction)  ;
        vec4 lambert = mydiffuse * lightcolor * max (nDotL, 0.0) ;

        float nDotH = dot(normal, halfvec) ;
        vec4 phong = myspecular * lightcolor * pow (max(nDotH, 0.0), myshininess) ;

        vec4 retval = lambert + phong ;
        return retval ;
}

void main (void)
{
    if (istex > 0) fragColor = texture(tex, texcoord);
    else if (islight == 0) fragColor = vec4(color, 1.0f) ;
    else {
        // They eye is always at (0,0,0) looking down -z axis
        // Also compute current fragment position and direction to eye

        const vec3 eyepos = vec3(0,0,0) ;
        vec3 mypos = myvertex.xyz / myvertex.w ; // Dehomogenize current location
        vec3 eyedirn = normalize(eyepos - mypos) ;

        // Compute normal, needed for shading.
        vec3 normal = normalize(mynormal) ;

        // Light 0, directional
        vec3 direction0 = normalize (light0dirn) ;
        vec3 half0 = normalize (direction0 + eyedirn) ;
        vec4 col0 = ComputeLight(direction0, light0color, normal, half0, diffuse, specular, shininess) ;

        // Light 1, point
        vec3 position = light1posn.xyz / light1posn.w ;
        vec3 direction1 = normalize (position - mypos) ; // no attenuation
        vec3 half1 = normalize (direction1 + eyedirn) ;
        vec4 col1 = ComputeLight(direction1, light1color, normal, half1, diffuse, specular, shininess) ;

        fragColor = ambient + col0 + col1 ;
        }
}
)";


class Vertex3D {
	public:
    float x, y, z;
    float u, v;
};
class Vec3
{
	public:
    float x, y, z;
};

Vec3 restar(Vec3 a, Vec3 b)
{
    return { a.x - b.x, a.y - b.y, a.z - b.z };
}

Vec3 sumar(Vec3 a, Vec3 b)
{
    return { a.x + b.x, a.y + b.y, a.z + b.z };
}

Vec3 escalar(Vec3 v, float s)
{
    return { v.x * s, v.y * s, v.z * s };
}

float punto(Vec3 a, Vec3 b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

Vec3 cruz(Vec3 a, Vec3 b)
{
    return {
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    };
}

Vec3 normalizar(Vec3 v)
{
    float len = sqrt(punto(v, v));
    if (len < 1e-6f) return { 0.0f, 0.0f, 0.0f };
    return escalar(v, 1.0f / len);
}

class Matrix4
{
public:

    float m[16];

    Matrix4()
    {
        for (int i = 0; i < 16; i++)
        {
            m[i] = 0.0f;
        }
    }
};

Matrix4 identityMatrix()
{
    Matrix4 result;

    result.m[0]  = 1.0f;
    result.m[5]  = 1.0f;
    result.m[10] = 1.0f;
    result.m[15] = 1.0f;

    return result;
}


Matrix4 multiplyMatrix(const Matrix4& a, const Matrix4& b)
{
    Matrix4 result;

    for (int col = 0; col < 4; col++)
    {
        for (int row = 0; row < 4; row++)
        {
            float sum = 0.0f;

            for (int k = 0; k < 4; k++)
            {
                sum += a.m[k * 4 + row] * b.m[col * 4 + k];
            }

            result.m[col * 4 + row] = sum;
        }
    }

    return result;
}

Matrix4 translationMatrix(float x, float y, float z)
{
    Matrix4 result = identityMatrix();

    result.m[12] = x;
    result.m[13] = y;
    result.m[14] = z;

    return result;
}

Matrix4 scaleMatrix(float x, float y, float z)
{
    Matrix4 result = identityMatrix();

    result.m[0]  = x;
    result.m[5]  = y;
    result.m[10] = z;

    return result;
}

Matrix4 rotationXMatrix(float angle)
{
    Matrix4 result = identityMatrix();

    result.m[5]  = cos(angle);
    result.m[6]  = sin(angle);

    result.m[9]  = -sin(angle);
    result.m[10] = cos(angle);

    return result;
}

Matrix4 rotationYMatrix(float angle)
{
    Matrix4 result = identityMatrix();

    result.m[0]  = cos(angle);
    result.m[2]  = -sin(angle);

    result.m[8]  = sin(angle);
    result.m[10] = cos(angle);

    return result;
}

Matrix4 rotationZMatrix(float angle)
{
    Matrix4 result = identityMatrix();

    result.m[0]  = cos(angle);
    result.m[1]  = sin(angle);

    result.m[4]  = -sin(angle);
    result.m[5]  = cos(angle);

    return result;
}

Matrix4 perspectiveMatrix(float fov, float aspect, float nearPlane, float farPlane)
{
    Matrix4 result;

    float tanHalfFov = tan(fov / 2.0f);

    result.m[0]  = 1.0f / (aspect * tanHalfFov);
    result.m[5]  = 1.0f / tanHalfFov;
    result.m[10] = -(farPlane + nearPlane) / (farPlane - nearPlane);
    result.m[11] = -1.0f;
    result.m[14] = -(2.0f * farPlane * nearPlane) / (farPlane - nearPlane);

    return result;
}

Matrix4 lookAtMatrix(Vec3 eye, Vec3 center, Vec3 up)
{
    Vec3 f = normalizar(restar(center, eye));   
    Vec3 s = normalizar(cruz(f, up));           
    Vec3 u = cruz(s, f);                        

    Matrix4 result = identityMatrix();

    result.m[0]  = s.x;
    result.m[4]  = s.y;
    result.m[8]  = s.z;

    result.m[1]  = u.x;
    result.m[5]  = u.y;
    result.m[9]  = u.z;

    result.m[2]  = -f.x;
    result.m[6]  = -f.y;
    result.m[10] = -f.z;

    result.m[12] = -punto(s, eye);
    result.m[13] = -punto(u, eye);
    result.m[14] =  punto(f, eye);

    return result;
}


Vec3 transformarDireccion(const Matrix4& M, Vec3 v)
{
    return {
        M.m[0] * v.x + M.m[4] * v.y + M.m[8]  * v.z,
        M.m[1] * v.x + M.m[5] * v.y + M.m[9]  * v.z,
        M.m[2] * v.x + M.m[6] * v.y + M.m[10] * v.z
    };
}


Vec3 transformarPunto(const Matrix4& M, Vec3 v)
{
    return {
        M.m[0] * v.x + M.m[4] * v.y + M.m[8]  * v.z + M.m[12],
        M.m[1] * v.x + M.m[5] * v.y + M.m[9]  * v.z + M.m[13],
        M.m[2] * v.x + M.m[6] * v.y + M.m[10] * v.z + M.m[14]
    };
}




void cargarOBJ(const std::string& ruta, std::vector<Vertex3D>& vertices_salida);


GLuint cargarTextura(const std::string& ruta);






class Material
{
	public:
    float ambient[4];
    float diffuse[4];
    float specular[4];
    float shininess;
};


const Material MAT_PANEL   = { {0.20f, 0.20f, 0.22f, 1.0f}, {0.78f, 0.78f, 0.80f, 1.0f}, {0.18f, 0.18f, 0.18f, 1.0f}, 32.0f };
const Material MAT_BOTON   = { {0.04f, 0.04f, 0.04f, 1.0f}, {0.16f, 0.16f, 0.17f, 1.0f}, {0.30f, 0.30f, 0.30f, 1.0f}, 48.0f };
const Material MAT_PISO    = { {0.10f, 0.11f, 0.14f, 1.0f}, {0.40f, 0.44f, 0.55f, 1.0f}, {0.22f, 0.22f, 0.24f, 1.0f}, 64.0f };
const Material MAT_VIGA    = { {0.18f, 0.19f, 0.24f, 1.0f}, {0.64f, 0.67f, 0.82f, 1.0f}, {0.12f, 0.12f, 0.12f, 1.0f}, 16.0f };
const Material MAT_TECHO   = { {0.16f, 0.17f, 0.21f, 1.0f}, {0.58f, 0.61f, 0.74f, 1.0f}, {0.10f, 0.10f, 0.10f, 1.0f}, 16.0f };
const Material MAT_REJILLA = { {0.02f, 0.02f, 0.02f, 1.0f}, {0.08f, 0.08f, 0.09f, 1.0f}, {0.20f, 0.20f, 0.20f, 1.0f}, 24.0f };
const Material MAT_PUERTA  = { {0.07f, 0.07f, 0.08f, 1.0f}, {0.30f, 0.30f, 0.33f, 1.0f}, {0.15f, 0.15f, 0.15f, 1.0f}, 24.0f };


const Vec3  LUZ0_DIRECCION = { 0.3f, 1.0f, 0.4f };
const float LUZ0_COLOR[4]  = { 0.35f, 0.35f, 0.38f, 1.0f };

const Vec3  LUZ1_POSICION  = { 0.0f, CUARTO_ALTO - 0.6f, 0.0f };
const float LUZ1_COLOR[4]  = { 0.85f, 0.85f, 0.90f, 1.0f };




Vec3  camPos   = { 0.0f, 1.7f, 9.0f };
float camYaw   = -PI / 2.0f;   
float camPitch = 0.0f;


bool camaraAerea = false;






Vec3  mueblePos   = { -2.56025f, 0.224581f, -11.0112f };
float muebleRotY  = -0.0181858f;
float muebleEscala = 1.82988f;



Vec3  mueble2Pos   = { 3.9818f, 0.272694f, -0.657206f };
float mueble2RotY  = -1.59472f;
float mueble2Escala = 1.81113f;



unsigned int muebleVAO = 0;
unsigned int muebleVBO = 0;
size_t muebleNumVertices = 0;
unsigned int muebleTexID = 0;





float deltaTime = 0.0f;
float lastFrame = 0.0f;


Matrix4 g_view;


class Uniforms
{
	public:
    int projection, modelview, istex, islight, color;
    int light0dirn, light0color, light1posn, light1color;
    int ambient, diffuse, specular, shininess;
    int tex;
};

Uniforms U;

unsigned int cuboVAO = 0;




void crearCubo()
{
    const float v[] = {
        // posicion              normal
        // frente (+Z)
        -0.5f, -0.5f,  0.5f,     0.0f,  0.0f,  1.0f,
         0.5f, -0.5f,  0.5f,     0.0f,  0.0f,  1.0f,
         0.5f,  0.5f,  0.5f,     0.0f,  0.0f,  1.0f,
        -0.5f,  0.5f,  0.5f,     0.0f,  0.0f,  1.0f,
        // atras (-Z)
         0.5f, -0.5f, -0.5f,     0.0f,  0.0f, -1.0f,
        -0.5f, -0.5f, -0.5f,     0.0f,  0.0f, -1.0f,
        -0.5f,  0.5f, -0.5f,     0.0f,  0.0f, -1.0f,
         0.5f,  0.5f, -0.5f,     0.0f,  0.0f, -1.0f,
        // derecha (+X)
         0.5f, -0.5f,  0.5f,     1.0f,  0.0f,  0.0f,
         0.5f, -0.5f, -0.5f,     1.0f,  0.0f,  0.0f,
         0.5f,  0.5f, -0.5f,     1.0f,  0.0f,  0.0f,
         0.5f,  0.5f,  0.5f,     1.0f,  0.0f,  0.0f,
        // izquierda (-X)
        -0.5f, -0.5f, -0.5f,    -1.0f,  0.0f,  0.0f,
        -0.5f, -0.5f,  0.5f,    -1.0f,  0.0f,  0.0f,
        -0.5f,  0.5f,  0.5f,    -1.0f,  0.0f,  0.0f,
        -0.5f,  0.5f, -0.5f,    -1.0f,  0.0f,  0.0f,
        // arriba (+Y)
        -0.5f,  0.5f,  0.5f,     0.0f,  1.0f,  0.0f,
         0.5f,  0.5f,  0.5f,     0.0f,  1.0f,  0.0f,
         0.5f,  0.5f, -0.5f,     0.0f,  1.0f,  0.0f,
        -0.5f,  0.5f, -0.5f,     0.0f,  1.0f,  0.0f,
        // abajo (-Y)
        -0.5f, -0.5f, -0.5f,     0.0f, -1.0f,  0.0f,
         0.5f, -0.5f, -0.5f,     0.0f, -1.0f,  0.0f,
         0.5f, -0.5f,  0.5f,     0.0f, -1.0f,  0.0f,
        -0.5f, -0.5f,  0.5f,     0.0f, -1.0f,  0.0f,
    };

    unsigned int indices[36];

    for (int cara = 0; cara < 6; cara++)
    {
        int base = cara * 4;
        int i    = cara * 6;

        indices[i + 0] = base + 0;
        indices[i + 1] = base + 1;
        indices[i + 2] = base + 2;

        indices[i + 3] = base + 0;
        indices[i + 4] = base + 2;
        indices[i + 5] = base + 3;
    }

    unsigned int VBO, EBO;

    glGenVertexArrays(1, &cuboVAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(cuboVAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(v), v, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}



unsigned int compilarShader(GLenum tipo, const char* fuente, const char* nombre)
{
    unsigned int shader = glCreateShader(tipo);

    glShaderSource(shader, 1, &fuente, NULL);
    glCompileShader(shader);

    int ok = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);

    if (!ok)
    {
        char log[1024];
        glGetShaderInfoLog(shader, 1024, NULL, log);
        std::cout << "ERROR compilando " << nombre << ":\n" << log << std::endl;
    }

    return shader;
}

unsigned int createShader()
{
    unsigned int vs = compilarShader(GL_VERTEX_SHADER,   VERTEX_SHADER_SRC,   "vertex shader");
    unsigned int fs = compilarShader(GL_FRAGMENT_SHADER, FRAGMENT_SHADER_SRC, "fragment shader");

    unsigned int programa = glCreateProgram();

    glAttachShader(programa, vs);
    glAttachShader(programa, fs);
    glLinkProgram(programa);

    int ok = 0;
    glGetProgramiv(programa, GL_LINK_STATUS, &ok);

    if (!ok)
    {
        char log[1024];
        glGetProgramInfoLog(programa, 1024, NULL, log);
        std::cout << "ERROR enlazando programa:\n" << log << std::endl;
    }

    glDeleteShader(vs);
    glDeleteShader(fs);

    return programa;
}

void obtenerUniforms(unsigned int programa)
{
    U.projection  = glGetUniformLocation(programa, "projection");
    U.modelview   = glGetUniformLocation(programa, "modelview");
    U.istex       = glGetUniformLocation(programa, "istex");
    U.islight     = glGetUniformLocation(programa, "islight");
    U.color       = glGetUniformLocation(programa, "color");
    U.light0dirn  = glGetUniformLocation(programa, "light0dirn");
    U.light0color = glGetUniformLocation(programa, "light0color");
    U.light1posn  = glGetUniformLocation(programa, "light1posn");
    U.light1color = glGetUniformLocation(programa, "light1color");
    U.ambient     = glGetUniformLocation(programa, "ambient");
    U.diffuse     = glGetUniformLocation(programa, "diffuse");
    U.specular    = glGetUniformLocation(programa, "specular");
    U.shininess   = glGetUniformLocation(programa, "shininess");
    U.tex = glGetUniformLocation(programa, "tex");
}





void cargarOBJ(const std::string& ruta, std::vector<Vertex3D>& vertices_salida)
{
    class Vec3Pos { 
		public:
		float x, y, z; 
	};
    class Vec2UV { 
		public:
		float u, v; 
	};

    std::vector<Vec3Pos> pos_temp;
    std::vector<Vec2UV> uv_temp;

    std::ifstream archivo(ruta);
    if (!archivo.is_open()) {
        std::cerr << "Error: No se pudo abrir el archivo " << ruta << std::endl;
        return;
    }

    std::string linea;
    while (std::getline(archivo, linea))
    {
        std::istringstream iss(linea);
        std::string tipo;
        iss >> tipo;

        if (tipo == "v") {
            Vec3Pos v;
            iss >> v.x >> v.y >> v.z;
            pos_temp.push_back(v);
        }
        else if (tipo == "vt") {
            Vec2UV uv;
            iss >> uv.u >> uv.v;
            uv_temp.push_back(uv);
        }
        else if (tipo == "f")
        {
            class FaceVert { 
				public:
				int posIdx, uvIdx; 
			};
            std::vector<FaceVert> cara;
            std::string token;
            while (iss >> token)
            {
                std::istringstream viss(token);
                std::string part;
                std::vector<std::string> parts;
                while (std::getline(viss, part, '/'))
                    parts.push_back(part);

                FaceVert fv = { 0, 0 };
                if (parts.size() > 0 && !parts[0].empty())
                    fv.posIdx = std::stoi(parts[0]) - 1;
                if (parts.size() > 1 && !parts[1].empty())
                    fv.uvIdx = std::stoi(parts[1]) - 1;
                cara.push_back(fv);
            }

            if (cara.size() >= 3)
            {
                auto makeVert = [&](const FaceVert& fv) -> Vertex3D {
                    Vertex3D vert;
                    vert.x = pos_temp[fv.posIdx].x;
                    vert.y = pos_temp[fv.posIdx].y;
                    vert.z = pos_temp[fv.posIdx].z;
                    if (fv.uvIdx >= 0 && fv.uvIdx < (int)uv_temp.size()) {
                        vert.u = uv_temp[fv.uvIdx].u;
                        vert.v = uv_temp[fv.uvIdx].v;
                    } else {
                        vert.u = vert.v = 0.0f;
                    }
                    return vert;
                };

                for (size_t i = 1; i < cara.size() - 1; ++i) {
                    vertices_salida.push_back(makeVert(cara[0]));
                    vertices_salida.push_back(makeVert(cara[i]));
                    vertices_salida.push_back(makeVert(cara[i+1]));
                }
            }
        }
    }
    std::cout << "OBJ cargado: " << vertices_salida.size() / 3 << " triangulos\n";
}


GLuint cargarTextura(const std::string& ruta)
{
    GLuint texID;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_set_flip_vertically_on_load(true);
    int width, height, channels;
    unsigned char* data = stbi_load(ruta.c_str(), &width, &height, &channels, 0);
    if (data) {
        GLenum format = (channels == 4) ? GL_RGBA : GL_RGB;
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        std::cout << "Textura cargada: " << ruta << " (" << width << "x" << height << ")\n";
        stbi_image_free(data);
    } else {
        std::cerr << "Error cargando textura: " << ruta << "\n";
    }
    return texID;
}




void dibujarCuboModel(const Matrix4& model, const Material& mat)
{
    Matrix4 modelview = multiplyMatrix(g_view, model);

    glUniformMatrix4fv(U.modelview, 1, GL_FALSE, modelview.m);

    glUniform1i(U.islight, 1);
    glUniform4fv(U.ambient,  1, mat.ambient);
    glUniform4fv(U.diffuse,  1, mat.diffuse);
    glUniform4fv(U.specular, 1, mat.specular);
    glUniform1f(U.shininess, mat.shininess);
    glUniform1i(U.istex, 0);

    glBindVertexArray(cuboVAO);
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
}


void dibujarCubo(float px, float py, float pz,
                 float sx, float sy, float sz,
                 const Material& mat)
{
    Matrix4 model = multiplyMatrix(translationMatrix(px, py, pz),
                                   scaleMatrix(sx, sy, sz));
    dibujarCuboModel(model, mat);
}


void dibujarCuboEmisivo(float px, float py, float pz,
                        float sx, float sy, float sz,
                        float r, float g, float b)
{
    Matrix4 model     = multiplyMatrix(translationMatrix(px, py, pz),
                                       scaleMatrix(sx, sy, sz));
    Matrix4 modelview = multiplyMatrix(g_view, model);

    glUniformMatrix4fv(U.modelview, 1, GL_FALSE, modelview.m);

    glUniform1i(U.islight, 0);
    glUniform3f(U.color, r, g, b);
    glUniform1i(U.istex, 0);

    glBindVertexArray(cuboVAO);
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
}


void dibujarCuarto()
{
    float mitadAncho = CUARTO_ANCHO * 0.5f;
    float mitadLargo = CUARTO_LARGO * 0.5f;

   
    dibujarCubo(0.0f, -0.1f, 0.0f, CUARTO_ANCHO, 0.2f, CUARTO_LARGO, MAT_PISO);

   
    int colsLado = (int)(CUARTO_LARGO / TAM_PANEL);

    for (int lado = 0; lado < 2; lado++)
    {
        float x      = (lado == 0 ? -1.0f : 1.0f) * mitadAncho;
        float adentro = (lado == 0 ? 1.0f : -1.0f);   

        for (int f = 0; f < FILAS_PANEL; f++)
        {
            for (int c = 0; c < colsLado; c++)
            {
                float y = TAM_PANEL * 0.5f + f * TAM_PANEL;
                float z = -mitadLargo + TAM_PANEL * 0.5f + c * TAM_PANEL;

               
                dibujarCubo(x, y, z,
                            0.35f, TAM_PANEL - PANEL_SEP, TAM_PANEL - PANEL_SEP,
                            MAT_PANEL);

                
                dibujarCubo(x + adentro * 0.20f, y, z,
                            0.06f, 0.16f, 0.16f, MAT_BOTON);
            }
        }

        
        dibujarCubo(x, FILAS_PANEL * TAM_PANEL + 0.75f, 0.0f,
                    0.30f, 1.5f, CUARTO_LARGO, MAT_PANEL);

        
        dibujarCubo(x + adentro * 0.55f, 5.1f, 0.0f,
                    0.85f, 0.95f, CUARTO_LARGO, MAT_VIGA);

        
        dibujarCuboEmisivo(x + adentro * 0.35f, 4.58f, 0.0f,
                           0.14f, 0.10f, CUARTO_LARGO, 1.0f, 1.0f, 1.0f);
        dibujarCuboEmisivo(x + adentro * 0.35f, 5.66f, 0.0f,
                           0.14f, 0.10f, CUARTO_LARGO, 1.0f, 1.0f, 1.0f);
    }

   
    int   colsFondo = 7;
    float tamF      = CUARTO_ANCHO / colsFondo;

    for (int f = 0; f < 4; f++)
    {
        for (int c = 0; c < colsFondo; c++)
        {
            float xC = -mitadAncho + tamF * 0.5f + c * tamF;
            float yC = TAM_PANEL * 0.5f + f * TAM_PANEL;

            
            bool esPuerta = (fabs(xC) < tamF * 0.6f) && (f < 2);
            if (esPuerta) continue;

            dibujarCubo(xC, yC, -mitadLargo,
                        tamF - PANEL_SEP, TAM_PANEL - PANEL_SEP, 0.35f,
                        MAT_PANEL);

            dibujarCubo(xC, yC, -mitadLargo + 0.20f,
                        0.16f, 0.16f, 0.06f, MAT_BOTON);
        }
    }

    
    dibujarCubo(0.0f, 1.5f, -mitadLargo - 0.05f,
                tamF + 0.3f, 3.0f, 0.25f, MAT_PUERTA);

    
    if (camaraAerea) return;

    
    dibujarCubo(0.0f, CUARTO_ALTO + 0.1f, 0.0f,
                CUARTO_ANCHO, 0.2f, CUARTO_LARGO, MAT_TECHO);

    
    for (int i = 0; i < 3; i++)
    {
        dibujarCubo(-2.0f + 2.0f * i, CUARTO_ALTO - 0.25f, 0.0f,
                    0.12f, 0.12f, CUARTO_LARGO, MAT_REJILLA);
    }

    
    int nTransversales = (int)(CUARTO_LARGO / 2.0f);

    for (int i = 0; i <= nTransversales; i++)
    {
        dibujarCubo(0.0f, CUARTO_ALTO - 0.25f, -mitadLargo + i * 2.0f,
                    6.0f, 0.10f, 0.10f, MAT_REJILLA);
    }


    for (int i = 0; i < 6; i++)
    {
        float z = -mitadLargo + 2.0f + i * 4.0f;

        dibujarCuboEmisivo(-1.0f, CUARTO_ALTO - 0.12f, z,
                           0.15f, 0.08f, 1.6f, 1.0f, 1.0f, 1.0f);
        dibujarCuboEmisivo( 1.0f, CUARTO_ALTO - 0.12f, z,
                           0.15f, 0.08f, 1.6f, 1.0f, 1.0f, 1.0f);
    }
}




void dibujarMueble()
{
    if (muebleNumVertices == 0) return;

    
    Matrix4 modelo = translationMatrix(mueblePos.x, mueblePos.y, mueblePos.z);
    Matrix4 rot    = rotationYMatrix(muebleRotY);
    Matrix4 escala = scaleMatrix(muebleEscala, muebleEscala, muebleEscala);

    Matrix4 temp = multiplyMatrix(rot, escala);     
    Matrix4 model = multiplyMatrix(modelo, temp);  

   
    Matrix4 modelview = multiplyMatrix(g_view, model);
    glUniformMatrix4fv(U.modelview, 1, GL_FALSE, modelview.m);

    
    glUniform1i(U.istex, 1);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, muebleTexID);
    glUniform1i(U.tex, 0);   

    
    glBindVertexArray(muebleVAO);
    glDrawArrays(GL_TRIANGLES, 0, (GLsizei)muebleNumVertices);
}


void dibujarMuebleEn(const Vec3& pos, float rotY, float escala)
{
    if (muebleNumVertices == 0) return;


    Matrix4 modelo = translationMatrix(pos.x, pos.y, pos.z);
    Matrix4 rot    = rotationYMatrix(rotY);
    Matrix4 escalaMat = scaleMatrix(escala, escala, escala);

    Matrix4 temp = multiplyMatrix(rot, escalaMat);    
    Matrix4 model = multiplyMatrix(modelo, temp);     

    Matrix4 modelview = multiplyMatrix(g_view, model);
    glUniformMatrix4fv(U.modelview, 1, GL_FALSE, modelview.m);

    glUniform1i(U.istex, 1);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, muebleTexID);
    glUniform1i(U.tex, 0);

    glBindVertexArray(muebleVAO);
    glDrawArrays(GL_TRIANGLES, 0, (GLsizei)muebleNumVertices);
}







void renderizarEscena(int anchoFb, int altoFb)
{
    
    float aspecto = (altoFb > 0) ? (float)anchoFb / (float)altoFb : 1.0f;
    Matrix4 projection = perspectiveMatrix(45.0f * PI / 180.0f, aspecto, 0.1f, 120.0f);
    glUniformMatrix4fv(U.projection, 1, GL_FALSE, projection.m);

   
    if (camaraAerea)
    {

        g_view = lookAtMatrix({ 0.0f, 17.0f, 0.0f },
                              { 0.0f, 0.0f, 0.0f },
                              { 1.0f, 0.0f, 0.0f });
    }
    else
    {
        Vec3 frente = {
            cosf(camPitch) * cosf(camYaw),
            sinf(camPitch),
            cosf(camPitch) * sinf(camYaw)
        };

        g_view = lookAtMatrix(camPos, sumar(camPos, frente), { 0.0f, 1.0f, 0.0f });
    }


    Vec3 luz0eye = transformarDireccion(g_view, LUZ0_DIRECCION);
    Vec3 luz1eye = transformarPunto(g_view, LUZ1_POSICION);

    glUniform3f(U.light0dirn, luz0eye.x, luz0eye.y, luz0eye.z);
    glUniform4fv(U.light0color, 1, LUZ0_COLOR);
    glUniform4f(U.light1posn, luz1eye.x, luz1eye.y, luz1eye.z, 1.0f);
    glUniform4fv(U.light1color, 1, LUZ1_COLOR);

    glUniform1i(U.istex, 0);   

    glUniform1i(U.istex, 0);
    dibujarCuarto();
    dibujarMueble();

    dibujarMuebleEn(mueble2Pos, mueble2RotY, mueble2Escala);

 
}



void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(window, true);
    }

   
    if (key == GLFW_KEY_C && action == GLFW_PRESS)
    {
        camaraAerea = !camaraAerea;
    }


       
    if (key == GLFW_KEY_M && action == GLFW_PRESS)
    {
        std::cout << "\ndonde esta el mueble " << std::endl;
        std::cout << "Vec3  mueblePos   = { " << mueblePos.x << "f, " << mueblePos.y << "f, " << mueblePos.z << "f };" << std::endl;
        std::cout << "float muebleRotY  = " << muebleRotY << "f;" << std::endl;
        std::cout << "float muebleEscala = " << muebleEscala << "f;" << std::endl;
        std::cout << "\n" << std::endl;
    }
}


void procesarInput(GLFWwindow* window)
{
    float velMover = 6.0f * deltaTime;        
    float velGirar = 1.8f * deltaTime;        

    
    Vec3 frente = normalizar({ cosf(camYaw), 0.0f, sinf(camYaw) });
    Vec3 derecha = normalizar(cruz(frente, { 0.0f, 1.0f, 0.0f }));

    
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
        camPos = sumar(camPos, escalar(frente, velMover));

    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
        camPos = restar(camPos, escalar(frente, velMover));

    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camPos = restar(camPos, escalar(derecha, velMover));

    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camPos = sumar(camPos, escalar(derecha, velMover));

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camPos.y += velMover;

    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camPos.y -= velMover;

    
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
        camYaw -= velGirar;

    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
        camYaw += velGirar;

    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
        camPitch += velGirar;

    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
        camPitch -= velGirar;

   
    if (camPitch >  1.45f) camPitch =  1.45f;
    if (camPitch < -1.45f) camPitch = -1.45f;



    
    float velTraslacion = 5.0f * deltaTime;   
    float velRotacion   = 1.8f * deltaTime;   
    float velEscala     = 1.2f * deltaTime;   

    
    if (glfwGetKey(window, GLFW_KEY_U) == GLFW_PRESS)
        muebleRotY -= velRotacion;
    if (glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS)
        muebleRotY += velRotacion;

    
    if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS)
        muebleEscala += velEscala;
    if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS)
        muebleEscala -= velEscala;
    if (muebleEscala < 0.1f) muebleEscala = 0.1f;

    
    if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS)
        mueblePos.x += velTraslacion;
    if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS)
        mueblePos.x -= velTraslacion;

    
    if (glfwGetKey(window, GLFW_KEY_J) == GLFW_PRESS)
        mueblePos.z -= velTraslacion;  
    if (glfwGetKey(window, GLFW_KEY_H) == GLFW_PRESS)
        mueblePos.z += velTraslacion;   

    
    if (glfwGetKey(window, GLFW_KEY_Y) == GLFW_PRESS)
        mueblePos.y += velTraslacion;
    if (glfwGetKey(window, GLFW_KEY_N) == GLFW_PRESS)
        mueblePos.y -= velTraslacion;
}



int main()
{
    glfwInit();

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(
        SCR_WIDTH,
        SCR_HEIGHT,
        "Project 35 - Virtual Insanity",
        NULL,
        NULL
    );

    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetKeyCallback(window, key_callback);

    if (!gladLoadGL(glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    glEnable(GL_DEPTH_TEST);

    unsigned int programa = createShader();
    obtenerUniforms(programa);

    crearCubo();


        // --- Cargar el mueble (sofá) desde OBJ ---
    std::vector<Vertex3D> verticesMueble;
    cargarOBJ("E:/glfw-master/OwnProjects/Project_16/archivo.obj", verticesMueble);
    muebleNumVertices = verticesMueble.size();

    // Crear VAO, VBO para el mueble
    glGenVertexArrays(1, &muebleVAO);
    glGenBuffers(1, &muebleVBO);

    glBindVertexArray(muebleVAO);
    glBindBuffer(GL_ARRAY_BUFFER, muebleVBO);
    glBufferData(GL_ARRAY_BUFFER, verticesMueble.size() * sizeof(Vertex3D), verticesMueble.data(), GL_STATIC_DRAW);

    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex3D), (void*)0);
    glEnableVertexAttribArray(0);

    
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex3D), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);

    
    muebleTexID = cargarTextura("E:/glfw-master/OwnProjects/Project_16/texturas/Sofa_Base_Color.png");






    std::cout << "Controles: Q/E adelante/atras | A/D izquierda/derecha | W/S subir/bajar | flechas girar vista | C camara aerea | ESC salir" << std::endl;

    while (!glfwWindowShouldClose(window))
    {
        float ahora = (float)glfwGetTime();
        deltaTime = ahora - lastFrame;
        lastFrame = ahora;

        procesarInput(window);

        glClearColor(0.02f, 0.02f, 0.03f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(programa);

        int anchoFb, altoFb;
        glfwGetFramebufferSize(window, &anchoFb, &altoFb);

        renderizarEscena(anchoFb, altoFb);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}