#include <OpenMesh/Core/IO/Options.hh>
#include <OpenMesh/Core/IO/MeshIO.hh>
#include <iostream>
#include <cmath>
#include <cstdio>
#include <stdlib.h>
#include <GLUT/glut.h>
#include <Eigen/Core>
#include <Eigen/Eigenvalues>
#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>
#include "curvature.h"
#include "mesh_features.h"
#include "image_generation.h"
#include "decimate.h"
#include "shader.h"
using namespace std;
using namespace OpenMesh;
using namespace Eigen;

//#define HATCH_TEST

#ifndef M_PI
#define M_PI 3.14159265359
#endif

// Suggestive Contours thresholds
double angleThresh = M_PI/4;
double gradThresh = 1000.0;

// Mesh properties
VPropHandleT<double> viewCurvature;
FPropHandleT<Vec3f> viewCurvatureDerivative;
VPropHandleT<Vec3f> viewVecProjection;
VPropHandleT<CurvatureInfo> curvature;
VPropHandleT<Vec2f> textureCoord;

Mesh mesh;
vector<unsigned int> indices;
vector<Vec2f> texCoords;
void setTextureCoords();

bool leftDown = false, rightDown = false, middleDown = false;
int lastPos[2];
float cameraPos[4] = {0,0,4,1};
Vec3f up, pan;
int windowWidth = 640, windowHeight = 480;
bool showSurface = true, showAxes = false, showCurvature = false, showContours = true, showNormals = false;
string displayType = "smooth";

// Light source attributes
float specularLight[] = { 1.0, 1.0, 1.0, 1.0 };
float ambientLight[]  = { 0.1, 0.1, 0.1, 1.0 };
float diffuseLight[]  = { 1.0, 1.0, 1.0, 1.0 };

// Material color properties
float materialAmbient[]  = { 0.5, 0.5, 0.5, 1.0 };
float materialDiffuse[]  = { 0.6, 0.6, 0.6, 1.0 };
float materialSpecular[] = { 0.8, 0.8, 0.8, 1.0 };
float shininess[]          = { 50.0 };

// Shaders
GLuint hatchProg;
GLuint hatchVert;
GLuint hatchFrag;
Shader* shader;

// Texture images
GLuint tamX3[2];    // stores highest detail tams   (256x256)
GLuint tamX2[2];    // stores high detail tams      (128x128)
GLuint tamX1[2];    // stores low detail tams       (64x64)
GLuint tamX0[2];    // stores lowest detail tams    (32x32)

