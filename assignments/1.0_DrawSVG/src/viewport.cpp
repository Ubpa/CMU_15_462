#include "viewport.h"

#include "CMU462.h"

namespace CMU462 {

void ViewportImp::set_viewbox( float x, float y, float span ) {

  // Task 5 (part 2): 
  // Set svg to normalized device coordinate transformation. Your input
  // arguments are defined as SVG canvans coordinates.
  
	this->x = x;
	this->y = y;
	this->span = span;
	CMU462::Matrix3x3 m[4];
	for(size_t i=0;i<4;i++)
		m[i] = CMU462::Matrix3x3::identity();

	m[0][2] = CMU462::Vector3D(-x, -y, 1);
	m[1][0][0] = 1 / span;
	m[1][1][1] = 1 / span;
	m[2][2] = CMU462::Vector3D(1, 1, 1);
	m[3][0][0] = 0.5;
	m[3][1][1] = 0.5;
	svg_2_norm = m[3] * m[2] * m[1] * m[0];
}

void ViewportImp::update_viewbox( float dx, float dy, float scale ) { 
	set_viewbox(x - dx, y - dy, span*scale);
}

} // namespace CMU462