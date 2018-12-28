#include <float.h>
#include <assert.h>
#include "meshEdit.h"
#include "mutablePriorityQueue.h"
#include "error_dialog.h"

//#include <set>
#include <deque>

using namespace CMU462;

bool HalfedgeMesh::IsValid(VertexIter v, const string & info) {
	// Check iterator, if it is not valid, show fatal error

	if (v == vertices.end()) {
		showError(info, true);
		return false;
	}

	return true;
}

bool HalfedgeMesh::IsValid(HalfedgeIter he, const string & info) {
	// Check iterator, if it is not valid, show fatal error

	if (he == halfedges.end()) {
		showError(info, true);
		return false;
	}

	return true;
}

bool HalfedgeMesh::IsValid(EdgeIter e, const string & info) {
	// Check iterator, if it is not valid, show fatal error

	if (e == edges.end()) {
		showError(info, true);
		return false;
	}

	return true;
}

bool HalfedgeMesh::IsValid(FaceIter f, const string & info) {
	// Check iterator, if it is not valid, show fatal error

	if (f == faces.end()) {
		showError(info, true);
		return false;
	}

	return true;
}

EdgeIter HalfedgeMesh::GetSameEdge(VertexIter v0, VertexIter v1) {
	// if v0 and v1 are on a same edge, return the edge, otherwise return edges.end()

	if (!IsValid(v0, "GetSameEdge : v0 is null") 
		|| !IsValid(v1, "GetSameEdge : v1 is null"))
		return edges.end();

	vector<EdgeIter> adjEdgesOfV0 = v0->AdjEdges();
	vector<EdgeIter> adjEdgesOfV1 = v1->AdjEdges();

	for (auto adjE : adjEdgesOfV0) {
		auto target = find(adjEdgesOfV1.begin(), adjEdgesOfV1.end(), adjE);
		if (target != adjEdgesOfV1.end())
			return *target;
	}

	return edges.end();
}

FaceIter HalfedgeMesh::GetSameFace(VertexIter v0, VertexIter v1) {
	// if v0 and v1 are on a same inner face, return the face, otherwise return faces.end()
	// the inner face is not a boundary

	if (!IsValid(v0, "GetSameFace : v0 is null")
		|| !IsValid(v1, "GetSameFace : v1 is null"))
		return faces.end();

	vector<FaceIter> facesOfv0 = v0->AdjFaces();
	vector<FaceIter> facesOfv1 = v1->AdjFaces();

	for (auto f0 : facesOfv0) {
		if (f0->isBoundary())
			continue;

		for (auto f1 : facesOfv1) {
			if (f1->isBoundary())
				continue;

			if (f0 == f1)
				return f0;
		}
	}

	return faces.end();
}

VertexIter HalfedgeMesh::InsertVertex(EdgeIter e0) {
	// Insert a vertex on the middle position of edge.

	if (!IsValid(e0, "InsertVertex : e0 is null"))
		return vertices.end();
	
	//         |  |                           |  |
	//   (¡ü)he2|  |                     (¡ü)he2|  |
	//         |  |                           |  |
	//         [vv]                           [vv]
	//         |  |                           |  |
	//         |  |he3(¡ý)               (¡ü)he1|  |he3(¡ý)
	//         |  |                           |e1|
	//  {f0}   |  |   {f1}  =====>     {f0}   [v0]   {f1}
	//         |e0|                           |e0|
	//   (¡ü)he0|  |                     (¡ü)he0|  |he4(¡ý)
	//         |  |                           |  |
	//         [vv]                           [vv]
	//         |  |                           |  |
	//         |  |he5(¡ý)                     |  |he5(¡ý)
	//         |  |                           |  |

	VertexIter v0 = newVertex();
	
	EdgeIter e1 = newEdge();

	FaceIter f0 = e0->halfedge()->face();
	FaceIter f1 = e0->halfedge()->twin()->face();

	HalfedgeIter he0 = e0->halfedge();
	HalfedgeIter he2 = he0->next();
	HalfedgeIter he3 = he0->twin();
	HalfedgeIter he5 = he3->next();

	HalfedgeIter he1 = newHalfedge();
	HalfedgeIter he4 = newHalfedge();


	v0->position = e0->centroid();
	v0->halfedge() = he1;

	if (e0->halfedge() == he3)
		e0->halfedge() = he4;

	e1->halfedge() = he1;

	he1->vertex() = v0;
	he4->vertex() = v0;

	he1->edge() = e1;
	he3->edge() = e1;
	he4->edge() = e0;
	
	he1->face() = f0;
	he4->face() = f1;

	he0->twin() = he4;
	he4->twin() = he0;
	he1->twin() = he3;
	he3->twin() = he1;
	
	he0->next() = he1;
	he1->next() = he2;
	he3->next() = he4;
	he4->next() = he5;

	return v0;
}

VertexIter HalfedgeMesh::InsertVertex(vector<VertexIter> Vs, FaceIter f) {
	// Insert a vertex on the center position of [ordered] vertices in same face;
	// the face is not a boundary
	// this method won't check the validity of input

	if (f->isBoundary()) {
		showError("InsertVertex : f is boudary");
		return vertices.end();
	}

	if (Vs.size() < 2) {
		showError("InsertVertex : Vs' size is smaller than 2");
		return vertices.end();
	}

	// set Vs' order as f
	HalfedgeIter heOfV0 = halfedges.end();
	{// find halfedge of v0 in f
		HalfedgeIter he = f->halfedge();
		do {
			if (he->vertex() == Vs[0]) {
				heOfV0 = he;
				break;
			}

			he = he->next();
		} while (he != f->halfedge());

		if (!IsValid(heOfV0, "InsertVertex : v0 is not in f"))
			return vertices.end();
	}

	{// reverse Vs if it's order not like f
		HalfedgeIter nextIter = heOfV0->next();
		HalfedgeIter preIter = heOfV0->pre();

		while (true) {
			if (nextIter->vertex() == Vs[1])
				break;
			else if (preIter->vertex() == Vs[1]) {
				reverse(Vs.begin(), Vs.end());
				break;
			}
			if (nextIter == heOfV0 || preIter == heOfV0) {
				showError("InsertVertex : v1 is not in face");
				return vertices.end();
			}

			nextIter = nextIter->next();
			preIter = preIter->pre();
		}
	}
	
	// get center position
	Vector3D centerPos(0, 0, 0);
	for (auto v : Vs)
		centerPos += v->position;

	centerPos *= 1.0 / Vs.size();

	// connect form v0 to v1
	// the f will be on the [left side] of v0v1
	// so f can be use later
	EdgeIter e = ConnectVertex(Vs[0], Vs[1], f);

	VertexIter v = InsertVertex(e);

	for (size_t i = 2; i < Vs.size(); i++)
		ConnectVertex(v, Vs[i], f);

	v->position = centerPos;

	return v;
}

