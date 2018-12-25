#include <float.h>
#include <assert.h>
#include "meshEdit.h"
#include "mutablePriorityQueue.h"
#include "error_dialog.h"

#include <deque>

using namespace CMU462;

vector<HalfedgeIter> HalfedgeMesh::AllHalfedgesOf(VertexIter v) {
	// Collect all halfedges of v
	// the halfedges' vertex is v

	vector<HalfedgeIter> halfedges;

	HalfedgeIter he = v->halfedge();
	do {
		halfedges.push_back(he);
		he = he->twin()->next();
	} while (he->edge() != v->halfedge()->edge());

	return halfedges;
}

vector<EdgeIter> HalfedgeMesh::AllEdgesOf(VertexIter v) {
	// Collect all edges of v

	vector<EdgeIter> edges;

	HalfedgeIter he = v->halfedge();
	do {
		edges.push_back(he->edge());
		he = he->twin()->next();
	} while (he->edge() != v->halfedge()->edge());

	return edges;
}

vector<FaceIter> HalfedgeMesh::AllFacesOf(VertexIter v) {
	// Collect all faces of v

	vector<FaceIter> faces;

	HalfedgeIter he = v->halfedge();
	do {
		faces.push_back(he->face());
		he = he->twin()->next();
	} while (he->edge() != v->halfedge()->edge());

	return faces;
}

FaceIter HalfedgeMesh::InSameFace(VertexIter v0, VertexIter v1) {
	// if v0 and v1 are on a same inner face, return the face, otherwise return faces.end()
	// the inner face is not a boundary

	vector<FaceIter> facesOfv0 = AllFacesOf(v0);
	vector<FaceIter> facesOfv1 = AllFacesOf(v1);

	for (auto & f : facesOfv0) {
		auto target = find(facesOfv1.begin(), facesOfv1.end(), f);
		if (target != facesOfv1.end() && !(*target)->isBoundary())
			return *target;
	}

	return faces.end();
}

VertexIter HalfedgeMesh::InsertVertex(EdgeIter e) {
	// Insert a vertex on the middle of edge.

	VertexIter v0 = e->halfedge()->vertex();
	VertexIter v1 = e->halfedge()->twin()->vertex();
	HalfedgeIter he = e->halfedge();

	// pre backup
	HalfedgeIter hePre = he->pre();
	HalfedgeIter twin = he->twin();
	HalfedgeIter twinNext = he->twin()->next();
	Vector3D pos = he->edge()->centroid();

	VertexIter newV = newVertex();
	he->vertex() = newV;

	newV->position = pos;
	newV->halfedge() = he;

	HalfedgeIter he0 = newHalfedge();
	HalfedgeIter he1 = newHalfedge();

	he0->vertex() = v0;
	he1->vertex() = newV;

	EdgeIter newE = newEdge();
	newE->halfedge() = he0;
	he0->edge() = newE;
	he1->edge() = newE;

	he0->twin() = he1;
	he1->twin() = he0;

	he0->face() = he->face();
	he1->face() = twin->face();

	he0->next() = he;
	hePre->next() = he0;
	he1->next() = twinNext;
	twin->next() = he1;

	if (v0->halfedge() == he)
		v0->halfedge() = he0;

	return newV;
}

EdgeIter HalfedgeMesh::ConnectVertex(VertexIter v0, VertexIter v1) {
	// Connect two verties in a face.

	//  |                                        |
	// he0(¡ý)                                 (¡ü)he3
	//  |               new He0(->)              |
	//  v0  - - - - - - -  new E  - - - - - - -  v1
	//  |               new He1(<-)              |
	// he1(¡ý)                                 (¡ü)he2
	//  |                                        |

	FaceIter f = InSameFace(v0, v1);
	if (f == faces.end()) {
		showError("v0 and v1 are not in same face, connect failed");
		return edges.end();
	}

	vector<HalfedgeIter> halfedgesV0 = AllHalfedgesOf(v0);
	vector<HalfedgeIter> halfedgesV1 = AllHalfedgesOf(v1);

	HalfedgeIter he1, he3;
	for (auto he : halfedgesV0) {
		if (he->face() == f) {
			he1 = he;
			break;
		}
	}

	for (auto he : halfedgesV1) {
		if (he->face() == f) {
			he3 = he;
			break;
		}
	}

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
	he0->face()->halfedge() = newHe0;
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
	newHe0->face() = he0->face();
	newHe1->face() = he1->face();


	he0->next() = newHe0;
	newHe0->next() = he3;
	he2->next() = newHe1;
	newHe1->next() = he1;

	return newE;
}

