#include "software_renderer.h"

#include <cmath>
#include <vector>
#include <iostream>
#include <algorithm>

#include "triangulation.h"

using namespace std;

namespace CMU462 {


// Implements SoftwareRenderer //

void SoftwareRendererImp::draw_svg( SVG& svg ) {

  // set top level transformation
  transformation = screen_to_buffer * canvas_to_screen;
  
  // change rendertarget
  unsigned char * render_target_backup = nullptr;
  if (sample_rate > 1) {
	render_target_backup = render_target;
	render_target = supersample_render_target;
	target_w *= sample_rate;
	target_h *= sample_rate;
	for (size_t i = 0; i < target_h; i++) {
	  for (size_t j = 0; j < target_w; j++) {
		render_target[(i*target_w + j) * 4 + 0] = 255;
		render_target[(i*target_w + j) * 4 + 1] = 255;
		render_target[(i*target_w + j) * 4 + 2] = 255;
		render_target[(i*target_w + j) * 4 + 3] = 255;
	  }
	}
  }

  // draw all elements
  //printf("---------------------------------------\n");
  for ( size_t i = 0; i < svg.elements.size(); ++i ) {
    draw_element(svg.elements[i]);
  }

  // draw canvas outline
  Vector2D a = transform(Vector2D(    0    ,     0    )); a.x--; a.y--;
  Vector2D b = transform(Vector2D(svg.width,     0    )); b.x++; b.y--;
  Vector2D c = transform(Vector2D(    0    ,svg.height)); c.x--; c.y++;
  Vector2D d = transform(Vector2D(svg.width,svg.height)); d.x++; d.y++;

  rasterize_line(a.x, a.y, b.x, b.y, Color::Black);
  rasterize_line(a.x, a.y, c.x, c.y, Color::Black);
  rasterize_line(d.x, d.y, b.x, b.y, Color::Black);
  rasterize_line(d.x, d.y, c.x, c.y, Color::Black);

  // resolve and send to render target
  if (sample_rate > 1) {
    target_w /= sample_rate;
    target_h /= sample_rate;
    render_target = render_target_backup;
    resolve();
  }
}

void SoftwareRendererImp::set_sample_rate(size_t sample_rate) {

  // Task 4: 
  // You may want to modify this for supersampling support
	if (this->sample_rate == sample_rate)
		return;

	this->sample_rate = sample_rate;
	screen_to_buffer = CMU462::Matrix3x3::identity();
	screen_to_buffer(0, 0) = sample_rate;
	screen_to_buffer(1, 1) = sample_rate;
	delete[] supersample_render_target;
	supersample_render_target = nullptr;
	if (sample_rate != 1) {
		supersample_render_target = new unsigned char[target_w*target_h*sample_rate*sample_rate * 4];
	}
}

void SoftwareRendererImp::set_render_target( unsigned char* render_target,
                                             size_t width, size_t height ) {

  // Task 4: 
  // You may want to modify this for supersampling support
  this->render_target = render_target;
  this->target_w = width;
  this->target_h = height;

  if (sample_rate != 1) {
	  delete[] supersample_render_target;
	  supersample_render_target = new unsigned char[target_w*target_h*sample_rate*sample_rate * 4];
  }
}

void SoftwareRendererImp::draw_element(SVGElement* element) {

	// Task 5 (part 1):
	// Modify this to implement the transformation stack
	//static size_t depth = 0;
	//depth++;
	//for (size_t i = 1; i < depth; i++)
	//	printf("  ");
	transformation = transformation * element->transform;
	switch (element->type) {
	case POINT:
		//printf("Point\n");
		draw_point(static_cast<Point&>(*element));
		break;
	case LINE:
		//printf("Line\n");
		draw_line(static_cast<Line&>(*element));
		break;
	case POLYLINE:
		//printf("Polyline\n");
		draw_polyline(static_cast<Polyline&>(*element));
		break;
	case RECT:
		//printf("Rect\n");
		draw_rect(static_cast<Rect&>(*element));
		break;
	case POLYGON:
		//printf("Polygon\n");
		draw_polygon(static_cast<Polygon&>(*element));
		break;
	case ELLIPSE:
		//printf("Ellipse\n");
		draw_ellipse(static_cast<Ellipse&>(*element));
		break;
	case IMAGE:
		//printf("Image\n");
		draw_image(static_cast<Image&>(*element));
		break;
	case GROUP:
		//printf("Group\n");
		draw_group(static_cast<Group&>(*element));
		break;
	default:
		break;
	}
	//depth--;
	transformation = transformation * element->transform.inv();
}


// Primitive Drawing //

void SoftwareRendererImp::draw_point( Point& point ) {

  Vector2D p = transform(point.position);
  rasterize_point( p.x, p.y, point.style.fillColor );

}

void SoftwareRendererImp::draw_line( Line& line ) { 

  Vector2D p0 = transform(line.from);
  Vector2D p1 = transform(line.to);
  rasterize_line( p0.x, p0.y, p1.x, p1.y, line.style.strokeColor );

}

void SoftwareRendererImp::draw_polyline( Polyline& polyline ) {

  Color c = polyline.style.strokeColor;

  if( c.a != 0 ) {
    int nPoints = polyline.points.size();
    for( int i = 0; i < nPoints - 1; i++ ) {
      Vector2D p0 = transform(polyline.points[(i+0) % nPoints]);
      Vector2D p1 = transform(polyline.points[(i+1) % nPoints]);
      rasterize_line( p0.x, p0.y, p1.x, p1.y, c );
    }
  }
}

void SoftwareRendererImp::draw_rect( Rect& rect ) {
  
  Color c;
  
  // draw as two triangles
  float x = rect.position.x;
  float y = rect.position.y;
  float w = rect.dimension.x;
  float h = rect.dimension.y;

  Vector2D p0 = transform(Vector2D(   x   ,   y   ));
  Vector2D p1 = transform(Vector2D( x + w ,   y   ));
  Vector2D p2 = transform(Vector2D(   x   , y + h ));
  Vector2D p3 = transform(Vector2D( x + w , y + h ));
  
  // draw fill
  c = rect.style.fillColor;
  if (c.a != 0 ) {
    rasterize_triangle( p0.x, p0.y, p1.x, p1.y, p2.x, p2.y, c );
    rasterize_triangle( p2.x, p2.y, p1.x, p1.y, p3.x, p3.y, c );
  }

  // draw outline
  c = rect.style.strokeColor;
  if( c.a != 0 ) {
    rasterize_line( p0.x, p0.y, p1.x, p1.y, c );
    rasterize_line( p1.x, p1.y, p3.x, p3.y, c );
    rasterize_line( p3.x, p3.y, p2.x, p2.y, c );
    rasterize_line( p2.x, p2.y, p0.x, p0.y, c );
  }
}

void SoftwareRendererImp::draw_polygon( Polygon& polygon ) {

  Color c;

  // draw fill
  c = polygon.style.fillColor;
  if( c.a != 0 ) {

    // triangulate
    vector<Vector2D> triangles;
    triangulate( polygon, triangles );

    // draw as triangles
    for (size_t i = 0; i < triangles.size(); i += 3) {
      Vector2D p0 = transform(triangles[i + 0]);
      Vector2D p1 = transform(triangles[i + 1]);
      Vector2D p2 = transform(triangles[i + 2]);
      rasterize_triangle( p0.x, p0.y, p1.x, p1.y, p2.x, p2.y, c );
    }
  }

  // draw outline
  c = polygon.style.strokeColor;
  if( c.a != 0 ) {
    int nPoints = polygon.points.size();
    for( int i = 0; i < nPoints; i++ ) {
      Vector2D p0 = transform(polygon.points[(i+0) % nPoints]);
      Vector2D p1 = transform(polygon.points[(i+1) % nPoints]);
      rasterize_line( p0.x, p0.y, p1.x, p1.y, c );
    }
  }
}

void SoftwareRendererImp::draw_ellipse( Ellipse& ellipse ) {

  // Extra credit 

}

void SoftwareRendererImp::draw_image( Image& image ) {

  Vector2D p0 = transform(image.position);
  Vector2D p1 = transform(image.position + image.dimension);

  rasterize_image( p0.x, p0.y, p1.x, p1.y, image.tex );
}

void SoftwareRendererImp::draw_group( Group& group ) {
  for ( size_t i = 0; i < group.elements.size(); ++i ) {
    draw_element(group.elements[i]);
  }
}

// Rasterization //

// The input arguments in the rasterization functions 
// below are all defined in screen space coordinates

void SoftwareRendererImp::rasterize_point( float x, float y, Color color, bool supersample ) {

  // fill in the nearest pixel
  int sx = (int) floor(x);
  int sy = (int) floor(y);

  // check bounds
  if ( sx < 0 || sx >= target_w ) return;
  if ( sy < 0 || sy >= target_h ) return;

  // fill sample - NOT doing alpha blending!
  if (supersample) {
	  for (int ki = (1 - int(sample_rate)) / 2; ki <= int(sample_rate) / 2; ki++) {
		  for (int kj = (1 - int(sample_rate)) / 2; kj <= int(sample_rate) / 2; kj++) {
			  int x = clamp<int>(sx + ki, 0, target_w - 1);
			  int y = clamp<int>(sy + kj, 0, target_h - 1);
			  render_target[4 * (x + y * target_w) + 0] = (uint8_t)((color.r*color.a + (1 - color.a)*(render_target[4 * (x + y * target_w) + 0] / 255.0f)) * 255);
			  render_target[4 * (x + y * target_w) + 1] = (uint8_t)((color.g*color.a + (1 - color.a)*(render_target[4 * (x + y * target_w) + 1] / 255.0f)) * 255);
			  render_target[4 * (x + y * target_w) + 2] = (uint8_t)((color.b*color.a + (1 - color.a)*(render_target[4 * (x + y * target_w) + 2] / 255.0f)) * 255);
			  render_target[4 * (x + y * target_w) + 3] = (uint8_t)((color.a + (1 - color.a)*(render_target[4 * (x + y * target_w) + 3] / 255.0f)) * 255);
		  }
	  }
  }
  else {
	  render_target[4 * (sx + sy * target_w) + 0] = (uint8_t)((color.r*color.a + (1 - color.a)*(render_target[4 * (sx + sy * target_w) + 0] / 255.0f)) * 255);
	  render_target[4 * (sx + sy * target_w) + 1] = (uint8_t)((color.g*color.a + (1 - color.a)*(render_target[4 * (sx + sy * target_w) + 1] / 255.0f)) * 255);
	  render_target[4 * (sx + sy * target_w) + 2] = (uint8_t)((color.b*color.a + (1 - color.a)*(render_target[4 * (sx + sy * target_w) + 2] / 255.0f)) * 255);
	  render_target[4 * (sx + sy * target_w) + 3] = (uint8_t)((color.a + (1 - color.a)*(render_target[4 * (sx + sy * target_w) + 3] / 255.0f)) * 255);
  }
}

void SoftwareRendererImp::rasterize_line( float x0, float y0,
                                          float x1, float y1,
                                          Color color) {

  // Task 2: 
  // Implement line rasterization
	bool xySwap = false;
	if (x0 - x1 == 0 || (y1 - y0) / (x1 - x0) > 1 || (y1 - y0) / (x1 - x0) < -1) {
		swap(x0, y0);
		swap(x1, y1);
		xySwap = true;
	}

	if (x0 == x1)
		return;

	if (x1 < x0) {
		swap(x0, x1);
		swap(y0, y1);
	}

	float k = (y1 - y0) / (x1 - x0);

	if (xySwap) {
		for (float x = x0, y = y0; x < x1; x++) {
			rasterize_point(y, x, color, false);
			y += k;
		}
	}
	else {
		for (float x = x0, y = y0; x < x1; x++) {
			rasterize_point(x, y, color, false);
			y += k;
		}
	}
}

void SoftwareRendererImp::rasterize_triangle( float x0, float y0,
                                              float x1, float y1,
                                              float x2, float y2,
                                              Color color ) {
  // Task 3: 
  // Implement triangle rasterization
	float xMin = floor(min({ x0,x1,x2 })+0.5)+0.5;
	float xMax = max({ x0,x1,x2 });
	float yMin = floor(min({ y0,y1,y2 })+0.5)+0.5;
	float yMax = max({ y0,y1,y2 });

	float dX0 = x1 - x0;
	float dX1 = x2 - x1;
	float dX2 = x0 - x2;
	float dY0 = y1 - y0;
	float dY1 = y2 - y1;
	float dY2 = y0 - y2;
	float A0 = dY0;
	float A1 = dY1;
	float A2 = dY2;
	float B0 = -dX0;
	float B1 = -dX1;
	float B2 = -dX2;
	float C0 = -x0 * dY0 + y0 * dX0;
	float C1 = -x1 * dY1 + y1 * dX1;
	float C2 = -x2 * dY2 + y2 * dX2;


	for (float x = xMin; x <= xMax; x++) {
		for (float y = yMin; y <= yMax; y++) {
			if ((A0*x + B0 * y + C0 <= 0
				&& A1*x + B1 * y + C1 <= 0
				&& A2*x + B2 * y + C2 <= 0)
				|| (A0*x + B0 * y + C0 >= 0
					&& A1*x + B1 * y + C1 >= 0
					&& A2*x + B2 * y + C2 >= 0))
				rasterize_point(x, y, color, false);
		}
	}
}

void SoftwareRendererImp::rasterize_image( float x0, float y0,
                                           float x1, float y1,
                                           Texture& tex ) {
  // Task 6: 
  // Implement image rasterization
	float u_scale = x1 - x0;
	float v_scale = y1 - y0;
	for (float x = floor(x0 + 0.5) + 0.5; x <= x1; x++) {
		for (float y = floor(y0 + 0.5) + 0.5; y <= y1; y++) {
			float u = (x - x0) / (x1 - x0);
			float v = (y - y0) / (y1 - y0);
			//CMU462::Color color = sampler->sample_nearest(tex, u, v, 0);
			//CMU462::Color color = sampler->sample_bilinear(tex, u, v, 0);
			CMU462::Color color = sampler->sample_trilinear(tex, u, v, u_scale, v_scale);
			rasterize_point(x, y, color, false);
		}
	}
}

// resolve samples to render target
void SoftwareRendererImp::resolve(void) {

  // Task 4: 
  // Implement supersampling
  // You may also need to modify other functions marked with "Task 4".
	for (size_t i = 0; i < target_h; i++) {
		for (size_t j = 0; j < target_w; j++) {
			CMU462::Color color(0, 0, 0, 0);
			for (size_t ki = 0; ki < sample_rate; ki++) {
				for (size_t kj = 0; kj < sample_rate; kj++) {
					size_t idx = (i*sample_rate + ki) * target_w * sample_rate * 4 + (j*sample_rate + kj) * 4;
					float r = supersample_render_target[idx + 0];
					float g = supersample_render_target[idx + 1];
					float b = supersample_render_target[idx + 2];
					float a = supersample_render_target[idx + 3];
					color += CMU462::Color(r, g, b, a);
				}
			}
			color *= 1.0f / (sample_rate * sample_rate);
			size_t idx = 4 * (i*target_w + j);
			if (color.a != 0) {
				color.r *= 1.0 / color.a * 255.0f;
				color.g *= 1.0 / color.a * 255.0f;
				color.b *= 1.0 / color.a * 255.0f;
			}

			render_target[idx + 0] = color.r;
			render_target[idx + 1] = color.g;
			render_target[idx + 2] = color.b;
			render_target[idx + 3] = color.a;
		}
	}
}


} // namespace CMU462