VertexIter HalfedgeMesh::InsertVertex(set<VertexIter> Vs, FaceIter f) {
	// Insert a vertex on the center position of[unordered] vertices in same face
	// the face is not a boundary

	return InsertVertex(f->SortVertices(Vs), f);
}

VertexIter HalfedgeMesh::InsertVertex(FaceIter f) {
	// Insert a vertex on the center of f
	// f should not be a boundary

	if (f->isBoundary()) {
		showError("InsertVertex : f is boundary", true);
		return vertices.end();
	}
	
	vector<VertexIter> verticesOfFace = f->Vertices();

	return InsertVertex(verticesOfFace, f);
}

EdgeIter HalfedgeMesh::ConnectVertex(VertexIter v0, VertexIter v1, FaceIter f) {
	// Connect two verties in a face
	// original face is on the [left side] of v0v1
	// new face is on the [right side] of v0v1

	if (f->isBoundary()) {
		showError("ERROR::ConnectVertex : f is boundary");
		return edges.end();
	}

	if (!IsValid(v0, "ConnectVertex : v0 is null")
		|| !IsValid(v1, "ConnectVertex : v1 is null"))
		return edges.end();

	//  |                                        |
	// he0(¡ý)                                 (¡ü)he3
	//  |               new He0(->)              |
	//  v0  - - - - - - -  new E  - - - - - - -  v1
	//  |               new He1(<-)              |
	// he1(¡ý)                                 (¡ü)he2
	//  |                                        |

	vector<HalfedgeIter> halfedgesV0 = v0->AdjHalfedges();
	vector<HalfedgeIter> halfedgesV1 = v1->AdjHalfedges();

	HalfedgeIter he1 = halfedges.end();
	HalfedgeIter he3 = halfedges.end();

	// find a halfedge of v in f
	// !!! in special case, maybe more halfedges of v in f
	// example :
	// 
	//   ------------------------
	//   |                      |
	//   |                      |
	//   |      |        |      |
	//   |      |        |      |
	//   ------[v]------[v]------
	// 
	for (auto he : halfedgesV0) {
		if (!he->face()->isBoundary() && he->face() == f) {
			he1 = he;
			break;
		}
	}

	if (!IsValid(he1, "ConnectVertex : v0 is not in face f"))
		return edges.end();

	// go form he1, update he1 and find he3
	for (auto curHe = he1->next(), endHe = he1; curHe != endHe; curHe = curHe->next()) {
		if (curHe->vertex() == v0)
			he1 = curHe;
		else if (curHe->vertex() == v1) {
			he3 = curHe;
			break;
		}
	}

	if (!IsValid(he3, "ConnectVertex : v1 is not in face f"))
		return edges.end();

	HalfedgeIter he0 = he1->pre();
	HalfedgeIter he2 = he3->pre();


	HalfedgeIter newHe0 = newHalfedge();
	HalfedgeIter newHe1 = newHalfedge();


	newHe0->vertex() = v0;
	newHe1->vertex() = v1;


	EdgeIter newE = newEdge();
	newE->halfedge() = newHe1;
	newHe0->edge() = newE;
	newHe1->edge() = newE;


	newHe0->twin() = newHe1;
	newHe1->twin() = newHe0;

	FaceIter newF = newFace();
	f->halfedge() = newHe0;
	newF->halfedge() = newHe1;
	{
		HalfedgeIter he = he1;
		while (true) {
			he->face() = newF;
			if (he == he2)
				break;
			he = he->next();
		}
	}
	newHe0->face() = f;
	newHe1->face() = newF;


	he0->next() = newHe0;
	newHe0->next() = he3;
	he2->next() = newHe1;
	newHe1->next() = he1;

	return newE;
}

VertexIter HalfedgeMesh::splitEdge(EdgeIter e) {
	// This method should split the given edge and return an iterator to the
	// newly inserted vertex. The halfedge of this vertex should point along
	// the edge that was split, rather than the new edges.

	if (!IsValid(e, "splitEdge: e is null"))
		return vertices.end();

	// [Note:this method is for triangle meshes only!]
	if (e->halfedge()->face()->degree() != 3
		|| e->halfedge()->twin()->face()->degree() != 3) {
		showError("splitEdge is for triangle meshes only!");
		printf("ERROR::splitEdge : splitEdge is for triangle meshes only!\n(%d,%d)\n",
			e->halfedge()->face()->degree(),
			e->halfedge()->twin()->face()->degree());
		return vertices.end();
	}

	// The selected edge e is split at its midpoint
	// and the new vertex v is connected to the two opposite vertices
	// (or one in the case of a surface with boundary)

	// e is boundary
	if (e->isBoundary()) {
		//         --[vv]                              --[vv]
		//       --  |  |                            --  |  |
		//     --    |  |                          --    |  |
		//   --      |  |                        --      |e1|
		// --        |ee|                      --        |  |
		//[v0]     he|  |            ===>     [v0]-------[v1]
		// --        |  |                      --        |  | 
		//   --      |  | {boundary}             --      |e0| {boundary}
		//     --    |  |                          --    |  |
		//       --  |  |                            --  |  |
		//         --[vv]                              --[vv]

		HalfedgeIter he = e->halfedge()->isBoundary() ? e->halfedge() : e->halfedge()->twin();
		VertexIter v0 = he->next()->next()->vertex();

		VertexIter v1 = InsertVertex(e);

		ConnectVertex(v0, v1, he->face());

		return v1;
	}

	// normal case
	//
	//         --[vv]--                                --[vv]--
	//       --  |  |  --                            --  |  |  --
	//     --    |  |    --                        --    |  |    --
	//   --      |  |      --                    --      |e1|      --
	// --        |ee|        --                --        |  |        --
	//[v0]     he|  |twin   [v1]     ===>     [v0]-------[v2]-------[v1]
	// -- {f0}   |  |   {f1} --                --        |  |        --
	//   --      |  |      --                    --      |e0|      --
	//     --    |  |    --                        --    |  |    --
	//       --  |  |  --                            --  |  |  --
	//         --[vv]--                                --[vv]--


	HalfedgeIter he = e->halfedge();
	HalfedgeIter twin = he->twin();
	FaceIter f0 = he->face();
	FaceIter f1 = twin->face();
	VertexIter v0 = he->next()->next()->vertex();
	VertexIter v1 = twin->next()->next()->vertex();

	VertexIter v2 = InsertVertex(e);
	ConnectVertex(v0, v2, f0);
	ConnectVertex(v2, v1, f1);

	return v2;
}