void renderSuggestiveContours(Vec3f actualCamPos) { // use this camera position to account for panning etc.
	glColor3f(0.3,0.3,0.3);
    glBegin(GL_LINES);
    
    Mesh::FaceIter f_it, f_end = mesh.faces_end();
    Mesh::FaceVertexIter fv_it;
    
    for (f_it = mesh.faces_begin(); f_it != f_end; ++f_it) {
        // Face data
        Vec3f n = mesh.normal(f_it.handle());
        Vec3f Dw = mesh.property(viewCurvatureDerivative,f_it);
        
        // Per-vertex data
        fv_it = mesh.fv_iter(f_it.handle());
        Vec3f p0 = mesh.point(fv_it.handle());
        double kw0 = mesh.property(viewCurvature,fv_it);
        Vec3f w0 = mesh.property(viewVecProjection,fv_it);
        
        Vec3f p1 = mesh.point((++fv_it).handle());
        double kw1 = mesh.property(viewCurvature,fv_it);
        Vec3f w1 = mesh.property(viewVecProjection,fv_it);
        
        Vec3f p2 = mesh.point((++fv_it).handle());
        double kw2 = mesh.property(viewCurvature,fv_it);
        Vec3f w2 = mesh.property(viewVecProjection,fv_it);
        
        // Centroid and view vector
        Vec3f pC = (p0 + p1 + p2)/3;
        Vec3f v = actualCamPos - pC;
        v.normalize();
        
        // Skip face if normal is close to view vector
        if (acos(dot(v,n)) < angleThresh) continue;
        
        // Skip face if Dwkw is small and positive
        Vec3f wC = (w0 + w1 + w2)/3;    // take w to be the average of vertex w's
        double dirGrad = -dot(Dw,wC);
        if (dirGrad < 0 || (dirGrad > 0 && dirGrad < gradThresh)) continue;
        
        // EXTENSION (maybe): Ignore segments that are too short or uninteresting?
        // EXTENSION (maybe): Reintroduce segments that were discarded, but next to a non-discarded segment
        
        // Determine if face has zero crossings
        if ((kw0 > 0 && kw1 > 0 && kw2 > 0) || (kw0 < 0 && kw1 < 0 && kw2 < 0)) continue;
        // Zero crossings along edges 0->2 and 1->2
        if ((kw0 < 0 && kw1 < 0) || (kw0 > 0 && kw1 > 0)) {
            // lerp
            Vec3f v02 = p2*((0-kw0)/(kw2-kw0)) + p0*((kw2-0)/(kw2-kw0));
            Vec3f v12 = p2*((0-kw1)/(kw2-kw1)) + p1*((kw2-0)/(kw2-kw1));
            glVertex3f(v02[0],v02[1],v02[2]);
            glVertex3f(v12[0],v12[1],v12[2]);
        // Zero crossings along edges 0->1 and 1->2
        } else if ((kw0 < 0 && kw2 < 0) || (kw0 > 0 && kw2 > 0)) {
            // lerp
            Vec3f v01 = p1*((0-kw0)/(kw1-kw0)) + p0*((kw1-0)/(kw1-kw0));
            Vec3f v12 = p2*((0-kw1)/(kw2-kw1)) + p1*((kw2-0)/(kw2-kw1));
            glVertex3f(v01[0],v01[1],v01[2]);
            glVertex3f(v12[0],v12[1],v12[2]);
        // Zero crossings along edges 0->1 and 0->2
        } else if ((kw1 < 0 && kw2 < 0) || (kw1 > 0 && kw2 > 0)) {
            // lerp
            Vec3f v01 = p1*((0-kw0)/(kw1-kw0)) + p0*((kw1-0)/(kw1-kw0));
            Vec3f v02 = p2*((0-kw0)/(kw2-kw0)) + p0*((kw2-0)/(kw2-kw0));
            glVertex3f(v01[0],v01[1],v01[2]);
            glVertex3f(v02[0],v02[1],v02[2]);
        }
    }
    glEnd();
}

void renderMesh() {
	if (!showSurface) glColorMask(GL_FALSE,GL_FALSE,GL_FALSE,GL_FALSE); // render regardless to remove hidden lines
	
	glEnable(GL_LIGHTING);
	glLightfv(GL_LIGHT0, GL_POSITION, cameraPos);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuseLight);
    glLightfv(GL_LIGHT0, GL_SPECULAR, specularLight);
    glLightfv(GL_LIGHT0, GL_AMBIENT, ambientLight);

	glDepthRange(0.001,1);
	glEnable(GL_NORMALIZE);
	
	// set up indices for array data
    Mesh::ConstFaceIter f_end(mesh.faces_end());
    Mesh::ConstFaceIter f_begin(mesh.faces_begin());
    
    indices.clear();
    indices.reserve(mesh.n_faces()*3);
    
    for (Mesh::ConstFaceIter f_it = f_begin; f_it != f_end; ++f_it) {
        Mesh::ConstFaceVertexIter fv_it = mesh.cfv_iter(f_it.handle());
        indices.push_back(fv_it.handle().idx());
        indices.push_back((++fv_it).handle().idx());
        indices.push_back((++fv_it).handle().idx());
    }
    
    // draw the mesh
