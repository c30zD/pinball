//#Ident "$Id: TextureUtil.cpp,v 1.15 2003/06/18 10:43:45 henqvist Exp $"
/***************************************************************************
                          TextureUtil.cpp  -  description
                             -------------------
    begin                : Mon Nov 27 2000
    copyright            : (C) 2000 by Henrik Enqvist
    email                : henqvist@excite.com
***************************************************************************/


// TODO free textures and image memory on exit/clear function
#include "Private.h"
#include "TextureUtil.h"
#include "Config.h"
#include <iostream>

#include <SDL_opengl.h> //should fix GL portability ; instead of <OpenGL/gl.h>
// TODO remove glu
#if EM_DEBUG
#include <GL/glu.h>
#endif
#include <SDL.h>
#include <SDL_image.h>

extern "C" {
  struct_image* loadP(const char * filename);

  struct_image* loadP(const char * filename) {
    SDL_Surface* surface = IMG_Load(filename);
    if (surface == NULL) {
      cerr << "::loadP could not load: " << filename << endl;
      return NULL;
    }
    struct_image* image = (struct_image*) malloc(sizeof(struct_image));
    image->width = surface->w;
    image->height = surface->h;
    // asume 24 bits are 3*8 and 32 are 4*8
    if (surface->format->BitsPerPixel == 24) {
      image->channels = 3;
    } else if (surface->format->BitsPerPixel == 32) {
      image->channels = 4;
    } else {
      cerr << "::loadP Only 32 bit RGBA and 24 bit RGB images supported"<<endl;
      // TODO free surface struct
      free(image);
      return NULL;
    }
    image->pixels = (unsigned char*) surface->pixels;
    return image;
  }
}

TextureUtil* TextureUtil::p_TextureUtil = NULL;

TextureUtil::TextureUtil() {
  m_colClear.r = 0.0f;
  m_colClear.g = 0.0f;
  m_colClear.b = 0.0f;
  m_colClear.a = 0.0f;
  m_SDLWindow = NULL;
  m_glContext = NULL;
}

TextureUtil::~TextureUtil() {
  //freeTextures(); // TODO: check mem leaks here //!rzr
  //EM_COUT("TextureUtil::~TextureUtil",0);
}

TextureUtil* TextureUtil::getInstance() {
  if (p_TextureUtil == NULL) {
    p_TextureUtil = new TextureUtil();
  }
  return p_TextureUtil;
}

//!+rzr : this workaround a WIN32 bug // TODO: check (bugs possible)
void TextureUtil::freeTextures()  {
  //EM_COUT("+ TextureUtil::freeTextures",1);
  map<string,EmTexture*>::iterator i;
  for ( i = m_hEmTexture.begin();
        i != m_hEmTexture.end();
        i++) {
    glDeleteTextures (1, (GLuint*) ((*i).second) ); //is that correct ?
    free((*i).second);  // malloc -> free
    (*i).second = 0;
  }
  m_hEmTexture.clear();
  /// same pointers
  m_hImageName.clear();
}


// may solve the w32 bug on resize //TODO check it @w32
void TextureUtil::reloadTextures()  {
  //cout<<"+ TextureUtil::reloadTextures"<<endl;
  m_hImageName.clear();
  map<string,EmTexture*>::iterator i;
  for ( i = m_hEmTexture.begin();
        i != m_hEmTexture.end();
        i++) {
    glDeleteTextures (1, (GLuint*) ((*i).second) ); //is that correct ?
    genTexture( (*i).first.c_str() ,  (*i).second  );
    m_hImageName.insert(pair<EmTexture*,string>(  (*i).second, (*i).first ));
  }
  //EM_CERR("- TextureUtil::reloadTextures");
  //cout<<"- TextureUtil::reloadTextures"<<endl;
}

void TextureUtil::getFilename(list<string> & files) {
  map<EmTexture*,string>::iterator i;
  for ( i = m_hImageName.begin();
        i != m_hImageName.end();
        i++) {
    files.push_back( (*i).second);
  }
}
//!-rzr