VertexIter HalfedgeMesh::collapseEdge(EdgeIter e) {
	// This method should collapse the given edge and return an iterator to
	// the new vertex created by the collapse.

	if (e == edges.end()) {
		showError("splitEdge: e is null");
		return vertices.end();
	}

	// The selected edge e is replaced by a single vertex v.
	// This vertex is connected by edges to all vertices previously connected to either endpoint of e. 
	// Moreover, if either of the polygons containing e was a triangle, it will be replaced by an edge
	// (rather than a degenerate polygon with only two edges)

	// the new vertex is the centroid of the edge

	if (e->isBoundary()) {
		showError("Can't collapse a boundary edge");
		return vertices.end();
	}

	if (e->IsBridge()) {
		showError("Can't collapse a bridge edge");
		return vertices.end();
	}

	set<EdgeIter> adjEs = e->AdjEdges();

	for (auto adjE : adjEs) {
		if (adjE->isBoundary()) {
			showError("Can't collapse face with boundary vertex");
			return vertices.end();
		}

		if (adjE->IsBridge()) {
			showError("Can't collapse face with adjacent bridge edge");
			return vertices.end();
		}
	}

	vector<VertexIter> adjVs = e->AdjVertices();

	if (!IsValid(eraseEdge(e), "collapseEdge : erase edge fail"))
		return vertices.end();

	FaceIter f;

	while (adjEs.size() > 0) {
		size_t origSize = adjEs.size();
		for (auto adjE : adjEs) {
			if (!adjE->IsBridge()) {
				adjEs.erase(adjE);
				f = eraseEdge(adjE);
				if (!IsValid(f, "collapseEdge : erase a adjacent edge fail"))
					return vertices.end();
				break;// must break here because iterator has been destroy
			}
		}

		if (origSize == adjEs.size()) {
			showError("collapseFace : Can't delete more adjacent edges", true);
			return vertices.end();
		}
	}

	return InsertVertex(adjVs, f);
}

VertexIter HalfedgeMesh::collapseFace(FaceIter f) {
	// This method should collapse the given face and return an iterator to
	// the new vertex created by the collapse.
	
	// The selected face f is replaced by a single vertex v.
	// All edges previously connected to vertices of f are now connected directly to v.

	if (f->isBoundary()) {
		showError("collapseFace: f is boundary");
		return vertices.end();
	}

	if (f->IsBridge()) {
		showError("collapseFace: f is bridge");
		return vertices.end();
	}

	auto adjFs = f->AdjFaces();
	for (auto adjF : adjFs) {
		if (adjF->IsBridge()) {
			showError("collapseFace: one of adjacent face is bridge");
			return vertices.end();
		}
	}

	vector<EdgeIter> edgesOfFace = f->Edges();

	for (auto e : edgesOfFace) {
		if (e->isBoundary()) {
			showError("Can't collapse face with boundary vertex");
			return vertices.end();
		}

		if (e->isBoundary()) {
			showError("Can't collapse face with bridge edge");
			return vertices.end();
		}
	}
	
	set<EdgeIter> adjEsOfFace = f->AdjEdges();
	for (auto adjE :adjEsOfFace) {
		if (adjE->isBoundary()) {
			showError("Can't collapse face with boundary vertex");
			return vertices.end();
		}

		if (adjE->IsBridge()) {
			showError("Can't collapse face with adjacent bridge edge");
			return vertices.end();
		}
	}

	set<VertexIter> adjVs = f->AdjVertices();

	while (edgesOfFace.size() > 0) {
		size_t origSize = edgesOfFace.size();
		for (int i = edgesOfFace.size() - 1; i >= 0; i--) {
			EdgeIter e = edgesOfFace[i];
			if (!e->isBoundary() && !e->IsBridge()) {
				edgesOfFace.erase(edgesOfFace.begin() + i);
				if (!IsValid(eraseEdge(e), "collapseFace : erase edge fail"))
					return vertices.end();
			}
		}
		if (origSize == edgesOfFace.size())
			break;
	}

	for (auto e : edgesOfFace)
		adjEsOfFace.insert(e);

	while (adjEsOfFace.size() > 0) {
		size_t origSize = adjEsOfFace.size();
		for (auto adjE : adjEsOfFace) {
			if (!adjE->IsBridge()) {
				adjEsOfFace.erase(adjE);
				f = eraseEdge(adjE);
				if (!IsValid(f, "collapseFace : erase edge fail"))
					return vertices.end();
				break;// must break here because iterator has been destroy
			}
		}
		if (origSize == adjEsOfFace.size()) {
			showError("Can't delete more adjacent edges when collapsing face", true);
			return vertices.end();
		}
	}

	return InsertVertex(adjVs, f);
}

FaceIter HalfedgeMesh::eraseVertex(VertexIter v) {
	// This method should replace the given vertex and all its neighboring
	// edges and faces with a single face, returning the new face.

	if (v == vertices.end()) {
		showError("eraseVertex : v is null");
		return faces.end();
	}

	if (v->isBoundary()) {
		showError("Can't erase a boundary vertex");
		return faces.end();
	}

	vector<EdgeIter> adjEs = v->AdjEdges();
	for (auto adjE : adjEs) {
		if (adjE->IsBridge()) {
			showError("Can't erase a vertex with adjacent bridge edge");
			return faces.end();
		}
	}

	FaceIter f;
	for (auto adjE : adjEs) {
		f = eraseEdge(adjE);
		if (f == faces.end()) {
			showError("eraseVertex : erase edge fail", true);
		}
	}

	// no need to erase vertex
	// it will be erased by the final call of eraseEdge

	return f;
}

