#include "mesh_features.h"
using namespace OpenMesh;

bool isSilhouette(Mesh &mesh, const Mesh::EdgeHandle &e, Vec3f cameraPos)  {
    // Gather the parts
    Mesh::HalfedgeHandle hh = mesh.halfedge_handle(e,0);
    // vertices
    Mesh::VertexHandle v0 = mesh.from_vertex_handle(hh);
    Mesh::VertexHandle v1 = mesh.to_vertex_handle(hh);
    Vec3f p0 = mesh.point(v0);
    Vec3f p1 = mesh.point(v1);
    // faces
    Mesh::FaceHandle fr = mesh.face_handle(hh);
    Mesh::FaceHandle fl = mesh.face_handle(mesh.opposite_halfedge_handle(hh));
    // View and normal vectors
    Vec3f p = (p0+p1)/2;
    Vec3f v = cameraPos - p;
    Vec3f nr = mesh.normal(fr);
    Vec3f nl = mesh.normal(fl);
    
    // Determine if dot products have opposite sign
    return (dot(nr, v) * dot(nl, v) < 0.0f);
}

bool isSharpEdge(Mesh &mesh, const Mesh::EdgeHandle &e) {
	// Gather the parts
    Mesh::HalfedgeHandle hh = mesh.halfedge_handle(e,0);
    // faces
    Mesh::FaceHandle fr = mesh.face_handle(hh);
    Mesh::FaceHandle fl = mesh.face_handle(mesh.opposite_halfedge_handle(hh));
    // Normal vectors
    Vec3f nr(mesh.normal(fr));
    Vec3f nl(mesh.normal(fl));

    return (dot(nr, nl) < 0.5f);
}

bool isFeatureEdge(Mesh &mesh, const Mesh::EdgeHandle &e, Vec3f cameraPos) {
	return mesh.is_boundary(e) || isSilhouette(mesh,e, cameraPos) || isSharpEdge(mesh,e);
}