#ifndef HATCH_TEST
    if (displayType == "smooth") {
        glEnable(GL_LIGHTING);
        glShadeModel(GL_SMOOTH);
        
        glEnableClientState(GL_VERTEX_ARRAY);
        glEnableClientState(GL_NORMAL_ARRAY);
        glVertexPointer(3, GL_FLOAT, 0, mesh.points());
        glNormalPointer(GL_FLOAT, 0, mesh.vertex_normals());
        
        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, &indices[0]);
        
        glDisableClientState(GL_VERTEX_ARRAY);
        glDisableClientState(GL_NORMAL_ARRAY);
    } else if (displayType == "wireframe") {
        glDisable(GL_LIGHTING);
        glColor3f(0.3, 0.3, 0.3);
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        
        glEnableClientState(GL_VERTEX_ARRAY);
        glVertexPointer(3, GL_FLOAT, 0, mesh.points());
        
        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, &indices[0]);
        
        glDisableClientState(GL_VERTEX_ARRAY);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    } else {
        glEnable(GL_LIGHTING);
        glShadeModel(GL_FLAT);
        
        glEnableClientState(GL_VERTEX_ARRAY);
        glEnableClientState(GL_NORMAL_ARRAY);
        glVertexPointer(3, GL_FLOAT, 0, mesh.points());
        glNormalPointer(GL_FLOAT, 0, mesh.vertex_normals());
        
        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, &indices[0]);
        
        glDisableClientState(GL_VERTEX_ARRAY);
        glDisableClientState(GL_NORMAL_ARRAY);
        if (displayType == "flatwire") {
            glDisable(GL_LIGHTING);
            glColor3f(0.3, 0.3, 0.3);
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            
            glEnableClientState(GL_VERTEX_ARRAY);
            glVertexPointer(3, GL_FLOAT, 0, mesh.points());
            
            glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, &indices[0]);
            
            glDisableClientState(GL_VERTEX_ARRAY);
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }
    }
#endif
	
    ////////////////////////////////
    // Fixed-function pipeline stuff
    ////////////////////////////////
	if (!showSurface) glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);
	
	glDisable(GL_LIGHTING);
	glDepthRange(0,0.999);
	
	Vec3f actualCamPos(cameraPos[0]+pan[0],cameraPos[1]+pan[1],cameraPos[2]+pan[2]);

    if (showContours)
        renderSuggestiveContours(actualCamPos);
    
	// We'll be nice and provide you with code to render feature edges below
	glBegin(GL_LINES);
	glColor3f(0,0,0);
	glLineWidth(2.0f);
	for (Mesh::ConstEdgeIter it = mesh.edges_begin(); it != mesh.edges_end(); ++it)
		if (isFeatureEdge(mesh,*it,actualCamPos)) {
			Mesh::HalfedgeHandle h0 = mesh.halfedge_handle(it,0);
			Mesh::HalfedgeHandle h1 = mesh.halfedge_handle(it,1);
			Vec3f source(mesh.point(mesh.from_vertex_handle(h0)));
			Vec3f target(mesh.point(mesh.from_vertex_handle(h1)));
			glVertex3f(source[0],source[1],source[2]);
			glVertex3f(target[0],target[1],target[2]);
		}
	glEnd();
	
	if (showCurvature) {
		glBegin(GL_LINES);
        for (Mesh::ConstVertexIter v_it = mesh.vertices_begin(); v_it != mesh.vertices_end(); ++v_it) {
            CurvatureInfo info = mesh.property(curvature,v_it);
            double k1 = info.curvatures[0];
            double k2 = info.curvatures[1];
            Vec3f T1 = info.directions[0]; // min curv dir
            Vec3f T2 = info.directions[1]; // max curv dir
            
            Vec3f p = mesh.point(v_it.handle());
            
            // draw min curvature direction
            Vec3f d1 = p + T1*.01;
            Vec3f d2 = p - T1*.01;
            glColor3f(0.0,0.0,1.0);
            glVertex3f(d1[0],d1[1],d1[2]);
            glVertex3f(d2[0],d2[1],d2[2]);
            
            // draw max curvature direction
            d1 = p + T2*.01;
            d2 = p - T2*.01;
            glColor3f(1.0,0.0,0.0);
            glVertex3f(d1[0],d1[1],d1[2]);
            glVertex3f(d2[0],d2[1],d2[2]);
        }
        glEnd();
	}
	
	if (showNormals) {
		glBegin(GL_LINES);
		for (Mesh::ConstVertexIter it = mesh.vertices_begin(); it != mesh.vertices_end(); ++it) {
			Vec3f n = mesh.normal(it.handle());
			Vec3f p = mesh.point(it.handle());
			Vec3f d = p + n*.01;
            glColor3f(n[0],n[1],n[2]);
			glVertex3f(p[0],p[1],p[2]);
			glVertex3f(d[0],d[1],d[2]);
		}
		glEnd();
	}
	
	glDepthRange(0,1);
    
    ///////////////
    // Shader stuff
    ///////////////