FaceIter HalfedgeMesh::eraseEdge(EdgeIter e) {
	// This method should erase the given edge and return an iterator to the
	// merged face.

	// The selected edge e will be replaced with the union of the faces containing it
	// producing a new face e.(if e is a boundary edge, nothing happens).

	if (e == edges.end()) {
		showError("eraseEdge: e is null");
		return faces.end();
	}

	if (e->isBoundary()) {
		showError("Can't erase a boundary edge");
		return faces.end();
	}

	if (e->IsBridge()) {
		showError("Can't erase a bridge edge");
		return faces.end();
	}

	// same face
	if (e->halfedge()->face() == e->halfedge()->twin()->face()) {
		// single line
		if (e->halfedge()->next()->next() == e->halfedge() && e->halfedge()->next() == e->halfedge()->twin()) {
			deleteVertex(e->halfedge()->twin()->vertex());
			deleteVertex(e->halfedge()->vertex());
			deleteFace(e->halfedge()->face());
			deleteHalfedge(e->halfedge()->twin());
			deleteHalfedge(e->halfedge());
			deleteEdge(e);
			return faces.end();
		}

		//         [v1]
		//         |  |
		//         |ee|
		//    (¡ü)he|  |twin(¡ý)
		//         |  |
		// (¡ú)     |  |     (¡ú)
		// he0 ----[v0]---- he1

		HalfedgeIter he = e->halfedge()->next() == e->halfedge()->twin() ? e->halfedge() : e->halfedge()->twin();
		HalfedgeIter twin = he->twin();
		HalfedgeIter he0 = he->pre();
		HalfedgeIter he1 = twin->next();

		VertexIter v0 = he->vertex();
		VertexIter v1 = twin->vertex();

		FaceIter f = he->face();

		if (v0->halfedge() == he)
			v0->halfedge() = he1;

		if (f->halfedge() == he || f->halfedge() == twin)
			f->halfedge() = he0;

		he0->next() = he1;
		
		deleteVertex(v1);
		deleteHalfedge(he);
		deleteHalfedge(twin);
		deleteEdge(e);
		return f;
	}

	// normal case
	
	// -----------[v1]-------------
	//     he1    |  |     he2
	//     (¡û)    |  |     (¡û)
	//            |ee|
	// {f0}  (¡ü)he|  |twin(¡ý)  {f1}
	//            |  |
	//     (¡ú)    |  |     (¡ú)
	//     he0    |  |     he3
	// -----------[v0]-------------

	HalfedgeIter he = e->halfedge();
	HalfedgeIter twin = he->twin();
	HalfedgeIter he0 = he->pre();
	HalfedgeIter he1 = he->next();
	HalfedgeIter he2 = twin->pre();
	HalfedgeIter he3 = twin->next();

	VertexIter v0 = he->vertex();
	VertexIter v1 = twin->vertex();

	FaceIter f0 = he->face();
	FaceIter f1 = twin->face();

	if (v0->halfedge() == he)
		v0->halfedge() = he3;

	if (v1->halfedge() == twin)
		v1->halfedge() = he1;

	if (f0->halfedge() == he)
		f0->halfedge() = he0;

	for (HalfedgeIter curHe = he3; curHe != twin; curHe = curHe->next())
		curHe->face() = f0;

	he0->next() = he3;
	he2->next() = he1;

	deleteHalfedge(he);
	deleteHalfedge(twin);
	deleteEdge(e);
	deleteFace(f1);

	return f0;
}

EdgeIter HalfedgeMesh::flipEdge(EdgeIter e) {
	// This method should flip the given edge and return an iterator to the
	// flipped edge.

	// The selected edge e is "rotated" around the face, 
	// in the sense that each endpoint moves to the next vertex (in counter-clockwise order)
	// along the boundary of the two polygons containing e.
	if (!IsValid(e, "flipEdge: e is null"))
		return edges.end();

	if (e->isBoundary()) {
		showError("Can't flip boundary edge");
		return e;
	}

	VertexIter v0 = e->halfedge()->next()->twin()->vertex();
	VertexIter v1 = e->halfedge()->twin()->next()->twin()->vertex();

	FaceIter f = eraseEdge(e);
	if(!IsValid(f, "flipEdge : erase a edge fail"))
		return edges.end();

	return ConnectVertex(v0, v1, f);
}

void HalfedgeMesh::subdivideQuad(bool useCatmullClark) {
  // Unlike the local mesh operations (like bevel or edge flip), we will perform
  // subdivision by splitting *all* faces into quads "simultaneously."  Rather
  // than operating directly on the halfedge data structure (which as you've
  // seen
  // is quite difficult to maintain!) we are going to do something a bit nicer:
  //
  //    1. Create a raw list of vertex positions and faces (rather than a full-
  //       blown halfedge mesh).
  //
  //    2. Build a new halfedge mesh from these lists, replacing the old one.
  //
  // Sometimes rebuilding a data structure from scratch is simpler (and even
  // more
  // efficient) than incrementally modifying the existing one.  These steps are
  // detailed below.

  // Step I: Compute the vertex positions for the subdivided mesh.  Here
  // we're
  // going to do something a little bit strange: since we will have one vertex
  // in
  // the subdivided mesh for each vertex, edge, and face in the original mesh,
  // we
  // can nicely store the new vertex *positions* as attributes on vertices,
  // edges,
  // and faces of the original mesh.  These positions can then be conveniently
  // copied into the new, subdivided mesh.
  if (useCatmullClark) {
    computeCatmullClarkPositions();
  } else {
    computeLinearSubdivisionPositions();
  }

  // Step II: Assign a unique index (starting at 0) to each vertex, edge,
  // and
  // face in the original mesh.  These indices will be the indices of the
  // vertices
  // in the new (subdivided mesh).  They do not have to be assigned in any
  // particular
  // order, so long as no index is shared by more than one mesh element, and the
  // total number of indices is equal to V+E+F, i.e., the total number of
  // vertices
  // plus edges plus faces in the original mesh.  Basically we just need a
  // one-to-one
  // mapping between original mesh elements and subdivided mesh vertices.
  assignSubdivisionIndices();

  // Step III: Build a list of quads in the new (subdivided) mesh, as
  // tuples of
  // the element indices defined above.  In other words, each new quad should be
  // of
  // the form (i,j,k,l), where i,j,k and l are four of the indices stored on our
  // original mesh elements.  Note that it is essential to get the orientation
  // right
  // here: (i,j,k,l) is not the same as (l,k,j,i).  Indices of new faces should
  // circulate in the same direction as old faces (think about the right-hand
  // rule).
  vector<vector<Index> > subDFaces;
  vector<Vector3D> subDVertices;
  buildSubdivisionFaceList(subDFaces);
  buildSubdivisionVertexList(subDVertices);

  // Step IV: Pass the list of vertices and quads to a routine that clears
  // the
  // internal data for this halfedge mesh, and builds new halfedge data from
  // scratch,
  // using the two lists.
  rebuild(subDFaces, subDVertices);
}

/**
 * Compute new vertex positions for a mesh that splits each polygon
 * into quads (by inserting a vertex at the face midpoint and each
 * of the edge midpoints).  The new vertex positions will be stored
 * in the members Vertex::newPosition, Edge::newPosition, and
 * Face::newPosition.  The values of the positions are based on
 * simple linear interpolation, e.g., the edge midpoints and face
 * centroids.
 */
void HalfedgeMesh::computeLinearSubdivisionPositions() {
	// For each vertex, assign Vertex::newPosition to
	// its original position, Vertex::position.
	for (auto & v : vertices)
		v.newPosition = v.position;

	// For each edge, assign the midpoint of the two original
	// positions to Edge::newPosition.
	for (auto & e : edges)
		e.newPosition = e.centroid();

	// For each face, assign the centroid (i.e., arithmetic mean)
	// of the original vertex positions to Face::newPosition.  Note
	// that in general, NOT all faces will be triangles!
	for (auto & f : faces)
		f.newPosition = f.centroid();
}

/**
 * Compute new vertex positions for a mesh that splits each polygon
 * into quads (by inserting a vertex at the face midpoint and each
 * of the edge midpoints).  The new vertex positions will be stored
 * in the members Vertex::newPosition, Edge::newPosition, and
 * Face::newPosition.  The values of the positions are based on
 * the Catmull-Clark rules for subdivision.
 */