void TextureUtil::initGrx() {
  Config * config = Config::getInstance();

  cerr << "Initing SDL" << endl << endl;
  if (SDL_InitSubSystem(SDL_INIT_VIDEO) < 0) {
    cerr << "Couldn't initialize SDL video" << SDL_GetError() << endl;
    exit(1);
  }

  if (SDL_InitSubSystem(SDL_INIT_JOYSTICK) < 0) {
    cerr << "Couldn't initialize SDL joystick: " <<  SDL_GetError() << endl << endl;
  } else {
    int njoystick = SDL_NumJoysticks();
    cerr << njoystick << " joysticks were found." << endl;
    if (njoystick != 0) {
      cerr << "The names of the joysticks are:" << endl;
      for(int a=0; a<njoystick; a++ ) {
        cerr << "  " << SDL_JoystickNameForIndex(a) << endl;
      }
      cerr << "Using " << SDL_JoystickNameForIndex(0) << endl << endl;
      SDL_JoystickOpen(0);
      SDL_JoystickEventState(SDL_ENABLE);
    }
  }

  // See if we should detect the display depth
  int display_in_use = 0; /* Only using first display */
  SDL_DisplayMode mode;
  if (SDL_GetDesktopDisplayMode(display_in_use, &mode) != 0) {
    SDL_Log("SDL_GetDisplayMode failed: %s", SDL_GetError());
    exit(1);
  }

  if ( SDL_BITSPERPIXEL(mode.format) <= 8 ) {
    SDL_GL_SetAttribute( SDL_GL_RED_SIZE, 2 );
    SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, 3 );
    SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, 3 );
  } else        if ( SDL_BITSPERPIXEL(mode.format) <= 16 ) {
    SDL_GL_SetAttribute( SDL_GL_RED_SIZE, 5 );
    SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, 5 );
    SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, 5 );
  }     else {
    SDL_GL_SetAttribute( SDL_GL_RED_SIZE, 8 );
    SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, 8 );
    SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, 8 );
  }

  SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, 16 );
  SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );

  /* Initialize the display */
  m_SDLWindow =
    SDL_CreateWindow("Emilia Pinball", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                     config->getWidth(), config->getHeight(), /*config->getBpp(),*/
                     SDL_WINDOW_OPENGL
                     | (config->useFullScreen() ? SDL_WINDOW_FULLSCREEN : 0));
  if (m_SDLWindow == NULL) {
    cerr << "Couldn't create SDL window: " << SDL_GetError() << endl;
    exit(1);
  }
  
  m_glContext = SDL_GL_CreateContext(m_SDLWindow);
  if (m_glContext == NULL) {
    cerr << "Couldn't create GL context: " << SDL_GetError() << endl;
    exit(1);
  }

  SDL_ShowCursor(SDL_DISABLE);
  //SDL_WM_SetCaption("Emilia Pinball", NULL);

  cerr << "Vendor     : " << glGetString( GL_VENDOR ) << endl;
  cerr << "Renderer   : " << glGetString( GL_RENDERER ) << endl;
  cerr << "Version    : " << glGetString( GL_VERSION ) << endl;
  cerr << "Extensions : " << glGetString( GL_EXTENSIONS ) << endl << endl;
  //TODO: that would be usefull to report CPU/RAM specs also //!rzr

  int value;
  SDL_GL_GetAttribute( SDL_GL_RED_SIZE, &value );
  cerr << "SDL_GL_RED_SIZE: " << value << endl;
  SDL_GL_GetAttribute( SDL_GL_GREEN_SIZE, &value );
  cerr << "SDL_GL_GREEN_SIZE: " << value << endl;
  SDL_GL_GetAttribute( SDL_GL_BLUE_SIZE, &value );
  cerr << "SDL_GL_BLUE_SIZE: " << value << endl;
  SDL_GL_GetAttribute( SDL_GL_DEPTH_SIZE, &value );
  cerr << "SDL_GL_DEPTH_SIZE: " << value << endl;
  SDL_GL_GetAttribute( SDL_GL_DOUBLEBUFFER, &value );
  cerr << "SDL_GL_DOUBLEBUFFER: " << value << endl << endl;

  this->resizeView(config->getWidth(), config->getHeight());
}

void TextureUtil::stopGrx() {
  cerr << "Stopping graphics...";
  if(m_SDLWindow != NULL)
  {
      SDL_GL_DeleteContext(m_glContext);
      SDL_DestroyWindow(m_SDLWindow);
      m_SDLWindow = NULL;
  }
  SDL_Quit();

  cerr << "ok." << endl;
}

void TextureUtil::setClearColor(float r, float g, float b, float a) {
  m_colClear.r = r;
  m_colClear.g = g;
  m_colClear.b = b;
  m_colClear.a = a;
  glClearColor(m_colClear.r, m_colClear.g, m_colClear.b, m_colClear.a);
}