VertexIter HalfedgeMesh::splitEdge(EdgeIter e) {
	// TODO: (meshEdit)
	// This method should split the given edge and return an iterator to the
	// newly inserted vertex. The halfedge of this vertex should point along
	// the edge that was split, rather than the new edges.

	// [Note:this method is for triangle meshes only!]
	if (e->halfedge()->face()->degree() != 3
		|| e->halfedge()->twin()->face()->degree() != 3) {
		showError("splitEdge is for triangle meshes only!");
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

		ConnectVertex(v0, v1);

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
	// --        |  |        --                --        |  |        --
	//   --      |  |      --                    --      |e0|      --
	//     --    |  |    --                        --    |  |    --
	//       --  |  |  --                            --  |  |  --
	//         --[vv]--                                --[vv]--


	HalfedgeIter he = e->halfedge()->isBoundary() ? e->halfedge() : e->halfedge()->twin();
	HalfedgeIter twin = he->twin();
	VertexIter v0 = he->next()->next()->vertex();
	VertexIter v1 = twin->next()->next()->vertex();

	VertexIter v2 = InsertVertex(e);
	ConnectVertex(v0, v2);
	ConnectVertex(v1, v2);

	return v2;


}

VertexIter HalfedgeMesh::collapseEdge(EdgeIter e) {
	// TODO: (meshEdit)
	// This method should collapse the given edge and return an iterator to
	// the new vertex created by the collapse.

	// The selected edge e is replaced by a single vertex v.
	// This vertex is connected by edges to all vertices previously connected to either endpoint of e. 
	// Moreover, if either of the polygons containing e was a triangle, it will be replaced by an edge
	// (rather than a degenerate polygon with only two edges)

	// the new vertex is the centroid of the edge

	VertexIter v0 = e->halfedge()->vertex();
	VertexIter v1 = e->halfedge()->twin()->vertex();

	// erase edge that make a triangle
	HalfedgeIter he = v1->halfedge();
	HalfedgeIter twinNext = he->twin()->next();
	int n = v1->degree() + (v1->isBoundary() ? 1 : 0);
	while (n-- > 0) {
		// e
		if (he->twin()->vertex() != v0) {
			// triangle
			if (he->next()->twin()->vertex() == v0 || he->twin()->pre()->vertex() == v0) {
				FaceIter f = eraseEdge(he->edge(), false);
				if (f->halfedge()->edge() == e) {
					if (e->halfedge()->next() == e->halfedge()->twin())
						f->halfedge() = e->halfedge()->twin()->next();
					else
						f->halfedge() = e->halfedge()->next();
				}
			}
		}

		he = twinNext;
		twinNext = he->twin()->next();
	}

	v0->position = e->centroid();

	if (v0->halfedge()->edge() == e)
		v0->halfedge() = e->halfedge()->twin()->next();

	he = v1->halfedge();
	twinNext = he->twin()->next();
	do {
		// e
		if (he->twin()->vertex() == v0)
			continue;

		he->vertex() = v0;

		if (he->pre() == e->halfedge()) {
			he->vertex() = v0;
			he->pre()->pre()->next() = he;
			he->face()->halfedge() = he;
		}

		if (he->twin()->next() == e->halfedge()->twin()) {
			HalfedgeIter twin = he->twin();
			twin->next() = twin->next()->next();
			twin->face()->halfedge() = twin;
		}
	} while (he = twinNext, twinNext = he->twin()->next(), he != v1->halfedge());

	if (e->halfedge()->next()->next() == e->halfedge()) {
		deleteVertex(v0);
		if (e->halfedge()->face()->isBoundary())
			deleteBoundary(e->halfedge()->face());
		else
			deleteFace(e->halfedge()->face());
		v0 = vertices.end();
	}
	else {
		if (e->halfedge()->next() == e->halfedge()->twin())
			e->halfedge()->pre()->next() = e->halfedge()->twin()->next();
		else if (e->halfedge()->twin()->next() == e->halfedge())
			e->halfedge()->twin()->pre()->next() = e->halfedge()->next();
	}

	deleteHalfedge(e->halfedge()->twin());
	deleteHalfedge(e->halfedge());
	deleteEdge(e);
	deleteVertex(v1);
	
	return v0;
}

VertexIter HalfedgeMesh::collapseFace(FaceIter f) {
	// TODO: (meshEdit)
	// This method should collapse the given face and return an iterator to
	// the new vertex created by the collapse.
	
	// The selected face f is replaced by a single vertex v.
	// All edges previously connected to vertices of f are now connected directly to v.

	Vector3D pos(0, 0, 0);
	HalfedgeIter he = f->halfedge();
	deque<EdgeIter> faceEdges;
	do {
		pos += he->vertex()->position;
		he->edge()->halfedge() = he;
		faceEdges.push_back(he->edge());
		he = he->next();
	} while (he != f->halfedge());
	pos *= 1.0 / faceEdges.size();

	while (faceEdges.size() > 3) {
		HalfedgeIter he = faceEdges.front()->halfedge();
		// triangle
		if (he->next() == he->twin()->next()->next()->twin()) {
			faceEdges.pop_front();
			faceEdges.insert(faceEdges.begin()+1, he->twin()->next()->edge());
			he->twin()->next()->edge()->halfedge() = he->twin()->next();
		}
		collapseEdge(he->edge());
		faceEdges.pop_front();
	}
	collapseEdge(faceEdges.front());
	faceEdges.pop_front();
	faceEdges.pop_front();
	VertexIter v = collapseEdge(faceEdges.front());
	if (v == vertices.end())
		return v;

	faceEdges.pop_front();

	v->position = pos;
	he = v->halfedge();
	int n = v->degree() + (v->isBoundary() ? 1 : 0);
	while (n-- > 0) {
		// degeneration
		if (he->next()->next() == he) {
			if (he->edge() != he->next()->edge()) {
				if (v->halfedge() == he)
					v->halfedge() = he->twin()->next();
				he->next()->edge()->halfedge() = he->next()->twin();
				he->twin()->edge() = he->next()->edge();
				he->twin()->twin() = he->next()->twin();
				he->next()->twin()->twin() = he->twin();

				HalfedgeIter next = he->twin()->next();
				deleteFace(he->face());
				deleteHalfedge(he->next());
				deleteEdge(he->edge());
				deleteHalfedge(he);
				he = next;
			}
			else {
				deleteVertex(he->twin()->vertex());
				deleteVertex(he->vertex());
				deleteFace(he->face());
				deleteEdge(he->edge());
				deleteHalfedge(he->twin());
				deleteHalfedge(he);
				v = vertices.end();
			}
		}
		else
			he = he->twin()->next();
	}
	
	return v;
}

FaceIter HalfedgeMesh::eraseVertex(VertexIter v) {
	// This method should replace the given vertex and all its neighboring
	// edges and faces with a single face, returning the new face.

	if (v->isBoundary()) {
		showError("Can't erase a boundary vertex");
		return faces.end();
	}

	vector<EdgeIter> edges = AllEdgesOf(v);
	FaceIter f;
	for (auto & edge : edges)
		f = eraseEdge(edge);

	// no need to erase vertex
	// it will be erased by the final call of eraseEdge

	return f;
}

FaceIter HalfedgeMesh::eraseEdge(EdgeIter e, bool delSingleLine) {
	// TODO: (meshEdit)
	// This method should erase the given edge and return an iterator to the
	// merged face.

	// The selected edge e will be replaced with the union of the faces containing it
	// producing a new face e.(if e is a boundary edge, nothing happens).

	if (e->isBoundary()) {
		showError("Can't erase a boundary edge");
		return faces.end();
	}

	// same face
	if (e->halfedge()->face() == e->halfedge()->twin()->face()) {
		if (e->halfedge()->next() != e->halfedge()->twin()
			&& e->halfedge()->twin()->next() != e->halfedge()) {
			showError("Can't erase a strange case edge");
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

EdgeIter HalfedgeMesh::flipEdge(EdgeIter e0) {
	// This method should flip the given edge and return an iterator to the
	// flipped edge.

	// The selected edge e is "rotated" around the face, 
	// in the sense that each endpoint moves to the next vertex (in counter-clockwise order)
	// along the boundary of the two polygons containing e.
	if (e0->isBoundary()) {
		showError("Can't flip boundary edge");
		return e0;
	}

	HalfedgeIter he = e0->halfedge();
	HalfedgeIter twin = he->twin();

	// 1. vertex
	if (he->vertex()->halfedge() == he)
		he->vertex()->halfedge() = twin->next();
	if (twin->vertex()->halfedge() == twin)
		twin->vertex()->halfedge() = he->next();
	
	// 2. face
	if (he->face()->halfedge() == he->next())
		he->face()->halfedge() = he;
	if (twin->face()->halfedge() == twin->next())
		twin->face()->halfedge() = twin;

	// 3. edge
	// no changes

	// 4. halfedge
	// 4.1 halfedge->face
	he->next()->face() = twin->face();
	twin->next()->face() = he->face();

	// 4.2 halfedge->vertex
	he->vertex() = twin->next()->next()->vertex();
	twin->vertex() = he->next()->next()->vertex();

	// 4.3 halfedge->next
	HalfedgeIter heNext = he->next();
	HalfedgeIter heNext2= heNext->next();
	HalfedgeIter hePre = he->pre();
	HalfedgeIter twinNext = twin->next();
	HalfedgeIter twinNext2 = twinNext->next();
	HalfedgeIter twinPre = twin->pre();

	hePre->next() = twinNext;
	heNext->next() = twin;
	he->next() = heNext2;
	twinPre->next() = heNext;
	twinNext->next() = he;
	twin->next() = twinNext2;

	// 4.4 halfedge->twin and edge
	// no changes

	return e0;
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

  // TODO Step I: Compute the vertex positions for the subdivided mesh.  Here
  // we're
  // going to do something a little bit strange: since we will have one vertex
  // in
  // the subdivided mesh for each vertex, edge, and face in the original mesh,
  // we
  // can nicely store the new vertex *positions* as attributes on vertices,
  // edges,
  // and faces of the original mesh.  These positions can then be conveniently
  // copied into the new, subdivided mesh.
  // [See subroutines for actual "TODO"s]
  if (useCatmullClark) {
    computeCatmullClarkPositions();
  } else {
    computeLinearSubdivisionPositions();
  }

  // TODO Step II: Assign a unique index (starting at 0) to each vertex, edge,
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
  // [See subroutine for actual "TODO"s]
  assignSubdivisionIndices();

  // TODO Step III: Build a list of quads in the new (subdivided) mesh, as
  // tuples of
  // the element indices defined above.  In other words, each new quad should be
  // of
  // the form (i,j,k,l), where i,j,k and l are four of the indices stored on our
  // original mesh elements.  Note that it is essential to get the orientation
  // right
  // here: (i,j,k,l) is not the same as (l,k,j,i).  Indices of new faces should
  // circulate in the same direction as old faces (think about the right-hand
  // rule).
  // [See subroutines for actual "TODO"s]
  vector<vector<Index> > subDFaces;
  vector<Vector3D> subDVertices;
  buildSubdivisionFaceList(subDFaces);
  buildSubdivisionVertexList(subDVertices);

  // TODO Step IV: Pass the list of vertices and quads to a routine that clears
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
  // TODO For each vertex, assign Vertex::newPosition to
  // its original position, Vertex::position.

  // TODO For each edge, assign the midpoint of the two original
  // positions to Edge::newPosition.

  // TODO For each face, assign the centroid (i.e., arithmetic mean)
  // of the original vertex positions to Face::newPosition.  Note
  // that in general, NOT all faces will be triangles!
  showError("computeLinearSubdivisionPositions() not implemented.");
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
  // TODO The implementation for this routine should be
  // a lot like HalfedgeMesh::computeLinearSubdivisionPositions(),
  // except that the calculation of the positions themsevles is
  // slightly more involved, using the Catmull-Clark subdivision
  // rules. (These rules are outlined in the Developer Manual.)

  // TODO face

  // TODO edges

  // TODO vertices
  showError("computeCatmullClarkPositions() not implemented.");
}

/**
 * Assign a unique integer index to each vertex, edge, and face in
 * the mesh, starting at 0 and incrementing by 1 for each element.
 * These indices will be used as the vertex indices for a mesh
 * subdivided using Catmull-Clark (or linear) subdivision.
 */
void HalfedgeMesh::assignSubdivisionIndices() {
  // TODO Start a counter at zero; if you like, you can use the
  // "Index" type (defined in halfedgeMesh.h)

  // TODO Iterate over vertices, assigning values to Vertex::index

  // TODO Iterate over edges, assigning values to Edge::index

  // TODO Iterate over faces, assigning values to Face::index
  showError("assignSubdivisionIndices() not implemented.");
}

/**
 * Build a flat list containing all the vertex positions for a
 * Catmull-Clark (or linear) subdivison of this mesh.  The order of
 * vertex positions in this list must be identical to the order
 * of indices assigned to Vertex::newPosition, Edge::newPosition,
 * and Face::newPosition.
 */
void HalfedgeMesh::buildSubdivisionVertexList(vector<Vector3D>& subDVertices) {
  // TODO Resize the vertex list so that it can hold all the vertices.

  // TODO Iterate over vertices, assigning Vertex::newPosition to the
  // appropriate location in the new vertex list.

  // TODO Iterate over edges, assigning Edge::newPosition to the appropriate
  // location in the new vertex list.

  // TODO Iterate over faces, assigning Face::newPosition to the appropriate
  // location in the new vertex list.
  showError("buildSubdivisionVertexList() not implemented.");
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
  // TODO This routine is perhaps the most tricky step in the construction of
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

  // TODO iterate over faces
  // TODO loop around face
  // TODO build lists of four indices for each sub-quad
  // TODO append each list of four indices to face list
  showError("buildSubdivisionFaceList() not implemented.");
}

FaceIter HalfedgeMesh::bevelVertex(VertexIter v) {
	// TODO This method should replace the vertex v with a face, corresponding to
	// a bevel operation. It should return the new face.  NOTE: This method is
	// responsible for updating the *connectivity* of the mesh only---it does not
	// need to update the vertex positions.  These positions will be updated in
	// HalfedgeMesh::bevelVertexComputeNewPositions (which you also have to
	// implement!)


	// 1. collect edge
	vector<EdgeIter> edges = AllEdgesOf(v);

	// 2. insert V
	vector<VertexIter> newV;
	for (size_t i = 0; i < edges.size(); i++)
		newV.push_back(InsertVertex(edges[i]));
	
	// 3. connect V
	for (size_t i = 0; i < newV.size(); i++) {
		VertexIter v0 = newV[i];
		VertexIter v1 = newV[(i + 1) % newV.size()];
		ConnectVertex(v0, v1);
	}
	
	return eraseVertex(v);
}

FaceIter HalfedgeMesh::bevelEdge(EdgeIter e) {
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
		ConnectVertex(v0, v1);
	}
	
	VertexIter v0 = e->halfedge()->vertex();
	VertexIter v1 = e->halfedge()->twin()->vertex();
	eraseVertex(v0);
	return eraseVertex(v1);
}

FaceIter HalfedgeMesh::bevelFace(FaceIter f) {
  // TODO This method should replace the face f with an additional, inset face
  // (and ring of faces around it), corresponding to a bevel operation. It
  // should return the new face.  NOTE: This method is responsible for updating
  // the *connectivity* of the mesh only---it does not need to update the vertex
  // positions.  These positions will be updated in
  // HalfedgeMesh::bevelFaceComputeNewPositions (which you also have to
  // implement!)

  showError("bevelFace() not implemented.");
  return facesBegin();
}


void HalfedgeMesh::bevelFaceComputeNewPositions(
    vector<Vector3D>& originalVertexPositions,
    vector<HalfedgeIter>& newHalfedges, double normalShift,
    double tangentialInset) {
  // TODO Compute new vertex positions for the vertices of the beveled face.
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

}

void HalfedgeMesh::bevelVertexComputeNewPositions(
	Vector3D originalVertexPosition, vector<HalfedgeIter>& newHalfedges,
	double tangentialInset) {
	// TODO Compute new vertex positions for the vertices of the beveled vertex.
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
  // TODO Compute new vertex positions for the vertices of the beveled edge.
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

}

void HalfedgeMesh::splitPolygons(vector<FaceIter>& fcs) {
  for (auto f : fcs) splitPolygon(f);
}

void HalfedgeMesh::splitPolygon(FaceIter f) {
  // TODO: (meshedit) 
  // Triangulate a polygonal face
  showError("splitPolygon() not implemented.");
}

EdgeRecord::EdgeRecord(EdgeIter& _edge) : edge(_edge) {
  // TODO: (meshEdit)
  // Compute the combined quadric from the edge endpoints.
  // -> Build the 3x3 linear system whose solution minimizes the quadric error
  //    associated with these two endpoints.
  // -> Use this system to solve for the optimal position, and store it in
  //    EdgeRecord::optimalPoint.
  // -> Also store the cost associated with collapsing this edg in
  //    EdgeRecord::Cost.
}

void MeshResampler::upsample(HalfedgeMesh& mesh)
// This routine should increase the number of triangles in the mesh using Loop
// subdivision.
{
  // TODO: (meshEdit)
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

  // Next, compute the updated vertex positions associated with edges.

  // Next, we're going to split every edge in the mesh, in any order.  For
  // future
  // reference, we're also going to store some information about which
  // subdivided
  // edges come from splitting an edge in the original mesh, and which edges are
  // new.
  // In this loop, we only want to iterate over edges of the original
  // mesh---otherwise,
  // we'll end up splitting edges that we just split (and the loop will never
  // end!)

  // Finally, flip any new edge that connects an old and new vertex.

  // Copy the updated vertex positions to the subdivided mesh.
  showError("upsample() not implemented.");
}

void MeshResampler::downsample(HalfedgeMesh& mesh) {
  // TODO: (meshEdit)
  // Compute initial quadrics for each face by simply writing the plane equation
  // for the face in homogeneous coordinates. These quadrics should be stored
  // in Face::quadric
  // -> Compute an initial quadric for each vertex as the sum of the quadrics
  //    associated with the incident faces, storing it in Vertex::quadric
  // -> Build a priority queue of edges according to their quadric error cost,
  //    i.e., by building an EdgeRecord for each edge and sticking it in the
  //    queue.
  // -> Until we reach the target edge budget, collapse the best edge. Remember
  //    to remove from the queue any edge that touches the collapsing edge
  //    BEFORE it gets collapsed, and add back into the queue any edge touching
  //    the collapsed vertex AFTER it's been collapsed. Also remember to assign
  //    a quadric to the collapsed vertex, and to pop the collapsed edge off the
  //    top of the queue.
  showError("downsample() not implemented.");
}

void MeshResampler::resample(HalfedgeMesh& mesh) {
  // TODO: (meshEdit)
  // Compute the mean edge length.
  // Repeat the four main steps for 5 or 6 iterations
  // -> Split edges much longer than the target length (being careful about
  //    how the loop is written!)
  // -> Collapse edges much shorter than the target length.  Here we need to
  //    be EXTRA careful about advancing the loop, because many edges may have
  //    been destroyed by a collapse (which ones?)
  // -> Now flip each edge if it improves vertex degree
  // -> Finally, apply some tangential smoothing to the vertex positions
  showError("resample() not implemented.");
}