void HalfedgeMesh::computeCatmullClarkPositions() {
	// The implementation for this routine should be
	// a lot like HalfedgeMesh::computeLinearSubdivisionPositions(),
	// except that the calculation of the positions themsevles is
	// slightly more involved, using the Catmull-Clark subdivision
	// rules. (These rules are outlined in the Developer Manual.)

	if (boundaries.size() > 0) {
		showError("Can't support meshes with boundary");
		return;
	}

	// face
	for (auto & f : faces)
		f.newPosition = f.centroid();

	// edges
	for (auto & e : edges) {
		e.newPosition = 0.25 *
			(e.halfedge()->face()->newPosition
				+ e.halfedge()->twin()->face()->newPosition
				+ e.halfedge()->vertex()->position
				+ e.halfedge()->twin()->vertex()->position);
	}

	// vertices
	for (auto & v : vertices) {
		// Q
		Vector3D Q(0, 0, 0);
		vector<FaceIter> adjFs = v.AdjFaces();
		for (auto adjF : adjFs)
			Q += adjF->newPosition;

		Q *= 1.0 / adjFs.size();

		// R
		Vector3D R(0, 0, 0);
		vector<EdgeIter> adjEs = v.AdjEdges();
		for (auto adjE : adjEs)
			R += adjE->centroid();// [original] edge midpoints

		R *= 1.0 / adjEs.size();

		// S
		Vector3D S = v.position;

		// Vertex::newPosition
		size_t n = v.degree();
		v.newPosition = (Q + 2 * R + (n - 3) * S) / n;
	}
}

/**
 * Assign a unique integer index to each vertex, edge, and face in
 * the mesh, starting at 0 and incrementing by 1 for each element.
 * These indices will be used as the vertex indices for a mesh
 * subdivided using Catmull-Clark (or linear) subdivision.
 */
void HalfedgeMesh::assignSubdivisionIndices() {
	// Start a counter at zero; if you like, you can use the
	// "Index" type (defined in halfedgeMesh.h)
	size_t cnt = 0;

	// Iterate over vertices, assigning values to Vertex::index
	for (auto &v : vertices)
		v.index = cnt++;

	// Iterate over edges, assigning values to Edge::index
	for (auto &e : edges)
		e.index = cnt++;

	// Iterate over faces, assigning values to Face::index
	for (auto &f : faces)
		f.index = cnt++;
}

/**
 * Build a flat list containing all the vertex positions for a
 * Catmull-Clark (or linear) subdivison of this mesh.  The order of
 * vertex positions in this list must be identical to the order
 * of indices assigned to Vertex::newPosition, Edge::newPosition,
 * and Face::newPosition.
 */
void HalfedgeMesh::buildSubdivisionVertexList(vector<Vector3D>& subDVertices) {
	// Resize the vertex list so that it can hold all the vertices.
	subDVertices.resize(vertices.size() + edges.size() + faces.size());

	// Iterate over vertices, assigning Vertex::newPosition to the
	// appropriate location in the new vertex list.
	for (auto & v : vertices)
		subDVertices[v.index] = v.newPosition;

	// Iterate over edges, assigning Edge::newPosition to the appropriate
	// location in the new vertex list.
	for (auto & e : edges)
		subDVertices[e.index] = e.newPosition;

	// Iterate over faces, assigning Face::newPosition to the appropriate
	// location in the new vertex list.
	for (auto & f : faces)
		subDVertices[f.index] = f.newPosition;
}

/**
 * Build a flat list containing all the quads in a Catmull-Clark
 * (or linear) subdivision of this mesh.  Each quad is specified
 * by a vector of four indices (i,j,k,l), which come from the
 * members Vertex::index, Edge::index, and Face::index.  Note that
 * the ordering of these indices is important because it determines
 * the orientation of the new quads; it is also important to avoid
 * "bowties."  For instance, (l,k,j,i) has the opposite orientation
 * of (i,j,k,l), and if (i,j,k,l) is a proper quad, then (i,k,j,l)
 * will look like a bowtie.
 */
void HalfedgeMesh::buildSubdivisionFaceList(vector<vector<Index> >& subDFaces) {
	// This routine is perhaps the most tricky step in the construction of
	// a subdivision mesh (second, perhaps, to computing the actual Catmull-Clark
	// vertex positions).  Basically what you want to do is iterate over faces,
	// then for each for each face, append N quads to the list (where N is the
	// degree of the face).  For this routine, it may be more convenient to simply
	// append quads to the end of the list (rather than allocating it ahead of
	// time), though YMMV.  You can of course iterate around a face by starting
	// with its first halfedge and following the "next" pointer until you get
	// back to the beginning.  The tricky part is making sure you grab the right
	// indices in the right order---remember that there are indices on vertices,
	// edges, AND faces of the original mesh.  All of these should get used.  Also
	// remember that you must have FOUR indices per face, since you are making a
	// QUAD mesh!

	// iterate over faces
	for (auto & f : faces) {
		// loop around face
		HalfedgeIter he = f.halfedge();
		do {
			// build lists of four indices for each sub-quad
			Index i = he->edge()->index;
			Index j = he->twin()->vertex()->index;
			Index k = he->next()->edge()->index;
			Index l = f.index;
			vector<Index> quadIndices({ i,j,k,l });

			// append each list of four indices to face list
			subDFaces.push_back(quadIndices);

			he = he->next();
		} while (he != f.halfedge());
	}
}

FaceIter HalfedgeMesh::bevelVertex(VertexIter v) {
	// This method should replace the vertex v with a face, corresponding to
	// a bevel operation. It should return the new face.  NOTE: This method is
	// responsible for updating the *connectivity* of the mesh only---it does not
	// need to update the vertex positions.  These positions will be updated in
	// HalfedgeMesh::bevelVertexComputeNewPositions (which you also have to
	// implement!)

	if (!IsValid(v, "bevelVertex : v is null"))
		return faces.end();

	if (v->isBoundary()) {
		showError("bevelVertex : v is boundary");
		return faces.end();
	}

	// 1. collect edge
	vector<EdgeIter> edges = v->AdjEdges();

	// 2. insert V
	vector<VertexIter> newV;
	for (size_t i = 0; i < edges.size(); i++)
		newV.push_back(InsertVertex(edges[i]));
	
	// 3. connect V
	for (size_t i = 0; i < newV.size(); i++) {
		VertexIter v0 = newV[i];
		VertexIter v1 = newV[(i + 1) % newV.size()];
		ConnectVertex(v0, v1, GetSameFace(v0, v1));
	}
	
	return eraseVertex(v);
}

