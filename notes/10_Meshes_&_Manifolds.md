# 10 Meshes and Manifolds

## 10.1 Manifolds

**Smooth Surfaces **

Intuitively, a surface is the boundary or “shell” of an object 

Surfaces are manifold:

- If you zoom in far enough (at any point) looks like a plane* 

> *…or can easily be fattened into the plane, without cutting or ripping 
>
> ![1544716983951](assets/1544716983951.png)

not every shape is manifold, for instance

![1544716911212](assets/1544716911212.png)

**manifold polygon mesh **

A manifold polygon mesh has fans, not fins (鱼翅)

For polygonal surfaces just two easy conditions to check: 

- Every edge is contained in only two polygons (no “fins”) 

- The polygons containing the same vertex make a single “fan” 

  ![1544717219704](assets/1544717219704.png)


**Halfedge Data Structure**

- Store some information about neighbors 

- Don’t need an exhaustive list; just a few key pointers 

- Key idea: two halfedges act as “glue” between mesh elements: 

![1544717656289](assets/1544717656289.png)

- Each vertex, edge and face points to just one of its halfedges 

- Halfedge makes mesh traversal easy 

  - Use “twin” and “next” pointers to move around mesh 
  - Use “vertex”, “edge”, and “face” pointers to grab element 

  > Note: only makes sense if mesh is manifold! 

- Halfedge meshes are always manifold 

  > Require only “common-sense” conditions
  >
  > ```c++
  > twin->twin == this;
  > next != this;
  > twin != this;
  > ```

