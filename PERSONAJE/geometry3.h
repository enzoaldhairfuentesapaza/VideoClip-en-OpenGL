/**** Basic setup for defining and drawing objects ****/
#ifndef __INCLUDEGEOMETRY
#define __INCLUDEGEOMETRY

#include <vector>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <limits>       // std::numeric_limits
#include <glm/glm.hpp>

const int numobjects = 2; 
const int numperobj = 3;
const int ncolors = 4;

extern GLuint VAOs[numobjects + ncolors], teapotVAO; 
extern GLuint buffers[numperobj*numobjects + ncolors + 1], teapotbuffers[3]; 
extern GLuint objects[numobjects]; 
extern GLenum PrimType[numobjects];
extern GLsizei NumElems[numobjects];

// Externas declaradas en main.cpp que necesita este header
extern GLubyte woodtexture[256][256][3];
extern GLuint texNames[1];
extern GLuint istex;

// For the geometry of the teapot
inline std::vector <glm::vec3> teapotVertices;
inline std::vector <glm::vec3> teapotNormals;
inline std::vector <unsigned int> teapotIndices;
inline std::vector <glm::mat4> modelviewStack;

enum { Vertices, Colors, Elements }; 
enum { FLOOR, CUBE }; 

const GLfloat floorverts[4][3] = {
	{ 0.5, 0.5, 0.0 },{ -0.5, 0.5, 0.0 },{ -0.5, -0.5, 0.0 },{ 0.5, -0.5, 0.0 }
};
const GLfloat floorcol[4][3] = {
	{ 1.0, 1.0, 1.0 },{ 1.0, 1.0, 1.0 },{ 1.0, 1.0, 1.0 },{ 1.0, 1.0, 1.0 }
};
const GLubyte floorinds[1][6] = { { 0, 1, 2, 0, 2, 3 } };
const GLfloat floortex[4][2] = {
	{ 1.0, 1.0 },{ 0.0, 1.0 },{ 0.0, 0.0 },{ 1.0, 0.0 }
};

const GLfloat wd = 0.1;
const GLfloat ht = 0.5;
const GLfloat _cubecol[4][3] = {
	{ 1.0, 0.0, 0.0 },{ 0.0, 1.0, 0.0 },{ 0.0, 0.0, 1.0 },{ 1.0, 1.0, 0.0 } };

const GLfloat cubeverts[8][3] = {
	{ -wd, -wd, 0.0 },{ -wd, wd, 0.0 },{ wd, wd, 0.0 },{ wd, -wd, 0.0 },
	{ -wd, -wd, ht },{ wd, -wd, ht },{ wd, wd, ht },{ -wd, wd, ht }
};
inline GLfloat cubecol[12][3];
const GLubyte cubeinds[12][3] = {
	{ 0, 1, 2 },{ 0, 2, 3 }, 
	{ 4, 5, 6 },{ 4, 6, 7 }, 
	{ 0, 4, 7 },{ 0, 7, 1 }, 
	{ 0, 3, 5 },{ 0, 5, 4 }, 
	{ 3, 2, 6 },{ 3, 6, 5 }, 
	{ 1, 7, 6 },{ 1, 6, 2 } 
};

void initobject(GLuint object, GLfloat * vert, GLint sizevert, GLfloat * col, GLint sizecol, GLubyte * inds, GLint sizeind, GLenum type);
void drawobject(GLuint object);
void initcubes(GLuint object, GLfloat * vert, GLint sizevert, GLubyte * inds, GLint sizeind, GLenum type);
void drawcolor(GLuint object, GLuint color);
void inittexture(const char * filename, GLuint program);
void drawtexture(GLuint object, GLuint texture);
void loadteapot(std::string& path_to_teapot_obj);
void drawteapot();

inline void initobject(GLuint object, GLfloat * vert, GLint sizevert, GLfloat * col, GLint sizecol, GLubyte * inds, GLint sizeind, GLenum type) {
	int offset = object * numperobj;
	glBindVertexArray(VAOs[object]);
	glBindBuffer(GL_ARRAY_BUFFER, buffers[Vertices + offset]);
	glBufferData(GL_ARRAY_BUFFER, sizevert, vert, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), 0);

	glBindBuffer(GL_ARRAY_BUFFER, buffers[Colors + offset]);
	glBufferData(GL_ARRAY_BUFFER, sizecol, col, GL_STATIC_DRAW);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffers[Elements + offset]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeind, inds, GL_STATIC_DRAW);
	PrimType[object] = type;
	NumElems[object] = sizeind;
	glBindVertexArray(0);
}