#ifdef HATCH_TEST
    glUseProgram(hatchProg);
    
    setTextureCoords();

    // Vertices
    GLint position = glGetAttribLocation(hatchProg, "positionIn");
    glEnableVertexAttribArray(position);
    glVertexAttribPointer(position, 3, GL_FLOAT, 0, 0, mesh.points());
	
    // Normals
    GLint normal = glGetAttribLocation(hatchProg, "normalIn");
    glEnableVertexAttribArray(normal);
    glVertexAttribPointer(normal, 3, GL_FLOAT, 0, 0, mesh.vertex_normals());
    
    // Texture coords
    GLint texcoord = glGetAttribLocation(hatchProg, "texcoordIn");
    glEnableVertexAttribArray(texcoord);
    glVertexAttribPointer(texcoord, 2, GL_FLOAT, 0, 0, &texCoords[0]);
    
    // Set uniform vars for shader
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tamX3[0]);
    GLint handle = glGetUniformLocation(hatchProg, "hatch012");
    glUniform1i(handle, 0);
    
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, tamX3[1]);
    handle = glGetUniformLocation(hatchProg, "hatch345");
    glUniform1i(handle, 1);
    
    glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, &indices[0]);
#endif
}

void loadShaders() {
	shader = new Shader("shaders/hatch");
	if (!shader->loaded()) {
		string errS = shader->errors();
		const char* errC = errS.c_str();
		printf("%s", errC);
	}
    hatchProg = shader->programID();
    hatchVert = shader->vertexShaderID();
    hatchFrag = shader->fragmentShaderID();
    //delete shader;
}

void loadTextures() {
    glGenTextures(2, tamX3);
    
    sf::Image image0 = sf::Image();
    image0.loadFromFile("textures/tam03.bmp");
    sf::Vector2u size = image0.getSize();
    GLuint width = size.x;
    GLuint height = size.y;
    sf::Image image1 = sf::Image();
    image1.loadFromFile("textures/tam13.bmp");
    sf::Image image2 = sf::Image();
    image2.loadFromFile("textures/tam23.bmp");
    sf::Image texture1 = sf::Image(); // stores 3 textures in r, g, and b
    texture1.create(width,height);
    
    // set three grayscale images to R, G, and B values of texture1
    sf::Color p0, p1, p2;
    for (GLuint x = 0; x < width; x++) {
        for (GLuint y = 0; y < height; y++) {
            p0 = image0.getPixel(x,y);
            p1 = image1.getPixel(x,y);
            p2 = image2.getPixel(x,y);
            GLuint r = (p0.r + p0.g + p0.b)/3;
            GLuint g = (p1.r + p1.g + p1.b)/3;
            GLuint b = (p2.r + p2.g + p2.b)/3;
            texture1.setPixel(x,y,sf::Color(r,g,b,255));
        }
    }
    
    // tones 1, 2, and 3 (r, g, and b)
    glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, tamX3[0]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGBA, width, height, GL_RGBA, GL_UNSIGNED_BYTE, texture1.getPixelsPtr());
    //glTexImage2D(GL_TEXTURE_2D, 0, 4, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, texture1.getPixelsPtr());

    sf::Image image3 = sf::Image();
    image3.loadFromFile("textures/tam33.bmp");
    sf::Image image4 = sf::Image();
    image4.loadFromFile("textures/tam43.bmp");
    sf::Image image5 = sf::Image();
    image5.loadFromFile("textures/tam53.bmp");
    sf::Image texture2 = sf::Image(); // stores 3 textures in r, g, and b
    texture2.create(width,height);

    // set three grayscale images to R, G, and B values of texture2
    sf::Color p3, p4, p5;
    for (GLuint x = 0; x < width; x++) {
        for (GLuint y = 0; y < height; y++) {
            p3 = image3.getPixel(x,y);
            p4 = image4.getPixel(x,y);
            p5 = image5.getPixel(x,y);
            GLuint r = (p3.r + p3.g + p3.b)/3;
            GLuint g = (p4.r + p4.g + p4.b)/3;
            GLuint b = (p5.r + p5.g + p5.b)/3;
            texture2.setPixel(x,y,sf::Color(r,g,b,255));
        }
    }

    // tones 4, 5, and 6 (r, g, and b)
    glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, tamX3[1]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGBA, width, height, GL_RGBA, GL_UNSIGNED_BYTE, texture2.getPixelsPtr());
    //glTexImage2D(GL_TEXTURE_2D, 1, 4, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, texture2.getPixelsPtr());
    
    glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, tamX3[0]);
	
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, tamX3[1]);
}