FaceIter HalfedgeMesh::bevelEdge(EdgeIter e) {
	if (e->isBoundary()) {
		showError("bevelEdge : e is boundary");
		return faces.end();
	}

	if (e->halfedge()->vertex()->isBoundary()
		|| e->halfedge()->twin()->vertex()->isBoundary()) {
		showError("vevelEdge : a vertex of e is boundary");
		return faces.end();
	}

	// 1. collect edge
	vector<EdgeIter> edges;
	HalfedgeIter heArr[2] = { e->halfedge() , e->halfedge()->twin() };
	for (int i = 0; i < 2; i++) {
		HalfedgeIter he = heArr[i]->twin()->next();
		do {
			edges.push_back(he->edge());
			he = he->twin()->next();
		} while (he->edge() != e);
	}

	// 2. insert V
	vector<VertexIter> newV;
	for (size_t i = 0; i < edges.size(); i++)
		newV.push_back(InsertVertex(edges[i]));

	// 3. connect V
	for (size_t i = 0; i < newV.size(); i++) {
		VertexIter v0 = newV[i];
		VertexIter v1 = newV[(i + 1) % newV.size()];
		ConnectVertex(v0, v1, GetSameFace(v0,v1));
	}
	
	VertexIter v0 = e->halfedge()->vertex();
	VertexIter v1 = e->halfedge()->twin()->vertex();
	eraseVertex(v0);
	return eraseVertex(v1);
}

FaceIter HalfedgeMesh::bevelFace(FaceIter f) {
	// This method should replace the face f with an additional, inset face
	// (and ring of faces around it), corresponding to a bevel operation. It
	// should return the new face.  NOTE: This method is responsible for updating
	// the *connectivity* of the mesh only---it does not need to update the vertex
	// positions.  These positions will be updated in
	// HalfedgeMesh::bevelFaceComputeNewPositions (which you also have to
	// implement!)

	if (f->isBoundary()) {
		showError("bevelFace : f is boundary");
		return faces.end();
	}

	HalfedgeIter he = f->halfedge();

	if (f->isBoundary()) {
		showError("Can't bevel a boundary face");
		return faces.end();
	}

	VertexIter v = InsertVertex(f);

	FaceIter rstF = bevelVertex(v);

	auto hesOfFace = rstF->Halfedges();
	for (auto heOfFace : hesOfFace) {
		if (heOfFace->twin()->next()->next() == he) {
			rstF->halfedge() = heOfFace;
			break;
		}
	}

	return rstF;
}

void HalfedgeMesh::bevelFaceComputeNewPositions(
	vector<Vector3D>& originalVertexPositions,
	vector<HalfedgeIter>& newHalfedges, double normalShift,
	double tangentialInset) {
	// Compute new vertex positions for the vertices of the beveled face.
	//
	// These vertices can be accessed via newHalfedges[i]->vertex()->position for
	// i = 1, ..., newHalfedges.size()-1.
	//
	// The basic strategy here is to loop over the list of outgoing halfedges,
	// and use the preceding and next vertex position from the original mesh
	// (in the originalVertexPositions array) to compute an offset vertex
	// position.
	//
	// Note that there is a 1-to-1 correspondence between halfedges in
	// newHalfedges and vertex positions
	// in orig.  So, you can write loops of the form
	//
	// for( int i = 0; i < newHalfedges.size(); hs++ )
	// {
	//    Vector3D pi = originalVertexPositions[i]; // get the original vertex
	//    position correponding to vertex i
	// }
	//

	size_t n = newHalfedges.size();
	Vector3D norm(0, 0, 0);

	for (size_t i = 0; i < n - 2; i++) {
		Vector3D p0 = newHalfedges[i]->twin()->vertex()->position;
		Vector3D p1 = newHalfedges[i + 1]->twin()->vertex()->position;
		Vector3D p2 = newHalfedges[i + 2]->twin()->vertex()->position;
		Vector3D a = p1 - p0;
		Vector3D b = p2 - p0;
		norm += cross(a, b);
	}

	Vector3D centerPos(0, 0, 0);
	for (auto pos : originalVertexPositions)
		centerPos += pos;

	centerPos *= 1.0 / n;

	norm.normalize();

	double scale = 5.0;
	for (size_t i = 0; i < n; i++) {
		Vector3D dir = originalVertexPositions[i] - centerPos;
		newHalfedges[i]->vertex()->position += scale * (tangentialInset * dir + normalShift * norm);
	}
}

void HalfedgeMesh::bevelVertexComputeNewPositions(
	Vector3D originalVertexPosition, vector<HalfedgeIter>& newHalfedges,
	double tangentialInset) {
	// Compute new vertex positions for the vertices of the beveled vertex.
	//
	// These vertices can be accessed via newHalfedges[i]->vertex()->position for
	// i = 1, ..., hs.size()-1.
	//
	// The basic strategy here is to loop over the list of outgoing halfedges,
	// and use the preceding and next vertex position from the original mesh
	// (in the orig array) to compute an offset vertex position.

	vector<Vector3D> deltas;
	double scale0 = 10.0;
	for (size_t i = 0; i < newHalfedges.size(); i++) {
		VertexIter v0 = newHalfedges[i]->vertex();
		VertexIter v1 = newHalfedges[i]->twin()->vertex();

		Vector3D dir = v1->position - originalVertexPosition;
		Vector3D delta = dir * scale0 * tangentialInset;
		
		if ((tangentialInset < 0 && delta.norm2() >= (v0->position - originalVertexPosition).norm2())
			|| (tangentialInset > 0 && delta.norm2() >= (v1->position - v0->position).norm2()))
			return;
		
		deltas.push_back(delta);
	}

	for (size_t i = 0; i < newHalfedges.size(); i++)
		newHalfedges[i]->vertex()->position += deltas[i];
}

void HalfedgeMesh::bevelEdgeComputeNewPositions(
	vector<Vector3D>& originalVertexPositions,
	vector<HalfedgeIter>& newHalfedges, double tangentialInset) {
	// Compute new vertex positions for the vertices of the beveled edge.
	//
	// These vertices can be accessed via newHalfedges[i]->vertex()->position for
	// i = 1, ..., newHalfedges.size()-1.
	//
	// The basic strategy here is to loop over the list of outgoing halfedges,
	// and use the preceding and next vertex position from the original mesh
	// (in the orig array) to compute an offset vertex position.
	//
	// Note that there is a 1-to-1 correspondence between halfedges in
	// newHalfedges and vertex positions
	// in orig.  So, you can write loops of the form
	//
	// for( int i = 0; i < newHalfedges.size(); i++ )
	// {
	//    Vector3D pi = originalVertexPositions[i]; // get the original vertex
	//    position correponding to vertex i
	// }
	//

	vector<Vector3D> deltas;
	double scale0 = 5.0;
	for (size_t i = 0; i < newHalfedges.size(); i++) {
		VertexIter v0 = newHalfedges[i]->vertex();
		VertexIter v1 = newHalfedges[i]->twin()->vertex();

		Vector3D origPos = 2 * originalVertexPositions[i] - v1->position;

		Vector3D dir = v1->position - origPos;
		Vector3D delta = dir * scale0 * tangentialInset;

		if ((tangentialInset < 0 && delta.norm2() >= (v0->position - origPos).norm2())
			|| (tangentialInset > 0 && delta.norm2() >= (v1->position - v0->position).norm2()))
			return;

		deltas.push_back(delta);
	}

	for (size_t i = 0; i < newHalfedges.size(); i++)
		newHalfedges[i]->vertex()->position += deltas[i];
}