inline void inittexture(const char * filename, GLuint program) {
	int i, j, k;
	FILE * fp = std::fopen(filename, "rb");
    if (!fp) {
        std::cout<<"\nError texture file - File opening failed : " << filename << "\n";
		return;
    }
	
	int res = fscanf(fp, "%*s %*d %*d %*d%*c");
	for (i = 0; i < 256; i++)
		for (j = 0; j < 256; j++)
			for (k = 0; k < 3; k++) {
				res = fscanf(fp, "%c", &(woodtexture[i][j][k]));
			}
	fclose(fp);

	glGenTextures(1, texNames);
	glBindVertexArray(VAOs[FLOOR]);
	glBindBuffer(GL_ARRAY_BUFFER, buffers[numobjects*numperobj + ncolors]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(floortex), floortex, GL_STATIC_DRAW);
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), 0);

	glActiveTexture(GL_TEXTURE0);
	glEnable(GL_TEXTURE_2D);

	glBindTexture(GL_TEXTURE_2D, texNames[0]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 256, 256, 0, GL_RGB, GL_UNSIGNED_BYTE, woodtexture);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glBindVertexArray(0);

	GLint texsampler = glGetUniformLocation(program, "tex");
	glUniform1i(texsampler, 0);
	istex = glGetUniformLocation(program, "istex");
}

inline void initcubes(GLuint object, GLfloat * vert, GLint sizevert, GLubyte * inds, GLint sizeind, GLenum type) {
	for (int i = 0; i < ncolors; i++) {
		for (int j = 0; j < 8; j++)
			for (int k = 0; k < 3; k++)
				cubecol[j][k] = _cubecol[i][k];
		glBindVertexArray(VAOs[object + i]);
		int offset = object * numperobj;
		int base = numobjects * numperobj;
		glBindBuffer(GL_ARRAY_BUFFER, buffers[Vertices + offset]);
		glBufferData(GL_ARRAY_BUFFER, sizevert, vert, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), 0);

		glBindBuffer(GL_ARRAY_BUFFER, buffers[base + i]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(cubecol), cubecol, GL_STATIC_DRAW);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffers[Elements + offset]);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeind, inds, GL_STATIC_DRAW);
		PrimType[object] = type;
		NumElems[object] = sizeind;
		glBindVertexArray(0);
	}
}

inline void drawcolor(GLuint object, GLuint color) {
	glBindVertexArray(VAOs[object + color]);
	glDrawElements(PrimType[object], NumElems[object], GL_UNSIGNED_BYTE, 0);
	glBindVertexArray(0);
}

inline void drawtexture(GLuint object, GLuint texture) {
	glBindTexture(GL_TEXTURE_2D, texture);
	glBindVertexArray(VAOs[object]);
	glDrawElements(PrimType[object], NumElems[object], GL_UNSIGNED_BYTE, 0);
	glBindVertexArray(0);
}

inline void drawobject(GLuint object) {
	glBindVertexArray(VAOs[object]);
	glDrawElements(PrimType[object], NumElems[object], GL_UNSIGNED_BYTE, 0);
	glBindVertexArray(0);
}