void setTextureCoords() {
    Mesh::FaceIter f_it, f_end = mesh.faces_end();
    for (f_it = mesh.faces_begin(); f_it != f_end; ++f_it) {
        // face vertices
        Mesh::FaceVertexIter fv_it = mesh.fv_iter(f_it.handle());
        Mesh::VertexHandle v0, v1, v2;
        v0 = fv_it.handle();
        v1 = (++fv_it).handle();
        v2 = (++fv_it).handle();
        
        // curvature info per vertex
        CurvatureInfo info0 = mesh.property(curvature, v0);
        CurvatureInfo info1 = mesh.property(curvature, v1);
        CurvatureInfo info2 = mesh.property(curvature, v2);
        Vec3f T01 = info0.directions[0];
        Vec3f T11 = info1.directions[0];
        Vec3f T21 = info2.directions[0];
        Vec3f T02 = info0.directions[1];
        Vec3f T12 = info1.directions[1];
        Vec3f T22 = info2.directions[1];
        Vec3f T1 = (T01 + T11 + T21)/3.0;
        Vec3f T2 = (T02 + T12 + T22)/3.0;
        
        // global up vector projected onto face plane
        Vec3f n = mesh.normal(f_it.handle());
        Vec3f p0 = mesh.point(v0);
        Vec3f p1 = mesh.point(v1);
        Vec3f p2 = mesh.point(v2);
        Vec3f t1 = p1 - p0;
        t1.normalize();
        Vec3f t2 = cross(n,t1);
        t2.normalize();
        Vec3f T = dot(up,t1)*t1 + dot(up,t2)*t2;
        //Vec3f T = dot(up,T1)*T1 + dot(up,T2)*T2;
        T.normalize();
        Vec3f S = cross(n,T);
        S.normalize();
//        double a, b, c, d;
//        a = n[0]; b = n[1]; c = n[2];
//        d = -dot(n,v);
        
        // edge vectors
        Vec3f e01 = (p1-p0).normalize();
        Vec3f e02 = (p2-p0).normalize();
        Vec3f e12 = (p2-p1).normalize();
        
        // set texture coords based on principal dir's
        mesh.property(textureCoord,v0) = Vec2f(T[0],T[1]);
        mesh.property(textureCoord,v1) = Vec2f(T[0],T[1]);
        mesh.property(textureCoord,v2) = Vec2f(T[0],T[1]);
//        mesh.property(textureCoord,v0) = Vec2f(dot(e12,T),dot(e12,S));
//        mesh.property(textureCoord,v1) = Vec2f(dot(e01,T),dot(e01,S));
//        mesh.property(textureCoord,v2) = Vec2f(dot(e02,T),dot(e02,S));
    }
    // set texture coords in vertex order
    texCoords.clear();
    texCoords.reserve(mesh.n_vertices());
    Mesh::VertexIter v_it, v_end = mesh.vertices_end();
    for (v_it = mesh.vertices_begin(); v_it != v_end; ++v_it) {
        //texCoords.push_back(mesh.texcoord2D(v_it.handle()));
        texCoords.push_back(mesh.property(textureCoord,v_it));
    }
}

