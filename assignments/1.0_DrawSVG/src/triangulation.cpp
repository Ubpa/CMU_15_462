#include "triangulation.h"

#include <vector>

using namespace std;

static const float EPSILON = 0.0000000001f;

namespace CMU462 {

bool inside(float Ax, float Ay,
            float Bx, float By,
            float Cx, float Cy,
            float Px, float Py) {

  float ax, ay, bx, by, cx, cy, apx, apy, bpx, bpy, cpx, cpy;
  float cCROSSap, bCROSScp, aCROSSbp;

  ax = Cx - Bx;  ay = Cy - By;
  bx = Ax - Cx;  by = Ay - Cy;
  cx = Bx - Ax;  cy = By - Ay;
  apx= Px - Ax;  apy= Py - Ay;
  bpx= Px - Bx;  bpy= Py - By;
  cpx= Px - Cx;  cpy= Py - Cy;

  aCROSSbp = ax*bpy - ay*bpx;
  cCROSSap = cx*apy - cy*apx;
  bCROSScp = bx*cpy - by*cpx;

  return ((aCROSSbp >= 0.0f) && (bCROSScp >= 0.0f) && (cCROSSap >= 0.0f));
};


float area(const vector<Vector2D> &contour) {

  int n = contour.size();

  float a = 0.0f;
  for(int p=n-1,q=0; q<n; p=q++) {
    a += contour[p].x * contour[q].y - contour[q].x * contour[p].y;
  }

  return a * 0.5f;
}

bool snip(const vector<Vector2D>& contour,int u,int v,int w,int n,int *V ) {

  int p;
  float Ax, Ay, Bx, By, Cx, Cy, Px, Py;

  Ax = contour[V[u]].x;
  Ay = contour[V[u]].y;

  Bx = contour[V[v]].x;
  By = contour[V[v]].y;

  Cx = contour[V[w]].x;
  Cy = contour[V[w]].y;

  if ( EPSILON > (((Bx-Ax)*(Cy-Ay)) - ((By-Ay)*(Cx-Ax))) ) return false;

  for ( p = 0; p < n; p++ ) {
    if( (p == u) || (p == v) || (p == w) ) continue;
    Px = contour[V[p]].x;
    Py = contour[V[p]].y;
    if (inside(Ax,Ay,Bx,By,Cx,Cy,Px,Py)) return false;
  }

  return true;
}

void triangulate(const Polygon& polygon, vector<Vector2D>& triangles) {
  
  const vector<Vector2D>& contour = polygon.points;

  // allocate and initialize list of vertices in polygon
  int n = contour.size();
  if ( n < 3 ) return;

  vector<int> V(n);

  // we want a counter-clockwise polygon in V
  if ( 0.0f < area(contour) ) {
    for (int v=0; v<n; v++) V[v] = v;
  } else {
    for(int v=0; v<n; v++) V[v] = (n-1)-v;
  }
  
  int nv = n;

  // remove nv-2 Vertices, creating 1 triangle every time
  int count = 2*nv;   // error detection 

  for(int m = 0, v = nv - 1; nv > 2;) {

    // if we loop, it is probably a non-simple polygon
    if (0 >= (count--)) {
      // Triangulate: ERROR - probable bad polygon!
      return;
    }

    // three consecutive vertices in current polygon, <u,v,w>
    int u = v     ; if (nv <= u) u = 0;      // prev
    v = u + 1     ; if (nv <= v) v = 0;      // new v   
    int w = v + 1 ; if (nv <= w) w = 0;      // next    

    if ( snip(contour,u,v,w,nv,&V[0]) ) {

      int a,b,c,s,t;

      // true names of the vertices
      a = V[u]; b = V[v]; c = V[w];

      // output Triangle
      triangles.push_back( contour[a] );
      triangles.push_back( contour[b] );
      triangles.push_back( contour[c] );

      m++;

      // remove v from remaining polygon
      for( s = v, t = v + 1; t < nv; s++, t++) V[s] = V[t]; nv--;

      // resest error detection counter
      count = 2 * nv;
    }
  }
}

} // namespace CMU462