inline void loadteapot(std::string &path_to_teapot_obj) {
	FILE* fp;
	float x, y, z;
	int c1, c2;

	float minY = std::numeric_limits<float>::infinity(), minZ = std::numeric_limits<float>::infinity();
	float maxY = -std::numeric_limits<float>::infinity(), maxZ = -std::numeric_limits<float>::infinity();

	fp = fopen(path_to_teapot_obj.c_str(), "rb");
	if (fp == NULL) {
		std::cerr << "Error loading file: " << path_to_teapot_obj << std::endl;
		exit(-1);
	}

	// Vectores temporales para almacenar los datos directos del archivo .obj
	std::vector<glm::vec3> temp_vertices;
	std::vector<glm::vec2> temp_uvs;
	std::vector<glm::vec3> temp_normals;

	// Estructura para asociar los índices compuestos del formato OBJ (v/vt/vn)
	struct VertexIndex { int v, vt, vn; };
	std::vector<VertexIndex> face_indices;

	char lineHeader[128];
	while (true) {
		int res = fscanf(fp, "%s", lineHeader);
		if (res == EOF) break; 

		if (strcmp(lineHeader, "v") == 0) {
			fscanf(fp, "%f %f %f\n", &x, &y, &z);
			temp_vertices.push_back(glm::vec3(x, y, z));
			if (y < minY) minY = y;
			if (z < minZ) minZ = z;
			if (y > maxY) maxY = y;
			if (z > maxZ) maxZ = z;
		}
		else if (strcmp(lineHeader, "vt") == 0) {
			float u, v_coord;
			fscanf(fp, "%f %f\n", &u, &v_coord);
			temp_uvs.push_back(glm::vec2(u, v_coord)); // Guardamos las coordenadas UV
		}
		else if (strcmp(lineHeader, "vn") == 0) {
			fscanf(fp, "%f %f %f\n", &x, &y, &z);
			temp_normals.push_back(glm::vec3(x, y, z));
		}
		else if (strcmp(lineHeader, "f") == 0) {
			int vIdx[3], vtIdx[3], vnIdx[3];
			int matches = fscanf(fp, "%d/%d/%d %d/%d/%d %d/%d/%d\n", 
				&vIdx[0], &vtIdx[0], &vnIdx[0],
				&vIdx[1], &vtIdx[1], &vnIdx[1],
				&vIdx[2], &vtIdx[2], &vnIdx[2]);
			
			if (matches == 9) {
				face_indices.push_back({vIdx[0]-1, vtIdx[0]-1, vnIdx[0]-1});
				face_indices.push_back({vIdx[1]-1, vtIdx[1]-1, vnIdx[1]-1});
				face_indices.push_back({vIdx[2]-1, vtIdx[2]-1, vnIdx[2]-1});
			}
		}
	}
	fclose(fp); 

	// Limpiamos los vectores globales del proyecto para reconstruirlos de forma lineal
	teapotVertices.clear();
	teapotNormals.clear();
	teapotIndices.clear();
	std::vector<glm::vec2> final_uvs;

	// Alineamos los datos desordenados del archivo .obj en arreglos indexados puros
	for (unsigned int i = 0; i < face_indices.size(); i++) {
		VertexIndex lookup = face_indices[i];
		
		teapotVertices.push_back(temp_vertices[lookup.v]);
		
		if (lookup.vt >= 0 && lookup.vt < (int)temp_uvs.size()) {
			final_uvs.push_back(temp_uvs[lookup.vt]);
		} else {
			final_uvs.push_back(glm::vec2(0.0f, 0.0f));
		}

		if (lookup.vn >= 0 && lookup.vn < (int)temp_normals.size()) {
			teapotNormals.push_back(temp_normals[lookup.vn]);
		} else {
			teapotNormals.push_back(glm::vec3(0.0f, 1.0f, 0.0f));
		}

		teapotIndices.push_back(i);
	}

	// Centrar el maniquí de manera idéntica al código original
	float avgY = (minY + maxY) / 2.0f - 0.02f;
	float avgZ = (minZ + maxZ) / 2.0f;
	for (unsigned int i = 0; i < teapotVertices.size(); ++i) {
		teapotVertices[i] = teapotVertices[i] - glm::vec3(0.0f, avgY, avgZ);
	}

	std::cout << "\n[OBJ Loader] Datos procesados para el Maniqui:";
	std::cout << "\nVertices finales: " << teapotVertices.size();
	std::cout << "\nCoordenadas UV (vt): " << final_uvs.size() << std::endl;

	// Enviar toda la información procesada a los buffers de la GPU
	glBindVertexArray(teapotVAO);
	
	// Buffer 0: Vértices (Location = 0)
	glBindBuffer(GL_ARRAY_BUFFER, teapotbuffers[0]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * teapotVertices.size(), &teapotVertices[0], GL_STATIC_DRAW);
	glEnableVertexAttribArray(0); 
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), 0);
	
	// Buffer 1: Normales (Location = 1)
	glBindBuffer(GL_ARRAY_BUFFER, teapotbuffers[1]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * teapotNormals.size(), &teapotNormals[0], GL_STATIC_DRAW);
	glEnableVertexAttribArray(1); 
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), 0);
	
	// --- ADICIÓN: Buffer 3 de TeapotBuffers para Coordenadas UV (Location = 2) ---
	glBindBuffer(GL_ARRAY_BUFFER, buffers[numobjects * numperobj + ncolors]); 
	glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec2) * final_uvs.size(), &final_uvs[0], GL_STATIC_DRAW);
	glEnableVertexAttribArray(2); // Activamos layout (location = 2) para el Vertex Shader
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), 0);

	// Buffer de Índices (Elementos de renderizado)
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, teapotbuffers[2]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * teapotIndices.size(), &teapotIndices[0], GL_STATIC_DRAW);
	
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

inline void drawteapot() {
	glBindVertexArray(teapotVAO);
	glDrawElements(GL_TRIANGLES, teapotIndices.size(), GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);
}

#endif