void display() {
	glClearColor(1,1,1,1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_LINE_SMOOTH);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_LIGHTING);
	glShadeModel(GL_SMOOTH);
    glMaterialfv(GL_FRONT, GL_AMBIENT, materialAmbient);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, materialDiffuse);
	glMaterialfv(GL_FRONT, GL_SPECULAR, materialSpecular);
	glMaterialfv(GL_FRONT, GL_SHININESS, shininess);
	glEnable(GL_LIGHT0);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glViewport(0,0,windowWidth,windowHeight);
	
	float ratio = (float)windowWidth / (float)windowHeight;
	gluPerspective(50, ratio, 0.5, 1000); // 50 degree vertical viewing angle, zNear = 1, zFar = 1000
	
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	gluLookAt(cameraPos[0]+pan[0], cameraPos[1]+pan[1], cameraPos[2]+pan[2], pan[0], pan[1], pan[2], up[0], up[1], up[2]);
	
	// Draw mesh
	renderMesh();

	// Draw axes
	if (showAxes) {
		glDisable(GL_LIGHTING);
		glBegin(GL_LINES);
		glLineWidth(1);
			glColor3f(1,0,0); glVertex3f(0,0,0); glVertex3f(1,0,0); // x axis
			glColor3f(0,1,0); glVertex3f(0,0,0); glVertex3f(0,1,0); // y axis
			glColor3f(0,0,1); glVertex3f(0,0,0); glVertex3f(0,0,1); // z axis
		glEnd(/*GL_LINES*/);
	}
	
	glutSwapBuffers();
}

void mouse(int button, int state, int x, int y) {
	if (button == GLUT_LEFT_BUTTON) leftDown = (state == GLUT_DOWN);
	else if (button == GLUT_RIGHT_BUTTON) rightDown = (state == GLUT_DOWN);
	else if (button == GLUT_MIDDLE_BUTTON) middleDown = (state == GLUT_DOWN);
	
	lastPos[0] = x;
	lastPos[1] = y;
}

void mouseMoved(int x, int y) {
	int dx = x - lastPos[0];
	int dy = y - lastPos[1];
	Vec3f curCamera(cameraPos[0],cameraPos[1],cameraPos[2]);
	Vec3f curCameraNormalized = curCamera.normalized();
	Vec3f right = up % curCameraNormalized;

	if (leftDown) {
		// Assume here that up vector is (0,1,0)
		Vec3f newPos = curCamera - 2*(float)((float)dx/(float)windowWidth) * right + 2*(float)((float)dy/(float)windowHeight) * up;
		newPos = newPos.normalized() * curCamera.length();
		
		up = up - (up | newPos) * newPos / newPos.sqrnorm();
		up.normalize();
		
		for (int i = 0; i < 3; i++) cameraPos[i] = newPos[i];
	}
	else if (rightDown) for (int i = 0; i < 3; i++) cameraPos[i] *= pow(1.1,dy*.1);
	else if (middleDown) {
		pan += -2*(float)((float)dx/(float)windowWidth) * right + 2*(float)((float)dy/(float)windowHeight) * up;
	}

	
	lastPos[0] = x;
	lastPos[1] = y;
	
	Vec3f actualCamPos(cameraPos[0]+pan[0],cameraPos[1]+pan[1],cameraPos[2]+pan[2]);
	computeViewCurvature(mesh,actualCamPos,curvature,viewCurvature,viewCurvatureDerivative, viewVecProjection);
	
	glutPostRedisplay();
}

void keyboardSpec(int key, int x, int y) {
	if (key == GLUT_KEY_LEFT) angleThresh *= 0.9;
    else if (key == GLUT_KEY_RIGHT) angleThresh *= 10./9.;
    else if (key == GLUT_KEY_DOWN) gradThresh *= 0.9;
    else if (key == GLUT_KEY_UP) gradThresh *= 10./9.;
    cout << "angleThresh: " << angleThresh << endl;
    cout << "gradThresh: " << gradThresh << endl << endl;
	glutPostRedisplay();
}