void HalfedgeMesh::splitPolygons(vector<FaceIter>& fcs) {
  for (auto f : fcs) splitPolygon(f);
}

void HalfedgeMesh::splitPolygon(FaceIter f) {
	// Triangulate a polygonal face
	// zig-zag
	
	if (f->isBoundary()) {
		showError("Can't triangulate a boundary face");
		return;
	}

	vector<VertexIter> verticesOfFace = f->Vertices();

	bool side = true;
	for (size_t left = 1, right = verticesOfFace.size() - 1; right - left > 1; left += side, right -= !side) {
		// connect form right to left
		// and original face will be [left side] of v0v1
		// so the face can be use later
		ConnectVertex(verticesOfFace[right], verticesOfFace[left], f);

		side = !side;
	}
}

EdgeRecord::EdgeRecord(EdgeIter& _edge) : edge(_edge) {
	// Compute the combined quadric from the edge endpoints.

	// -> Build the 3x3 linear system whose solution minimizes the quadric error
	//    associated with these two endpoints.
	Matrix4x4 quadricE = edge->halfedge()->vertex()->quadric + edge->halfedge()->twin()->vertex()->quadric;
	Matrix3x3 A;
	for (size_t x = 0; x <= 2; x++) {
		for (size_t y = 0; y <= 2; y++)
			A(x, y) = quadricE(x, y);
	}
	Vector3D w(quadricE(0,3), quadricE(1,3), quadricE(2,3));
	Vector3D b = -w;

	// -> Use this system to solve for the optimal position, and store it in
	//    EdgeRecord::optimalPoint.
	if (abs(A.det()) < 0.000001) {
		optimalPoint = edge->centroid();
	}
	else {
		optimalPoint = A.inv() * b;
	}

	// -> Also store the cost associated with collapsing this edge in
	//    EdgeRecord::score.
	Vector4D u = Vector4D(optimalPoint, 1);
	score = dot((quadricE * u), u);
}

void MeshResampler::upsample(HalfedgeMesh& mesh) {
	// This routine should increase the number of triangles in the mesh using Loop
	// subdivision.

	// Compute new positions for all the vertices in the input mesh, using
	// the Loop subdivision rule, and store them in Vertex::newPosition.
	// -> At this point, we also want to mark each vertex as being a vertex of the
	//    original mesh.
	// -> Next, compute the updated vertex positions associated with edges, and
	//    store it in Edge::newPosition.
	// -> Next, we're going to split every edge in the mesh, in any order.  For
	//    future reference, we're also going to store some information about which
	//    subdivided edges come from splitting an edge in the original mesh, and
	//    which edges are new, by setting the flat Edge::isNew. Note that in this
	//    loop, we only want to iterate over edges of the original mesh.
	//    Otherwise, we'll end up splitting edges that we just split (and the
	//    loop will never end!)
	// -> Now flip any new edge that connects an old and new vertex.
	// -> Finally, copy the new vertex positions into final Vertex::position.

	// Each vertex and edge of the original surface can be associated with a
	// vertex in the new (subdivided) surface.
	// Therefore, our strategy for computing the subdivided vertex locations is to
	// *first* compute the new positions
	// using the connectity of the original (coarse) mesh; navigating this mesh
	// will be much easier than navigating
	// the new subdivided (fine) mesh, which has more elements to traverse.  We
	// will then assign vertex positions in
	// the new mesh based on the values we computed for the original mesh.

	// Compute updated positions for all the vertices in the original mesh, using
	// the Loop subdivision rule.
	for (auto v = mesh.verticesBegin(); v != mesh.verticesEnd(); v++) {
		vector<VertexIter> adjVs = v->AdjVertices();
		size_t n = adjVs.size();
		double u = n == 3 ? 3.0 / 16.0 : 3.0 / (8.0*n);
		v->newPosition = (1 - n * u)*v->position;
		for (auto adjV : adjVs)
			v->newPosition += u * adjV->position;

		v->isNew = false;
	}

	// Next, compute the updated vertex positions associated with edges.
	for (auto e = mesh.edgesBegin(); e != mesh.edgesEnd(); e++) {
		Vector3D pos0 = e->halfedge()->vertex()->position;
		Vector3D pos1 = e->halfedge()->twin()->vertex()->position;
		Vector3D pos2 = e->halfedge()->next()->next()->vertex()->position;
		Vector3D pos3 = e->halfedge()->twin()->next()->next()->vertex()->position;

		e->newPosition = (3.0*(pos0 + pos1) + pos2 + pos3) / 8.0;
		e->isNew = false;
	}

	// Next, we're going to split every edge in the mesh, in any order.  For
	// future reference, we're also going to store some information about which
	// subdivided edges come from splitting an edge in the original mesh, and
	// which edges are new. In this loop, we only want to iterate over edges of
	// the original mesh---otherwise, we'll end up splitting edges that we just split
	// (and the loop will never end!)

	// iterate over all edges in the mesh
	int n = mesh.nEdges();
	EdgeIter e = mesh.edgesBegin();
	for (int i = 0; i < n; i++) {
		// get the next edge NOW!
		EdgeIter nextEdge = e;
		nextEdge++;

		VertexIter v = mesh.splitEdge(e);
		v->isNew = true;
		v->newPosition = e->newPosition;

		HalfedgeIter he = e->halfedge()->vertex() == v ? e->halfedge() : e->halfedge()->twin();
		he->pre()->edge()->isNew = true;
		he->twin()->next()->edge()->isNew = true;

		// ...we still have a valid reference to the next edge.
		e = nextEdge;
	}

	// Finally, flip any new edge that connects an old and new vertex.
	for (auto e = mesh.edgesBegin(); e != mesh.edgesEnd(); e++) {
		if (!e->isNew)
			continue;

		if (e->halfedge()->vertex()->isNew + e->halfedge()->twin()->vertex()->isNew != 1)
			continue;

		mesh.flipEdge(e);
	}

	// Copy the updated vertex positions to the subdivided mesh.
	for (auto v = mesh.verticesBegin(); v != mesh.verticesEnd(); v++)
		v->position = v->newPosition;
}

