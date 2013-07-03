#include "decimate.h"
#include <iostream>
#include <set>
#include <float.h>
#include <math.h>
using namespace OpenMesh;

VPropHandleT<Quadricd> vquadric;
VPropHandleT<float> vprio;
VPropHandleT<Mesh::HalfedgeHandle> vtarget;


void initDecimation(Mesh & mesh);
bool is_collapse_legal(Mesh &mesh, Mesh::HalfedgeHandle _hh);
float priority(Mesh &mesh, Mesh::HalfedgeHandle _heh);
void decimate(Mesh &mesh, unsigned int _n_vertices);
void enqueue_vertex(Mesh &mesh, Mesh::VertexHandle vh); 


// access quadric of vertex _vh
Quadricd& quadric(Mesh& mesh, Mesh::VertexHandle _vh) {
	return mesh.property(vquadric, _vh);
}

// access priority of vertex _vh
float& priority(Mesh& mesh, Mesh::VertexHandle _vh) {
	return mesh.property(vprio, _vh);
}

// access target halfedge of vertex _vh
Mesh::HalfedgeHandle& target(Mesh& mesh, Mesh::VertexHandle _vh) {
	return mesh.property(vtarget, _vh);
}


// NOTE:  We're making a global pointer to the mesh object here for notational convenience.
//    This will NOT work if you make your code multithreaded and is horrible programming practice.
//    But, it works and we're not asking you to implement this part.

Mesh *meshPtr;

// compare functor for priority queue
struct VertexCmp {
	bool operator()(Mesh::VertexHandle _v0, Mesh::VertexHandle _v1) const {
		Mesh &mesh = *meshPtr;
		// std::set needs UNIQUE keys -> handle equal priorities
		return ((priority(mesh,_v0) == priority(mesh,_v1)) ?
				(_v0.idx() < _v1.idx()) : (priority(mesh,_v0) < priority(mesh,_v1)));
	}
};

std::set<Mesh::VertexHandle, VertexCmp> queue;

void simplify(Mesh &mesh, float percentage) {
	meshPtr = &mesh; // NEVER EVER DO THIS IN REAL LIFE

	// add required properties
	mesh.request_vertex_status();
	mesh.request_edge_status();
	mesh.request_face_status();
	mesh.request_face_normals();
	mesh.add_property(vquadric);
	mesh.add_property(vprio);
	mesh.add_property(vtarget);

	// compute normals & quadrics
	initDecimation(mesh);

	// decimate
	decimate(mesh, (int) (percentage * mesh.n_vertices()));
	std::cout << "Simplifying to #vertices: " << (int) (mesh.n_vertices()) << std::endl;
    
    // write mesh to debug
    IO::write_mesh(mesh, "models/test.off");
}

void initDecimation(Mesh &mesh) {
	// compute face normals
	mesh.update_face_normals();

	Mesh::VertexIter v_it, v_end = mesh.vertices_end();
	Mesh::Point n;
	Mesh::VertexFaceIter vf_it;          // To iterate through incident faces
	double a, b, c, d, length, one_over_length;
	Mesh::Scalar sum;
    
	for (v_it = mesh.vertices_begin(); v_it != v_end; ++v_it) {
		priority(mesh, v_it) = -1.0;
		quadric(mesh, v_it).clear();
		sum = 0;                            // Reset for each iteration

        // Calculate vertex quadrics from adjacent faces
        Mesh::Point v = mesh.point(v_it.handle());
        for (vf_it = mesh.vf_iter(v_it.handle()); vf_it; ++vf_it) {
            n = mesh.normal(vf_it.handle());
            // Determine plane equation
            // ax + by + cz + d = 0
            // n[0](x-v[0])+n[1](y-v[1])+n[2](z-v[2])=0
            a = n[0]; b = n[1]; c = n[2];
            d = -dot(n,v);
            // Normalize plane vector <a,b,c,d>
            length = sqrt(a*a + b*b + c*c + d*d);
            one_over_length = 1.0f/length;
            a *= one_over_length; b *= one_over_length;
            c *= one_over_length; d *= one_over_length;
            // Construct quadric matrix for ith face and sum
            Quadricd qi(a,b,c,d);
            quadric(mesh, v_it) += qi;
        }
	}
    std::cout << "Finished init" << std::endl;
}

