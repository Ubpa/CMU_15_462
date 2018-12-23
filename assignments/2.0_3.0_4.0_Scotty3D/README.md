# CMU 15-462 - Scotty3D 

Welcome to Scotty3D! This 3D graphics software implements interactive mesh
editing, realistic path tracing, and dynamic animation. Implementing the
functionality of the program constitutes the majority of the coursework for
15-462/662 Computer Graphics at Carnegie Mellon University.

Further information is available in the User Guide and Developer Manual, both
available on the project wiki accessible via tab at the top of the Github.com UI.

## 任务简述 

[User Guide - Mesh Edit](https://github.com/cmu462/Scotty3D/wiki/User-Guide-(MeshEdit)) 

[Developer Manual - Local Mesh Operations](https://github.com/cmu462/Scotty3D/wiki/Local-Mesh-Operations) 

## Notes 

### 1. Local 

#### 1.1 Spilt Edge 

- Not Boundary

![1545485889782](assets/1545485889782.jpg)

- Boundary

![1545492608038](assets/1545492608038.jpg)

#### 1.2 Collapse Edge 

![1545504132406](assets/1545504132406.jpg)

#### 1.3 Erase Edge

- normal

![1545538991552](assets/1545538991552.jpg)

- degeneration

![1545543042515](assets/1545543042515.jpg)

- boundary

![1545547201892](assets/1545547201892.jpg)

#### 1.4 Collaps Face

用 Collaps Edge 实现 Collaps Face

#### 1.5 Erase Vertex

用 Erase Edge 实现 Erase Vertex