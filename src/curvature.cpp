#include <Eigen/Core>
#include <Eigen/Eigenvalues>
#include <iostream>
#include <math.h>
#include "curvature.h"
using namespace OpenMesh;
using namespace Eigen;
using namespace std;

void computeCurvature(Mesh &mesh, OpenMesh::VPropHandleT<CurvatureInfo> &curvature) {
    for (Mesh::VertexIter v_it = mesh.vertices_begin(); v_it != mesh.vertices_end(); ++v_it) {
        // Per-vertex normal
		Vec3f normal = mesh.normal(v_it.handle());
		Vector3d Nvi(normal[0],normal[1],normal[2]);
        // Vertex position
        Vec3f point_i = mesh.point(v_it.handle());
        Vector3d vi(point_i[0],point_i[1],point_i[2]);
        
        // Estimate the matrix Mvi
        Matrix3d normalProjection = Matrix3d::Identity()-Nvi*Nvi.transpose();
        Matrix3d Mvi = Matrix3d::Zero();
        double sumAreas = 0.0;
        
        for (Mesh::VertexOHalfedgeIter voh_it = mesh.voh_iter(v_it.handle()); voh_it; ++voh_it) {
            // Neighbor vertex position
            Vec3f point_j = mesh.point(mesh.to_vertex_handle(voh_it.handle()));
            Vector3d vj(point_j[0],point_j[1],point_j[2]);
            Vector3d vji = vj-vi;
            
            // Compute Tij
            Vector3d Tij = (Matrix3d::Identity()-Nvi*Nvi.transpose())*vji;
            Tij.normalize();
            // Compute kij
            double kij = 2*vji.dot(Nvi) / vji.dot(vji);
            // Weight wij
            double wij = mesh.calc_sector_area(voh_it.handle()) + mesh.calc_sector_area(mesh.opposite_halfedge_handle(voh_it.handle()));
            sumAreas += wij;
            
            // Update Mvi
            Mvi += wij*kij*Tij*Tij.transpose();
        }
        Mvi /= sumAreas;
        
        // Determine curvatures and principal directions from Mvi
        EigenSolver<Matrix3d> solver(Mvi);
        
        Vector3d T1 = solver.pseudoEigenvectors().block(0,0,3,1);
        double eig1 = real(solver.eigenvalues()(0));
        Vector3d T2 = solver.pseudoEigenvectors().block(0,1,3,1);
        double eig2 = real(solver.eigenvalues()(1));
        
        if (T1.cross(Nvi).norm() < 1e-5) {
            T1 = solver.pseudoEigenvectors().block(0,2,3,1);
            eig1 = real(solver.eigenvalues()(2));
        } else if (T2.cross(Nvi).norm() < 1e-5) {
            T2 = solver.pseudoEigenvectors().block(0,2,3,1);
            eig2 = real(solver.eigenvalues()(2));
        }
        
        double m11 = T1.transpose()*Mvi*T1;
        double m22 = T2.transpose()*Mvi*T2;
        
        CurvatureInfo info;
        info.curvatures[0] = 3*m11-m22;
        info.curvatures[1] = 3*m22-m11;
        info.directions[0] = Vec3f(T1(0),T1(1),T1(2));
        info.directions[1] = Vec3f(T2(0),T2(1),T2(2));
        
        if (fabs(info.curvatures[0]) > fabs(info.curvatures[1])) {
            double temp = info.curvatures[0];
            info.curvatures[0] = info.curvatures[1];
            info.curvatures[1] = temp;
            
            Vec3f temp2 = info.directions[0];
            info.directions[0] = info.directions[1];
            info.directions[1] = temp2;
        }
        
		mesh.property(curvature,v_it) = info;
	}
}

void computeViewCurvature(Mesh &mesh, OpenMesh::Vec3f camPos, OpenMesh::VPropHandleT<CurvatureInfo> &curvature, OpenMesh::VPropHandleT<double> &viewCurvature, OpenMesh::FPropHandleT<OpenMesh::Vec3f> &viewCurvatureDerivative, OpenMesh::VPropHandleT<OpenMesh::Vec3f> &viewVecProjection) {
    Mesh::VertexIter v_it, v_end = mesh.vertices_end();
    for (v_it = mesh.vertices_begin(); v_it != v_end; ++v_it) {
        // Compute view vector
        Vec3f p = mesh.point(v_it.handle());
        Vec3f v = camPos - p;
        
        // Project view vector onto tangent plane
        CurvatureInfo info = mesh.property(curvature,v_it);
        Vec3f T1 = info.directions[0];
        Vec3f T2 = info.directions[1];
        Vec3f w = dot(v,T1)*T1 + dot(v,T2)*T2;
        w.normalize();
        
        // store w vector for rendering
        mesh.property(viewVecProjection,v_it) = w;
        
        // Use components in principal directions to compute view curvature
        double k1 = info.curvatures[0];
        double k2 = info.curvatures[1];
        double phi = acos(dot(w,T1));
        double cosPhi = cos(phi);
        double sinPhi = sin(phi);
        double kw = k1*cosPhi*cosPhi + k2*sinPhi*sinPhi;
        
        mesh.property(viewCurvature,v_it) = k1*cosPhi*cosPhi + k2*sinPhi*sinPhi;
    }

	// We'll use the finite elements piecewise hat method to find per-face gradients of the view curvature
	// CS 348a doesn't cover how to differentiate functions on a mesh (Take CS 468! Spring 2013!) so we provide code here
	
	for (Mesh::FaceIter it = mesh.faces_begin(); it != mesh.faces_end(); ++it) {
		double c[3];
		Vec3f p[3];
		
		Mesh::ConstFaceVertexIter fvIt = mesh.cfv_iter(it);
		for (int i = 0; i < 3; i++) {
			p[i] = mesh.point(fvIt.handle());
			c[i] = mesh.property(viewCurvature,fvIt.handle());
			++fvIt;
		}
		
		Vec3f N = mesh.normal(it.handle());
		double area = mesh.calc_sector_area(mesh.halfedge_handle(it.handle()));

		mesh.property(viewCurvatureDerivative,it) = (N%(p[0]-p[2]))*(c[1]-c[0])/(2*area) + (N%(p[1]-p[0]))*(c[2]-c[0])/(2*area);
	}
}