bool is_collapse_legal(Mesh &mesh, Mesh::HalfedgeHandle _hh)
{
    // collect vertices
    Mesh::VertexHandle v0, v1;
    v0 = mesh.from_vertex_handle(_hh);
    v1 = mesh.to_vertex_handle(_hh);


    // collect faces
    Mesh::FaceHandle fl = mesh.face_handle(_hh);
    Mesh::FaceHandle fr = mesh.face_handle(mesh.opposite_halfedge_handle(_hh));


    // backup point positions
    Mesh::Point p0 = mesh.point(v0);
    Mesh::Point p1 = mesh.point(v1);


    // topological test
    if (!mesh.is_collapse_ok(_hh))
        return false;

    // test boundary stuff
    if (mesh.is_boundary(v0) && !mesh.is_boundary(v1))
        return false;

    for (Mesh::VertexFaceIter vfIt = mesh.vf_iter(v0); vfIt; ++vfIt) {
        if (vfIt.handle() == fl || vfIt.handle() == fr) continue;

        Mesh::Point p[3];

        Mesh::ConstFaceVertexIter cfvIt = mesh.cfv_iter(vfIt.handle());
        p[0] = mesh.point(cfvIt.handle());
        p[1] = mesh.point((++cfvIt).handle());
        p[2] = mesh.point((++cfvIt).handle());

        Mesh::Point q[3];

        for (int i = 0; i < 3; i++)
            q[i] = (p[i] == p0)? p1 : p[i];

        Mesh::Point n1 = (p[1]-p[0])%(p[2]-p[0]);
        Mesh::Point n2 = (q[1]-q[0])%(q[2]-q[0]);

        if ((n1|n2) < n1.length()*n2.length()/sqrt(2.)) return false;
    }

    return true;
}


float priority(Mesh &mesh, Mesh::HalfedgeHandle _heh) {
    // return priority: the smaller the better
	// use quadrics to estimate approximation error
	Mesh::VertexHandle v0, v1;
    v0 = mesh.from_vertex_handle(_heh);
    v1 = mesh.to_vertex_handle(_heh);
    
    // Quadrics from halfedge vertices
    Quadricd q0 = quadric(mesh, v0);
    Quadricd q1 = quadric(mesh, v1);
    Vec3f p0 = mesh.point(v0);
    Vec3f p1 = mesh.point(v1);
    
    // Quadrics of each vertex evaluated at other vertex
    return q0(p1)+q1(p1);
}

void enqueue_vertex(Mesh &mesh, Mesh::VertexHandle _vh) {
	float prio, min_prio(FLT_MAX);
	Mesh::HalfedgeHandle min_hh;

	// find best out-going halfedge
	for (Mesh::VOHIter vh_it(mesh, _vh); vh_it; ++vh_it) {
		if (is_collapse_legal(mesh,vh_it)) {
			prio = priority(mesh, vh_it);
			if (prio != -1.0 && prio < min_prio) {
				min_prio = prio;
				min_hh = vh_it.handle();
			}
		}
	}

	// update queue
	if (priority(mesh, _vh) != -1.0) {
		queue.erase(_vh);
		priority(mesh, _vh) = -1.0;
	}

	if (min_hh.is_valid()) {
		priority(mesh, _vh) = min_prio;
		target(mesh, _vh) = min_hh;
		queue.insert(_vh);
	}
}

void decimate(Mesh &mesh, unsigned int _n_vertices) {
	unsigned int nv(mesh.n_vertices());
    std::cout << "Got to decimate" << std::endl;

	Mesh::HalfedgeHandle hh;
	Mesh::VertexHandle to, from;
	Mesh::VVIter vv_it;

	std::vector<Mesh::VertexHandle> one_ring;
	std::vector<Mesh::VertexHandle>::iterator or_it, or_end;

	// build priority queue
	Mesh::VertexIter v_it = mesh.vertices_begin(), v_end =
			mesh.vertices_end();

	queue.clear();
	for (; v_it != v_end; ++v_it)
		enqueue_vertex(mesh, v_it.handle());

    // Decimate using priority queue
    while ((nv > _n_vertices) && !queue.empty()) {
        // take 1st element of queue
        from = *(queue.begin());
        hh = target(mesh, from);
        to = mesh.to_vertex_handle(hh);
        queue.erase(from);
        // collapse halfedge
        if (!is_collapse_legal(mesh, hh))
            continue;
        mesh.collapse(hh);
        quadric(mesh, to) += quadric(mesh, from);
        // update queue
        enqueue_vertex(mesh, to);
        for (vv_it = mesh.vv_iter(to); vv_it; ++vv_it) {
            enqueue_vertex(mesh, vv_it.handle());
        }
        nv--;
    }

	// clean up after decimation
	queue.clear();

	// now, delete the items marked to be deleted
	mesh.garbage_collection();
    std::cout << "Out of decimate" << std::endl;
}