void TextureUtil::resizeView(unsigned int w, unsigned int h) {
  glViewport(0, 0, (GLsizei) w, (GLsizei) h);

  glClearColor(m_colClear.r, m_colClear.g, m_colClear.b, m_colClear.a);
  glClearDepth(1.0);

  glShadeModel(GL_SMOOTH);

  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);
  glFrontFace(GL_CW);

  //    glDisable(GL_DITHER);
  glDisable(GL_LIGHTING);

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glFrustum(-EM_RIGHT*EM_NEAR, EM_RIGHT*EM_NEAR, -EM_UP*EM_NEAR, EM_UP*EM_NEAR,
            EM_NEAR, EM_FAR);
  glMatrixMode(GL_MODELVIEW);

#if OPENGL_LIGHTS
  glEnable(GL_LIGHTING);
#endif
}


EmImage* TextureUtil::loadImage(const char* filename) {
  struct_image * image = loadP(filename);
  if (image == NULL) {
    cerr << "TextureUtil::loadImage unable to load " << filename << endl;
  }
  return image;
}
int TextureUtil::genTexture( char const * const filename,
                             EmTexture * const texture)
{
  //cout<<"+ Texture::genTexture : "<<filename<<endl;
  *texture = 0;

  // Load Texture
  struct_image* image = 0;

  // load the texture
  image = loadP(filename);
  if (image == NULL) {
    cerr << "TextureUtil::loadTexture error loading file " << filename << endl;
    return -1;
  }
  //TODO : Pad texture != 2^n x 2^n //!rzr

  //cout<<" Create Texture"<<endl;
  glGenTextures(1, texture);
  glBindTexture(GL_TEXTURE_2D, *texture);


  // 2d texture, level of detail 0 (normal), 3 components (red, green, blue),
  // x size from image, y size from image,
  // border 0 (normal), rgb color data, unsigned byte data,
  // and finally the data itself.x
  GLenum comp;
  switch (image->channels) {
  case 3: comp = GL_RGB; break;
  case 4: comp = GL_RGBA; break;
  default: comp = GL_RGB;
    cerr << "TextureUtil::loadTexture unknown image format" << endl;
    return -1;
  }


  glTexImage2D(GL_TEXTURE_2D, 0, comp, image->width, image->height, 0, comp,
               GL_UNSIGNED_BYTE, image->pixels);

  EM_COUT("loaded texture: " << filename, 1);
  EM_COUT("size " << image->width <<" "<< image->height, 1);
  //    EM_COUT("bytes per pixel " << (int)image->format->BytesPerPixel, 1);
  //    EM_COUT("bits per pixel " << (int)image->format->BitsPerPixel, 1);

  EM_COUT("- Texture::genTexture : "<<filename<<hex<<texture,0);
  return 1;
}


EmTexture* TextureUtil::loadTexture(const char* filename) {
  EmTexture* texture = 0;
  // look if the texture is already loaded
  if (m_hEmTexture.find(string(filename)) != m_hEmTexture.end()) {
    EM_COUT("TextureUtil::loadTexture found texture "
            << filename << " in cache", 0);
    map<string, EmTexture*>::iterator element
      = m_hEmTexture.find(string(filename));
    return (*element).second;
  }

  texture =  new EmTexture;
  int t = genTexture( filename, texture); //!rzr {
  if ( t < 0 ) { delete texture; return 0; } // could have been written better

  //EM_GLERROR(" In TextureUtil::loadTexture ");
  // insert the texture into the cache
  m_hEmTexture.insert(pair<string, EmTexture*>(string(filename), texture));
  m_hImageName.insert(pair<EmTexture*, string>(texture, string(filename)));
  //cout<<"- TextureUtil::loadTexture"<<endl;
  return texture;
}

const char * TextureUtil::getTextureName(EmTexture * tex) {
  map<EmTexture*, string>::iterator element = m_hImageName.find(tex);
  if (element != m_hImageName.end()) {
    return (*element).second.c_str();
  }
  cerr << "TextureUtil::getTextureName could not find image name" << endl;
  return NULL;
}
/*
  void TextureUtil::load(list<string> & files)
  {
  list<string>::iterator is = files.begin();
  for ( ; is != files.end() ; is++ )
  loadTexture( (*is).c_str() );
  }
*/

SDL_Window* TextureUtil::getSDLWindow() const
{
    return m_SDLWindow;
}

//EOF: $Id: TextureUtil.cpp,v 1.15 2003/06/18 10:43:45 henqvist Exp $