void MeshResampler::downsample(HalfedgeMesh& mesh) {
	// Compute initial quadrics for each face by simply writing the plane equation
	// for the face in homogeneous coordinates. These quadrics should be stored
	// in Face::quadric
	for (auto f = mesh.facesBegin(); f != mesh.facesEnd(); f++) {
		double d = -dot(f->normal(), f->halfedge()->vertex()->position);
		Vector4D v = Vector4D(f->normal(), d);
		f->quadric = outer(v, v);
	}

	// -> Compute an initial quadric for each vertex as the sum of the quadrics
	//    associated with the incident faces, storing it in Vertex::quadric
	for (auto v = mesh.verticesBegin(); v != mesh.verticesEnd(); v++) {
		auto adjFs = v->AdjFaces();
		v->quadric.zero();
		for (auto f : adjFs)
			v->quadric += f->quadric;
	}

	// -> Build a priority queue of edges according to their quadric error cost,
	//    i.e., by building an EdgeRecord for each edge and sticking it in the
	//    queue.
	MutablePriorityQueue<EdgeRecord> queue;
	for (auto e = mesh.edgesBegin(); e != mesh.edgesEnd(); e++) {
		e->record = EdgeRecord(e);
		queue.insert(e->record);
	}

	// -> Until we reach the target edge budget, collapse the best edge. Remember
	//    to remove from the queue any edge that touches the collapsing edge
	//    BEFORE it gets collapsed, and add back into the queue any edge touching
	//    the collapsed vertex AFTER it's been collapsed. Also remember to assign
	//    a quadric to the collapsed vertex, and to pop the collapsed edge off the
	//    top of the queue.
	size_t targetNum = mesh.nFaces() / 4;
	while (mesh.nFaces() > targetNum) {
		EdgeRecord eR = queue.top();
		queue.pop();
		
		{//remove adjEs' record in queue
			auto adjEs = eR.edge->AdjEdges();
			for (auto adjE : adjEs)
				queue.remove(adjE->record);
		}

		VertexIter newV = mesh.collapseEdge(eR.edge);
		if (!mesh.IsValid(newV, "downsample : collapse an edge fail"))
			return;

		newV->position = eR.optimalPoint;

		{// set adjFs' and newV's quadric
			newV->quadric.zero();
			auto adjFs = newV->AdjFaces();
			for (auto adjF : adjFs) {
				double d = -dot(adjF->normal(), adjF->halfedge()->vertex()->position);
				Vector4D v = Vector4D(adjF->normal(), d);
				adjF->quadric = outer(v, v);
				newV->quadric += adjF->quadric;
			}
		}

		{// set adjVs' quadric
			auto adjVs = newV->AdjVertices();
			for (auto adjV : adjVs) {
				adjV->quadric.zero();
				auto adjFs = adjV->AdjFaces();
				for (auto adjF : adjFs)
					adjV->quadric += adjF->quadric;
			}
		}
		
		{// set adjEs' record
			auto adjEs = newV->AdjEdges();
			for (auto adjE : adjEs) {
				adjE->record = EdgeRecord(adjE);
				queue.insert(adjE->record);
			}
		}
	}
}

void MeshResampler::resample(HalfedgeMesh& mesh) {
	// Compute the mean edge length.
	double meanLen = 0.0;
	for (auto e = mesh.edgesBegin(); e != mesh.edgesEnd(); e++)
		meanLen += e->length();

	meanLen /= mesh.nEdges();

	// Repeat the four main steps for 5 or 6 iterations
	for (size_t i = 0; i < 10; i++) {
		printf("INFO::resample : loop %d\n", i);
		// -> Split edges much longer than the target length (being careful about
		//    how the loop is written!)
		size_t n = mesh.nEdges();
		EdgeIter e = mesh.edgesBegin();
		while (n-- > 0) {
			EdgeIter nextE = e;
			nextE++;

			if (e->length() > 4.0 / 3.0*meanLen)
				mesh.splitEdge(e);

			e = nextE;
		}

		for (auto f = mesh.facesBegin(); f != mesh.facesEnd(); f++) {
			if (f->degree() != 3) {
				printf("ERROR::resample : face degree is not 3 after split edge, it is %d\n", f->degree());
				showError("face degree is not 3", true);
			}
		}

		// -> Collapse edges much shorter than the target length.  Here we need to
		//    be EXTRA careful about advancing the loop, because many edges may have
		//    been destroyed by a collapse (which ones?)
		set<EdgeIter> edges;
		for (auto e = mesh.edgesBegin(); e != mesh.edgesEnd(); e++)
			edges.insert(e);

		while (edges.size() > 0) {
			EdgeIter e = *edges.begin();
			edges.erase(e);
			if (e->length() < 0.8*meanLen) {
				auto adjEs = e->AdjEdges();
				for (auto adjE : adjEs)
					edges.erase(adjE);

				VertexIter v =  mesh.collapseEdge(e);
				if (!mesh.IsValid(v, "resample : collapse edge fail"))
					return;
			}
		}

		for (auto f = mesh.facesBegin(); f != mesh.facesEnd(); f++) {
			if (f->degree() != 3) {
				printf("ERROR::resample : face degree is not 3 after collapse edge, it is %d\n", f->degree());
				showError("face degree is not 3", true);
			}
		}

		// -> Now flip each edge if it improves vertex degree
		for (auto e = mesh.edgesBegin(); e != mesh.edgesEnd(); e++) {
			int v0d = e->halfedge()->vertex()->degree();
			int v1d = e->halfedge()->twin()->vertex()->degree();
			int v2d = e->halfedge()->next()->next()->vertex()->degree();
			int v3d = e->halfedge()->twin()->next()->next()->vertex()->degree();

			int cost = abs(v0d - 6) + abs(v1d - 6) + abs(v2d - 6) + abs(v3d - 6);
			int flipCost = abs(v0d - 1 - 6) + abs(v1d - 1 - 6) + abs(v2d + 1 - 6) + abs(v3d + 1 - 6);
			if (flipCost < cost)
				mesh.flipEdge(e);
		}

		for (auto f = mesh.facesBegin(); f != mesh.facesEnd(); f++) {
			if (f->degree() != 3) {
				printf("ERROR::resample : face degree is not 3 after flip edge, it is %d\n", f->degree());
				showError("face degree is not 3", true);
			}
		}

		// -> Finally, apply some tangential smoothing to the vertex 
		for (auto v = mesh.verticesBegin(); v != mesh.verticesEnd(); v++) {
			Vector3D offset = v->neighborhoodCentroid() - v->position;
			Vector3D norm = v->normal();
			Vector3D tangentOffset = offset - dot(offset, norm)*norm;
			v->newPosition = v->position + 0.1 * tangentOffset;
		}

		for (auto v = mesh.verticesBegin(); v != mesh.verticesEnd(); v++)
			v->position = v->newPosition;
	}
}
