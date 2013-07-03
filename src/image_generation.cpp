#include "image_generation.h"
#include "mesh_features.h"
#include <GLUT/glut.h>
#include <fstream>
#include <set>
#include <map>
#include <list>
using namespace OpenMesh;
using namespace std;

Vec3f toImagePlane(Vec3f point) {
	GLdouble point3DX = point[0], point3DY = point[1], point3DZ = point[2];

	GLdouble modelMatrix[16], projMatrix[16];
	GLint viewport[4];
	glGetDoublev(GL_MODELVIEW_MATRIX, modelMatrix);
	glGetDoublev(GL_PROJECTION_MATRIX, projMatrix);
	glGetIntegerv(GL_VIEWPORT, viewport);
	
	GLdouble point2DX, point2DY, point2DZ;
	gluProject(point3DX, point3DY, point3DZ, modelMatrix, projMatrix, viewport, &point2DX, &point2DY, &point2DZ);
	
	return Vec3f(point2DX,point2DY,point2DZ);
}

// Adapted from
// http://stackoverflow.com/questions/1311869/opengl-how-to-determine-if-a-3d-rendered-point-is-occluded-by-other-3d-rende
bool isVisible(Vec3f point) {
	Vec3f projected = toImagePlane(point);

	GLfloat bufDepth = 0.0;
	glReadPixels(static_cast<GLint>( projected[0] ), static_cast<GLint>( projected[1] ),
		     1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &bufDepth);

	GLdouble EPSILON = 0.01;
	return (bufDepth - projected[2]) > -EPSILON; // check sign!
}

void writeImage(Mesh &mesh, int width, int height, string filename, Vec3f camPos) {
	ofstream outfile(filename.c_str());
	outfile << "<?xml version=\"1.0\" standalone=\"no\"?>\n";
	outfile << "<svg width=\"5in\" height=\"5in\" viewBox=\"0 0 " << width << ' ' << height << "\">\n";
	outfile << "<g stroke=\"black\" fill=\"black\">\n";
	
	// Sample code for generating image of the entire triangle mesh:
	/*for (Mesh::ConstEdgeIter it = mesh.edges_begin(); it != mesh.edges_end(); ++it) {
		Mesh::HalfedgeHandle h0 = mesh.halfedge_handle(it,0);
		Mesh::HalfedgeHandle h1 = mesh.halfedge_handle(it,1);
		Vec3f source(mesh.point(mesh.from_vertex_handle(h0)));
		Vec3f target(mesh.point(mesh.from_vertex_handle(h1)));
		
		if (!isVisible(source) || !isVisible(target)) continue;
		
		Vec3f p1 = toImagePlane(source);
		Vec3f p2 = toImagePlane(target);
		outfile << "<line ";
		outfile << "x1=\"" << p1[0] << "\" ";
		outfile << "y1=\"" << height-p1[1] << "\" ";
		outfile << "x2=\"" << p2[0] << "\" ";
		outfile << "y2=\"" << height-p2[1] << "\" stroke-width=\"1\" />\n";
	}*/
	
	// WRITE CODE HERE TO GENERATE A .SVG OF THE MESH --------------------------------------------------------------

	// -------------------------------------------------------------------------------------------------------------
	
	outfile << "</g>\n";
	outfile << "</svg>\n";

}
