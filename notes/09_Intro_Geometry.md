# 09 Introduction to Geometry

**Digitally encode geometry**

- EXPLICIT

  > make some tasks easy (like sampling), make other tasks hard (like inside/outside tests)

  - point cloud
  - triangle meshes 
  - polygon mesh
  - subdivision, NURBS
  - ...

- IMPLICIT

  > make some tasks hard (like sampling), but make other tasks easy (like inside/outside tests)

  - level set
  - algebraic surface
  - L-systems
  - ...

Each choice best suited to a different task/type of geometry 

## 9.1 Implicit

**Algebraic Surfaces **

Surface is zero set of a polynomial in x, y, z (“algebraic variety”) 

> 喵喵喵? zero set?

![1544712735907](assets/1544712735907.png)

**Constructive Solid Geometry (Implicit) **

Build more complicated shapes via ==Boolean operations==

Basic operations: 

![1544712802743](assets/1544712802743.png)

**Blobby Surfaces **

Instead of Booleans, gradually blend surfaces together 

![1544712875326](assets/1544712875326.png)

Easier to understand in 2D: 

![1544712989474](assets/1544712989474.png)

**Blending Distance Functions **

A distance function gives distance to closest point on object 

Can blend any two distance functions d1, d2: 

![1544713264246](assets/1544713264246.png)

**Level Set Methods **

hard to describe complex shapes in closed form 

Alternative: store a grid of values approximating function 

Surface is found where interpolated values equal zero 

Provides much more explicit control over shape (like a texture) 

Often demands sophisticated fltering (trilinear, tricubic…) 

![1544713374081](assets/1544713374081.png)

Drawback: storage for 2D surface is now O(n3)

Can reduce cost by storing only a narrow band around surface 

**Fractals **

No precise defnition; exhibit self-similarity, detail at all scales 

New “language” for describing natural phenomena 

Hard to control shape! 

![1544713523128](assets/1544713523128.png)

**Implicit Representations - Pros & Cons **

Pros:
- description can be very compact (e.g., a polynomial)
- easy to determine if a point is in our shape (just plug it in!)
- other queries may also be easy (e.g., distance to surface)
- for simple shapes, exact description/no sampling error
- easy to handle changes in topology (e.g., fuid)

Cons:
- expensive to fnd all points in the shape (e.g., for drawing)
- very difficult to model complex shapes 

## 9.2 Explicit

**Point Cloud**

- Easiest representation: list of points (x,y,z)
- Often augmented with normals
- Easily represent any kind of geometry
- Useful for LARGE datasets (>>1 point/pixel)
- Hard to interpolate undersampled regions
- Hard to do processing / simulation / … 

**Polygon Mesh**

- Store vertices and polygons (most often triangles or quads) 
- Easier to do processing/simulation, adaptive sampling
- More complicated data structures
- Perhaps most common representation in graphics 

**Triangle Mesh **

- Store vertices as triples of coordinates (x,y,z) 

- Store triangles as triples of indices (i,j,k) 
- Use barycentric interpolation to defne points inside triangles

**Bézier Curves**

> **Bernstein Basis **
>
> $B_k^n(x)=C_n^kx^k(1-x)^{n-k}$

A Bézier curve is a curve expressed in the Bernstein basis: 
$$
\gamma(s)=\sum_{k=0}^nB_k^n(s)P_k
$$

1. For n=1, just get a line segment 
2. For n=3, get “cubic Bézier” 
3. interpolates endpoints
4. tangent to end segments
5. contained in convex hull (nice for rasterization) 

**Piecewise Bézier Curves  **

Widely-used technique (Illustrator, fonts, SVG, etc.) 

Formally, piecewise Bézier curve: 
$$
\gamma(u)=\gamma_i(\frac{u-u_i}{u_{i+1}-u_i}), u_i\le u\le u_{i+1}
$$
**Tensor Product**

Can use a pair of curves to get a surface 

Value at any point (u,v) given by product of a curve f at u and a curve g at v (sometimes called the “tensor product”): 

![1544714913562](assets/1544714913562.png)

**Bézier Patches**

Bézier patch is sum of (tensor) products of Bernstein bases 

![1544715559369](assets/1544715559369.png)

**Bézier Surface **

Just as we connected Bézier curves, can connect Bézier patches to get a surface 

Very easy to draw: just dice each patch into regular (u,v) grid! 

![1544715841888](assets/1544715841888.png)

**Subdivision  **

- Start with control curve
- Insert new vertex at each edge midpoint
- Update vertex positions according to fxed rule
- For careful choice of averaging rule, yields smooth curve 

![1544715978975](assets/1544715978975.png)

**Subdivision Surfaces **

- Start with coarse polygon mesh (“control cage”) 

- Subdivide each element 

- Update vertices via local averaging 

- Many possible rule: 

  - Catmull-Clark (quads)

  - Loop (triangles) 
  - ...