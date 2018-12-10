#include "CMU462.h"
#include "viewer.h"
#include "drawsvg.h"

#include <sys/stat.h>
#include <dirent.h>
#include <iostream>

using namespace std;
using namespace CMU462;

#define msg(s) cerr << "[DrawSVG] " << s << endl;

int loadFile( DrawSVG* drawsvg, const char* path ) {

  SVG* svg = new SVG();

  if( SVGParser::load( path, svg ) < 0) {
    delete svg;
    return -1;
  }
  
  drawsvg->newTab( svg );
  return 0;
}

int loadDirectory( DrawSVG* drawsvg, const char* path ) {

  DIR *dir = opendir (path);
  if(dir) {
    
    struct dirent *ent; size_t n = 0;
    
    // load files
    string pathname = path; 
    if (pathname[pathname.back()] != '/') pathname.push_back('/');
    while (((ent = readdir (dir)) != NULL) && (n < 9)) {

      string filename = ent->d_name;
      string filesufx = filename.substr(filename.find_last_of(".") + 1);
      if (filesufx == "svg" ) {
        cerr << "[DrawSVG] Loading " << filename << "... "; 
        if (loadFile(drawsvg, (pathname + filename).c_str()) < 0) {
          cerr << "Failed (Invalid SVG file)" << endl;
        } else {
          cerr << "Succeeded" << endl;
          n++;
        }
      }
    }

    closedir (dir);

    if (n) {
      msg("Successfully Loaded " << n << " files from " << path);
      return 0;
    }

    msg("No valid svg files found in " << path);
    return -1;
  } 

  msg("Could not open directory" << path);
  return -1;
}

int loadPath( DrawSVG* drawsvg, const char* path) {

  struct stat st;

  // file exist?
  if(stat(path, &st) < 0 ) {
    msg("File does not exit: " << path);
    return -1;
  }

  // load directory
  if( st.st_mode & S_IFDIR ) {
    return loadDirectory(drawsvg, path);
  } 

  // load file
  if( st.st_mode & S_IFREG ) {
    return loadFile(drawsvg, path);
  }

  msg("Invalid path: " << path);
  return -1;
}

int main( int argc, char** argv ) {

  // create viewer
  Viewer viewer = Viewer();

  // create drawsvg
  DrawSVG* drawsvg = new DrawSVG();

  // set drawsvg as renderer
  viewer.set_renderer(drawsvg);

  // load tests
  if( argc == 2 ) {
    if (loadPath(drawsvg, argv[1]) < 0) exit(0);
  } else {
    msg("Usage: drawsvg <path to test file or directory>"); exit(0);
  }

  // init viewer
  viewer.init();

  // start viewer
  viewer.start();

  return 0;
}
