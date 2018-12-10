#include "drawsvg.h"

#include <sstream>
#include <iostream>
#include <cstdlib>

using namespace std;

namespace CMU462 {

DrawSVG::~DrawSVG() {

  tabs.clear();
  viewport_imp.clear();
  viewport_ref.clear();

  delete hardware_renderer;

  delete software_renderer_imp;
  delete software_renderer_ref;

}

string DrawSVG::name() {
  return "DrawSVG";
}

string DrawSVG::info() {

  if (show_diff) return osd;

  if (method == Hardware) {
    osd = "Hardware Renderer";
  }

  if (method == Software) {
    osd = "Software Renderer ";
    if (software_renderer == software_renderer_ref) {
      osd += "- Reference";
    }
    if (sample_rate > 1) {
      osd += "( " + to_string(sample_rate * sample_rate) + "x SSAA)";
    }
  }

  return osd;
}

void DrawSVG::init() {

  // hardware renderer
  hardware_renderer = new HardwareRenderer();

  // software renderer implementations
  software_renderer_imp = new SoftwareRendererImp();
  software_renderer_ref = new SoftwareRendererRef();
  software_renderer = software_renderer_imp; // use imp at launch

  // texture sampler implementations
  sampler_imp = new Sampler2DImp();
  sampler_ref = new Sampler2DRef();
  sampler = sampler_imp; // use imp at launch

  software_renderer_imp->set_tex_sampler(sampler_imp);
  software_renderer_ref->set_tex_sampler(sampler_ref);

  // generate mipmaps & set initial viewports
  for (size_t i = 0; i < tabs.size(); ++i) {

    viewport_imp.push_back(new ViewportImp());
    viewport_ref.push_back(new ViewportRef());

    // auto adjust
    auto_adjust(i);

    // set initial canvas_to_norm for imp using ref
    viewport_imp[i]->set_canvas_to_norm(viewport_ref[i]->get_canvas_to_norm());

    // generate mipmaps
    regenerate_mipmap(i);
  }

  // set tab and transformation if tabs loaded
  current_tab = 0;

  // initial osd
  osd = "Software Renderer";

}

void DrawSVG::render() {

  if (method == Hardware ) {
    redraw();
  }

  if( method == Software ) {
    display_pixels( &framebuffer[0] );
  }

  if (show_zoom) {
    draw_zoom();
  }

}

void DrawSVG::resize( size_t width, size_t height ) {

  this->width  = width;
  this->height = height;

  // resize render target
  framebuffer.resize( 4 * width * height);
  software_renderer_imp->set_render_target(&framebuffer[0], width, height);
  software_renderer_ref->set_render_target(&framebuffer[0], width, height);

  // update hardware renderer
  hardware_renderer->resize(width, height);

  // re-adjust norm_to_screen
  float scale = min(width, height);
  norm_to_screen(0,0) = scale; norm_to_screen(0,2) = (width  - scale) / 2;
  norm_to_screen(1,1) = scale; norm_to_screen(1,2) = (height - scale) / 2;

  // redraw current tab with updated transformation
  redraw();
}

void DrawSVG::char_event( unsigned int key ) {

  switch( key ) {

    // reset view transformation
    case ' ':
      auto_adjust(current_tab);
      redraw();
      break;

    // SSAA controls
    case '=':
      inc_sample_rate();
      break;
    case '-':
      dec_sample_rate();
      break;

    // switch between iml and ref renderer
    case 'r': case 'R':
      if (software_renderer == software_renderer_imp) {
        software_renderer = software_renderer_ref;
      } else {software_renderer = software_renderer_imp; }
      setRenderMethod( Software );
      break;

    // switch between iml and ref sampler
    case ';':
      sampler = sampler_imp;
      regenerate_mipmap(current_tab); redraw();
      break;
    case '\'':
      sampler = sampler_ref;
      regenerate_mipmap(current_tab); redraw();
      break;

    // change render method
    case 's': case 'S':
      setRenderMethod( Software ); info();
      break;
    case 'h': case 'H':
      setRenderMethod( Hardware ); info();
      break;

    // toggle diff
    case 'd': case 'D':
      if (method == Software) {
        show_diff = !show_diff; 
        redraw();
      }
      break;

    // toggle zoom
    case 'z': case 'Z':
      show_zoom = !show_zoom;
      break;

    // tab selection
    case '0':
      setTab( 9 );
      break;
    case '1':
      setTab( 0 );
      break;
    case '2':
      setTab( 1 );
      break;
    case '3':
      setTab( 2 );
      break;
    case '4':
      setTab( 3 );
      break;
    case '5':
      setTab( 4 );
      break;
    case '6':
      setTab( 5 );
      break;
    case '7':
      setTab( 6 );
      break;
    case '8':
      setTab( 7 );
      break;
    case '9':
      setTab( 8 );
      break;

    default:
      return;
  }
}

void DrawSVG::mouse_event(int key, int event, unsigned char mods) {
  switch(event) {
    case EVENT_PRESS:
      switch(key) {
        case MOUSE_LEFT:
          leftDown = true;
          break;
      }
      break;
    case EVENT_RELEASE:
      switch(key) {
        case MOUSE_LEFT:
          leftDown = false;
          break;
      }
      break;
  }
}

void DrawSVG::cursor_event( float x, float y ) {
  
  // translate when left mouse button is held down
  // diff is disabled when panning - it's too slow
  if (leftDown) {
  
    show_diff = false;
    float dx = (x - cursor_x) / width  * tabs[current_tab]->width;
    float dy = (y - cursor_y) / height * tabs[current_tab]->height;
    viewport_imp[current_tab]->update_viewbox(dx, dy, 1);
    viewport_ref[current_tab]->update_viewbox(dx, dy, 1);
    redraw();
  }
  
  // register new cursor location
  cursor_x = x;
  cursor_y = y;
}

void DrawSVG::scroll_event( float offset_x, float offset_y ) {
  // diff is disabled when zooming - it's too slow
  if (offset_x || offset_y) {
    show_diff = false;
    // prevent inverting axis when scrolling too fast
    float scale = 1 + 0.05 * offset_x + 0.05 * offset_y;
    scale = scale < 0.5 ? 0.5 : (scale > 1.5 ? 1.5 : scale); 
    viewport_imp[current_tab]->update_viewbox(0, 0, scale);
    viewport_ref[current_tab]->update_viewbox(0, 0, scale);
    redraw();
  }
}

void DrawSVG::clear( void ) {

  if (method == Hardware ) {
    hardware_renderer->clear_target();
  }

  if( method == Software ) {
    software_renderer->clear_target();    
  }
}

void DrawSVG::newTab( SVG* svg ) {
  if (tabs.size() < 9) {
    tabs.push_back(svg);
  } else {
    fprintf(stderr, "DrawSVG can only hold up to 9 tabs");
  }
}

void DrawSVG::delTab( size_t tab_index ) {
  if (tab_index < tabs.size()) {
    tabs.erase(tabs.begin() + tab_index);
  }
}

void DrawSVG::setTab( size_t tab_index ) {

  if ( tab_index < tabs.size() ) {

    // switch tab and update transformation
    current_tab = tab_index;

    // update output
    redraw();
  }
}

void DrawSVG::draw_diff() {

  // get reference output
  software_renderer_ref->draw_svg(*tabs[current_tab]);
  
  // save reference output
  vector<unsigned char> reference ( 4 * width * height );
  memcpy(&reference[0], &framebuffer[0], 4 * width * height );
  memset(&framebuffer[0], 255, 4 * width * height);

  // get implementation output
  software_renderer_imp->draw_svg(*tabs[current_tab]);

  // take difference and count errors
  int errorCount = 0;
  for( size_t i = 0; i < width * height; i++ ) {

    framebuffer[i*4 + 0] = abs(reference[i*4 + 0] - framebuffer[i*4 + 0]);
    framebuffer[i*4 + 1] = abs(reference[i*4 + 1] - framebuffer[i*4 + 1]);
    framebuffer[i*4 + 2] = abs(reference[i*4 + 2] - framebuffer[i*4 + 2]);
    framebuffer[i*4 + 3] = 255;

    for( int k = 0; k < 3; k++ ) {
      if( framebuffer[i*4+k] ) {
        errorCount++;
        break;
      }
    }
  
    osd = to_string(errorCount) + " pixels different";
  }

}

void DrawSVG::draw_zoom() {

  // size (in pixels) of region of interest
  const size_t regionSize = 32;
  
  // relative size of zoom window
  size_t zoomFactor = 16;
  
  // compute zoom factor---the zoom window should never cover
  // more than 40% of the framebuffer, horizontally or vertically
  size_t bufferSize = min( width, height );
  if( regionSize*zoomFactor > bufferSize * 0.4) {
    zoomFactor = (bufferSize * 0.4 )/regionSize;
  }
  size_t zoomSize = regionSize * zoomFactor;

  // adjust the cursor coordinates so that the region of
  // interest never goes outside the bounds of the framebuffer
  size_t cX = max( regionSize/2, min( width-regionSize/2-1, (size_t) cursor_x ));
  size_t cY = max( regionSize/2, min( height-regionSize/2-1, height - (size_t) cursor_y ));

  // grab pixels from the region of interest
  vector<unsigned char> windowPixels( 3*regionSize*regionSize );
  glReadPixels( cX - regionSize/2,
                cY - regionSize/2 + 1, // meh
                regionSize,
                regionSize,
                GL_RGB,
                GL_UNSIGNED_BYTE,
                &windowPixels[0] );

  // upsample by the zoom factor, highlighting pixel boundaries
  vector<unsigned char> zoomPixels( 3*zoomSize*zoomSize );
  unsigned char* wp = &windowPixels[0];
  // outer loop over pixels in region of interest
  for( int y = 0; y < regionSize; y++ ) {
   int y0 = y*zoomFactor;
   for( int x = 0; x < regionSize; x++ ) {
      int x0 = x*zoomFactor;
      unsigned char* zp = &zoomPixels[ ( x0 + y0*zoomSize )*3 ];
      // inner loop over upsampled block
      for( int j = 0; j < zoomFactor; j++ ) {
        for( int i = 0; i < zoomFactor; i++ ) {
          for( int k = 0; k < 3; k++ ) {
            // highlight pixel boundaries
            if( i == 0 || j == 0 ) {
              const float s = .3;
              zp[k] = (int)( (1.-2.*s)*wp[k] + s*255. );
            } else {
              zp[k] = wp[k];
            }
          }
          zp += 3;
        }
        zp += 3*( zoomSize - zoomFactor );
      }
      wp += 3;
    }
  }

  // copy pixels to the screen using OpenGL
  glMatrixMode( GL_PROJECTION ); glPushMatrix(); glLoadIdentity(); glOrtho( 0, width, 0, height, 0.01, 1000. );
  glMatrixMode( GL_MODELVIEW  ); glPushMatrix(); glLoadIdentity(); glTranslated( 0., 0., -1. );
  
  glRasterPos2i( width-zoomSize, height-zoomSize );  
  glDrawPixels( zoomSize, zoomSize, GL_RGB, GL_UNSIGNED_BYTE, &zoomPixels[0] );
  glMatrixMode( GL_PROJECTION ); glPopMatrix();
  glMatrixMode( GL_MODELVIEW ); glPopMatrix();

}


void DrawSVG::inc_sample_rate() {
  if (method == Software) {
    sample_rate += sample_rate < 4 ? 1 : 0;
    software_renderer_imp->set_sample_rate(sample_rate);
    software_renderer_ref->set_sample_rate(sample_rate);
    redraw();
  }
}

void DrawSVG::dec_sample_rate() {
  if (method == Software) {
    sample_rate -= sample_rate > 1 ? 1 : 0;
    software_renderer_imp->set_sample_rate(sample_rate);
    software_renderer_ref->set_sample_rate(sample_rate);
    redraw();
  }
}

void DrawSVG::redraw() {

  clear();

  // set canvas_to_screen transformation
  Matrix3x3 m_imp = norm_to_screen * viewport_imp[current_tab]->get_canvas_to_norm();
  Matrix3x3 m_ref = norm_to_screen * viewport_ref[current_tab]->get_canvas_to_norm();
  software_renderer_imp->set_canvas_to_screen( m_imp ); 
  software_renderer_ref->set_canvas_to_screen( m_ref ); 
  hardware_renderer->set_canvas_to_screen( m_ref );

  switch (method) {

    case Hardware:  
      hardware_renderer->draw_svg(*tabs[current_tab]);
      break;
      
    case Software: 

      if (show_diff) { draw_diff(); return; }
      software_renderer->draw_svg(*tabs[current_tab]);
      display_pixels( &framebuffer[0] );
      break;

  }
}

void DrawSVG::regenerate_mipmap(size_t tab_index) {
  if (tab_index < tabs.size()) {
    SVG* svg = tabs[tab_index];
    for ( size_t i = 0; i < svg->elements.size(); ++i ) {
  
      SVGElement* element = svg->elements[i];
      if (element->type == IMAGE) {
          Texture& tex = static_cast<Image*>(element)->tex;
          sampler->generate_mips(tex, 0);
      }
    }
  }
}

void DrawSVG::auto_adjust(size_t tab_index) {
  
  float w = tabs[tab_index]->width;
  float h = tabs[tab_index]->height;
  float span = 1.2 * max(w,h) / 2;
  viewport_imp[tab_index]->set_viewbox( w / 2, h / 2, span);
  viewport_ref[tab_index]->set_viewbox( w / 2, h / 2, span);
}


void DrawSVG::display_pixels( const unsigned char* pixels ) const {

  // copy pixels to the screen
  glPushAttrib( GL_VIEWPORT_BIT );
  glViewport(0, 0, width, height);

  glMatrixMode( GL_PROJECTION );
  glPushMatrix();
  glLoadIdentity();
  glOrtho( 0, width, 0, height, 0, 0 );

  glMatrixMode( GL_MODELVIEW );
  glPushMatrix();
  glLoadIdentity();
  glTranslatef( -1, 1, 0 );

  glRasterPos2f(0, 0);
  glPixelZoom( 1.0, -1.0 );
  glDrawPixels( width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels );
  glPixelZoom( 1.0, 1.0 );

  glPopAttrib();
  glMatrixMode( GL_PROJECTION ); glPopMatrix();
  glMatrixMode( GL_MODELVIEW  ); glPopMatrix();

}

} // namespace CMU462
