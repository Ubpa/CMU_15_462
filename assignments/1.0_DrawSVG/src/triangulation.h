#ifndef CMU462_TRIANGULATION_H
#define CMU462_TRIANGULATION_H

#include "svg.h"

namespace CMU462 {

// triangulates a polygon and save the result as a triangle list
void triangulate(const Polygon& polygon, std::vector<Vector2D>& triangles );

} // namespace CMU462

#endif // CMU462_TRIANGULATION_H