void keyboard(unsigned char key, int x, int y) {
	Vec3f actualCamPos(cameraPos[0]+pan[0],cameraPos[1]+pan[1],cameraPos[2]+pan[2]);

	if (key == 's' || key == 'S') showSurface = !showSurface;
	else if (key == 'a' || key == 'A') showAxes = !showAxes;
	else if (key == 'c' || key == 'C') showCurvature = !showCurvature;
    else if (key == 'v' || key == 'V') showContours = !showContours;
	else if (key == 'n' || key == 'N') showNormals = !showNormals;
    else if (key == 'd' || key == 'D') {
        if (displayType == "wireframe") displayType = "smooth";
        else if (displayType == "smooth") displayType = "flat";
        else if (displayType == "flat") displayType = "flatwire";
        else if (displayType == "flatwire") displayType = "wireframe";
    }
	else if (key == 'w' || key == 'W') writeImage(mesh, windowWidth, windowHeight, "renderedImage.svg", actualCamPos);
	else if (key == 'q' || key == 'Q') exit(0);
	glutPostRedisplay();
}

void reshape(int width, int height) {
	windowWidth = width;
	windowHeight = height;
	glutPostRedisplay();
}

int main(int argc, char** argv) {
	if (argc < 2) {
		cout << "Usage: " << argv[0] << " mesh_filename\n";
		exit(0);
	}
	
	IO::Options opt;
	opt += IO::Options::VertexNormal;
	opt += IO::Options::FaceNormal;
	
	mesh.request_face_normals();
	mesh.request_vertex_normals();
    mesh.request_vertex_texcoords2D();
	
	cout << "Reading from file " << argv[1] << "...\n";
	if ( !IO::read_mesh(mesh, argv[1], opt )) {
		cout << "Read failed.\n";
		exit(0);
	}

	cout << "Mesh stats:\n";
	cout << '\t' << mesh.n_vertices() << " vertices.\n";
	cout << '\t' << mesh.n_edges() << " edges.\n";
	cout << '\t' << mesh.n_faces() << " faces.\n";
	
	simplify(mesh,0.1f);
	
	mesh.update_normals();
	
	mesh.add_property(viewCurvature);
	mesh.add_property(viewCurvatureDerivative);
    mesh.add_property(viewVecProjection);
	mesh.add_property(curvature);
    mesh.add_property(textureCoord);
	
	// Move center of mass to origin
	Vec3f center(0,0,0);
	for (Mesh::ConstVertexIter vIt = mesh.vertices_begin(); vIt != mesh.vertices_end(); ++vIt) center += mesh.point(vIt);
	center /= mesh.n_vertices();
	for (Mesh::VertexIter vIt = mesh.vertices_begin(); vIt != mesh.vertices_end(); ++vIt) mesh.point(vIt) -= center;

	// Fit in the unit sphere
	float maxLength = 0;
	for (Mesh::ConstVertexIter vIt = mesh.vertices_begin(); vIt != mesh.vertices_end(); ++vIt) maxLength = max(maxLength, mesh.point(vIt).length());
	for (Mesh::VertexIter vIt = mesh.vertices_begin(); vIt != mesh.vertices_end(); ++vIt) mesh.point(vIt) /= maxLength;
	
	computeCurvature(mesh,curvature);

	up = Vec3f(0,1,0);
	pan = Vec3f(0,0,0);
	
	Vec3f actualCamPos(cameraPos[0]+pan[0],cameraPos[1]+pan[1],cameraPos[2]+pan[2]);
	computeViewCurvature(mesh,actualCamPos,curvature,viewCurvature,viewCurvatureDerivative,viewVecProjection);

	glutInit(&argc, argv); 
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH); 
	glutInitWindowSize(windowWidth, windowHeight); 
	glutCreateWindow(argv[0]);
    
#ifdef HATCH_TEST
    // Load vertex/frag shaders and textures
    loadShaders();
    loadTextures();
    setTextureCoords();
#endif

	glutDisplayFunc(display);
	glutMotionFunc(mouseMoved);
	glutMouseFunc(mouse);
	glutReshapeFunc(reshape);
	glutKeyboardFunc(keyboard);
    glutSpecialFunc(keyboardSpec);

	glutMainLoop();
	
	return 0;
}
