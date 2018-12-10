#include "svg.h"
#include "png.h"
#include "base64.h"

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>

using namespace std;

namespace CMU462 {

Group::~Group() {
  for (size_t i = 0; i < elements.size(); i++) {
    delete elements[i];
  } elements.clear();
}

SVG::~SVG() {
  for (size_t i = 0; i < elements.size(); i++) {
    delete elements[i];
  } elements.clear();
}

// Parser //

int SVGParser::load( const char* filename, SVG* svg ) {

  ifstream in( filename );
  if( !in.is_open() ) {
     return -1;
  }
  in.close();

  XMLDocument doc;
  doc.LoadFile( filename );
  if( doc.Error() ) {
     doc.PrintError();
     exit( 1 );
  }

  XMLElement* root = doc.FirstChildElement( "svg" );
  if( !root ) {
     cerr << "Error: not an SVG file!" << endl;
     exit( 1 );
  }

  root->QueryFloatAttribute( "width",  &svg->width  );
  root->QueryFloatAttribute( "height", &svg->height );

  parseSVG( root, svg );

  return 0;
}

void SVGParser::parseSVG( XMLElement* xml, SVG* svg ) {

  /* NOTE (sky):
   * SVG uses a "painters model" when drawing elements. Elements 
   * that appear later in the document are drawn after (on top of) 
   * elements that appear earlier in the document. The parser loads
   * elements in the same order and the renderer should respect this
   * order when drawing elements.
   */

  XMLElement* elem = xml->FirstChildElement();
  while( elem ) {

    string elementType ( elem->Value() );
    if( elementType == "line" ) {

      Line* line = new Line();
      parseElement(elem, line );
      parseLine( elem, line );
      svg->elements.push_back( line );

    } else if( elementType == "polyline" ) {

      Polyline* polyline = new Polyline();
      parseElement(elem, polyline );
      parsePolyline( elem, polyline );
      svg->elements.push_back( polyline );

    } else if( elementType == "rect" ) {

      float w = elem->FloatAttribute("width" );
      float h = elem->FloatAttribute("height");

      // treat zero-size rectangles as points
      if (w == 0 && h == 0) {
        Point* point = new Point();
        parseElement(elem, point );
        parsePoint( elem, point );
        svg->elements.push_back( point );
      } else {
        Rect* rect = new Rect();
        parseElement( elem, rect );
        parseRect( elem, rect );
        svg->elements.push_back( rect );
      }

    } else if( elementType == "polygon" ) {

      Polygon* polygon = new Polygon();
      parseElement( elem, polygon);
      parsePolygon( elem, polygon );
      svg->elements.push_back( polygon );

    } else if( elementType == "ellipse" ) {

      Ellipse* ellipse = new Ellipse();
      parseElement( elem, ellipse);
      parseEllipse( elem, ellipse );
      svg->elements.push_back( ellipse );

    } else if ( elementType == "image" ) {

      Image* image = new Image();
      parseElement( elem, image);
      parseImage( elem, image);
      svg->elements.push_back( image ); 

    } else if( elementType == "g" ) {

       Group* group = new Group();
       parseElement( elem, group);
       parseGroup( elem, group );
       svg->elements.push_back( group );

    } else {
       // unknown element type --- include default handler here if desired
    }

    elem = elem->NextSiblingElement();
  }
}

void SVGParser::parseElement( XMLElement* xml, SVGElement* element ) {

  // parse style
  Style* style = &element->style;
  const char* fill = xml->Attribute( "fill" );
  if( fill ) style->fillColor = Color::fromHex( fill );

  const char* fill_opacity = xml->Attribute( "fill-opacity" );
  if( fill_opacity ) style->fillColor.a = atof( fill_opacity );

  const char* stroke = xml->Attribute( "stroke" );
  const char* stroke_opacity = xml->Attribute( "stroke-opacity" );
  if( stroke ) {
    style->strokeColor = Color::fromHex( stroke );
    if( stroke_opacity ) style->strokeColor.a = atof( stroke_opacity );
  } else {
    style->strokeColor = Color::Black;
    style->strokeColor.a = 0;
  }


  xml->QueryFloatAttribute( "stroke-width",      &style->strokeWidth );
  xml->QueryFloatAttribute( "stroke-miterlimit", &style->miterLimit  );

  // parse transformation
  const char* trans = xml->Attribute( "transform" );
  if ( trans ) {
    
    // NOTE (sky):
    // This implements the SVG transformation specification. All the SVG 
    // transformations are supported as documented in the link below:
    // https://developer.mozilla.org/en-US/docs/Web/SVG/Attribute/transform

    // consolidate transformation
    Matrix3x3 transform = Matrix3x3::identity();

    string trans_str = trans; size_t paren_l, paren_r;
    while ( trans_str.find_first_of('(') != string::npos ) {

      paren_l = trans_str.find_first_of('(');
      paren_r = trans_str.find_first_of(')');

      string type = trans_str.substr(0, paren_l);
      string data = trans_str.substr(paren_l + 1, paren_r - paren_l - 1);

      if ( type == "matrix" ) {
        
        string matrix_str = data;
        replace( matrix_str.begin(), matrix_str.end(), ',', ' ');

        stringstream ss (matrix_str);
        float a; float b; float c; float d; float e; float f;
        ss >> a; ss >> b; ss >> c; ss >> d; ss >> e; ss >> f;

        Matrix3x3 m;
        m(0,0) = a; m(0,1) = c; m(0,2) = e;
        m(1,0) = b; m(1,1) = d; m(1,2) = f;
        m(2,0) = 0; m(2,1) = 0; m(2,2) = 1;        
        transform = transform * m;
      
      } else if ( type == "translate" ) {
        
        stringstream ss (data);
        float x; if (!(ss >> x)) x = 0;
        float y; if (!(ss >> y)) y = 0;

        Matrix3x3 m = Matrix3x3::identity();
        
        m(0,2) = x;
        m(1,2) = y;
        
        transform = transform * m;

      } else if (type == "scale" ) {

        stringstream ss (data);
        float x; if (!(ss >> x)) x = 1;
        float y; if (!(ss >> y)) y = 1;

        Matrix3x3 m = Matrix3x3::identity();
        
        m(0,0) = x;
        m(1,1) = y;

        transform = transform * m;

      } else if (type == "rotate") {

        stringstream ss (data);
        float a; if (!(ss >> a)) a = 0;
        float x; if (!(ss >> x)) x = 0;
        float y; if (!(ss >> y)) y = 0;

        if ( x != 0 || y != 0 ) {

          Matrix3x3 m = Matrix3x3::identity();

          m(0,0) = cos(a*PI/180.0f); m(0,1) = -sin(a*PI/180.0f);
          m(1,0) = sin(a*PI/180.0f); m(1,1) =  cos(a*PI/180.0f);

          m(0,2) = -x * cos(a*PI/180.0f) + y * sin(a*PI/180.0f) + x;
          m(1,2) = -x * sin(a*PI/180.0f) - y * cos(a*PI/180.0f) + y;

          transform = transform * m;

        } else {
          
          Matrix3x3 m = Matrix3x3::identity();
          
          m(0,0) = cos(a*PI/180.0f); m(0,1) = -sin(a*PI/180.0f);
          m(1,0) = sin(a*PI/180.0f); m(1,1) =  cos(a*PI/180.0f);
          
          transform = transform * m;
        }
        
      } else if (type == "skewX" ) {

        stringstream ss (data);
        float a; ss >> a;

        Matrix3x3 m = Matrix3x3::identity();
        
        m(0,1) = tan(a*PI/180.0f);

        transform = transform * m;

      } else if (type == "skewY" ) {

        stringstream ss (data);
        float a; ss >> a;

        Matrix3x3 m = Matrix3x3::identity();
        
        m(1,0) = tan(a*PI/180.0f);

        transform = transform * m;

      } else {
        cerr << "unknown transformation type: " << type << endl;
      }

      size_t end = paren_r + 2;
      trans_str.erase(0, end);
    }

    element->transform = transform;
  }
}   


void SVGParser::parsePoint( XMLElement* xml, Point* point ) {
  point->position = Vector2D(xml->FloatAttribute( "x" ),
                             xml->FloatAttribute( "y" ));
}

void SVGParser::parseLine( XMLElement* xml, Line* line ) {
  line->from = Vector2D(xml->FloatAttribute( "x1" ),
                        xml->FloatAttribute( "y1" ));
  line->to   = Vector2D(xml->FloatAttribute( "x2" ),
                        xml->FloatAttribute( "y2" ));
}

void SVGParser::parsePolyline( XMLElement* xml, Polyline* polyline ) {

  stringstream points (xml->Attribute( "points" ));

  float x, y;
  char c;

  while( points >> x >> c >> y ) {
     polyline->points.push_back( Vector2D( x, y ) );
  }
}

void SVGParser::parseRect( XMLElement* xml, Rect* rect ) {
  rect->position  = Vector2D(xml->FloatAttribute( "x" ),
                             xml->FloatAttribute( "y" ));
  rect->dimension = Vector2D(xml->FloatAttribute( "width"  ),
                             xml->FloatAttribute( "height" ));
}

void SVGParser::parsePolygon( XMLElement* xml, Polygon* polygon ) {

  stringstream points (xml->Attribute( "points" ));

  float x, y;
  char c;

  while( points >> x >> c >> y ) {
     polygon->points.push_back( Vector2D( x, y ) );
  }
}

void SVGParser::parseEllipse( XMLElement* xml, Ellipse* ellipse ) {
  ellipse->center = Vector2D(xml->FloatAttribute( "cx" ),
                             xml->FloatAttribute( "cy" ));

  ellipse->radius = Vector2D(xml->FloatAttribute( "rx" ),
                             xml->FloatAttribute( "ry" ));
}

void SVGParser::parseImage( XMLElement* xml, Image* image ) {
  image->position  = Vector2D ( xml->FloatAttribute( "x" ),
                                xml->FloatAttribute( "y" ));
  image->dimension = Vector2D ( xml->FloatAttribute( "width"  ),
                                xml->FloatAttribute( "height" )); 

  // read png data
  const char* data = xml->Attribute( "xlink:href" );
  while (*data != ',') data++; data++;
  
  // decode base64 encoded data
  string encoded = data;
  encoded.erase(remove(encoded.begin(), encoded.end(), ' ' ), encoded.end());
  encoded.erase(remove(encoded.begin(), encoded.end(), '\t'), encoded.end());
  encoded.erase(remove(encoded.begin(), encoded.end(), '\n'), encoded.end());
  string decoded = base64_decode(encoded);

  // load decoded data into buffer
  const unsigned char* buffer = (unsigned char*) decoded.c_str(); 
  size_t size = decoded.size();

  // load into png
  PNG png; PNGParser::load(buffer, size, png);
  
  // create bitmap texture from png (mip level 0)
  MipLevel mip_start;
  mip_start.width  = png.width;
  mip_start.height = png.height;
  mip_start.texels = png.pixels;

  // add to svg
  image->tex.width  = mip_start.width;
  image->tex.height = mip_start.height;
  image->tex.mipmap.push_back(mip_start);
}

void SVGParser::parseGroup( XMLElement* xml, Group* group ) {

  /* NOTE (sky):
   * A group contains a list of elements, and optionally a transformation
   * to apply to all the elements it contains. Elements in a group follow
   * the same draw order as elements in a svg (top to bottom).  
   * A group should be considered as one single element outside its scope.
   * This means at draw time, all elements in a group should be drawn before 
   * elements outside the group. All elements in the group inherits the group
   * transformation, and keep in mind that transformation is accumulative.
   * Groups can also be nested.  
   */
  XMLElement* elem = xml->FirstChildElement();
  while( elem ) {

    string elementType ( elem->Value() );
    if( elementType == "line" ) {

      Line* line = new Line();
      parseElement( elem, line );
      parseLine( elem, line );
      group->elements.push_back( line );
    
    } else if( elementType == "polyline" ) {

      Polyline* polyline = new Polyline();
      parseElement( elem, polyline );
      parsePolyline( elem, polyline );
      group->elements.push_back( polyline );

    } else if( elementType == "rect" ) {

      float w = elem->FloatAttribute("width" );
      float h = elem->FloatAttribute("height");

      // treat zero-size rectangles as points
      if (w == 0 && h == 0) {
        Point* point = new Point();
        parseElement( elem, point );
        parsePoint( elem, point );
        group->elements.push_back( point );
      } else {
        Rect* rect = new Rect();
        parseElement( elem, rect );
        parseRect( elem, rect );
        group->elements.push_back( rect );
      }

    } else if( elementType == "polygon" ) {
    
      Polygon* polygon = new Polygon();
      parseElement( elem, polygon );
      parsePolygon( elem, polygon );
      group->elements.push_back( polygon );
    
    } else if( elementType == "ellipse" ) {
    
      Ellipse* ellipse = new Ellipse();
      parseElement( elem, ellipse );
      parseEllipse( elem, ellipse );
      group->elements.push_back( ellipse );

    } else if ( elementType == "image" ) {
    
      Image* image = new Image();
      parseElement( elem, image );
      parseImage( elem, image);
      group->elements.push_back( image ); 
    
    } else if( elementType == "g" ) {
    
       Group* sub_group = new Group();
       parseElement( elem, sub_group );
       parseGroup( elem, sub_group );
       group->elements.push_back( sub_group );
    
    } else {
       // unknown element type --- include default handler here if desired
    }    
    elem = elem->NextSiblingElement();
  }
}

} // namespace CMU462

