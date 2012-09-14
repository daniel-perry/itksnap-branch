/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: OpenGLSliceTexture.cxx,v $
  Language:  C++
  Date:      $Date: 2009/08/25 19:44:25 $
  Version:   $Revision: 1.2 $
  Copyright (c) 2007 Paul A. Yushkevich
  
  This file is part of ITK-SNAP 

  ITK-SNAP is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

  -----

  Copyright (c) 2003 Insight Software Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notices for more information. 

=========================================================================*/
#include <itkRGBAPixel.h>
#include <itkImageRegionIteratorWithIndex.h>

#include "OpenGLSliceTexture.h"

OpenGLSliceTexture
::OpenGLSliceTexture()
{
  // Set to -1 to force a call to 'generate'
  m_IsTextureInitalized = false;

  // Set the update time to -1
  m_UpdateTime = 0;

  // Init the GL settings to uchar, luminance defautls, which are harmless
  m_GlComponents = 1;
  m_GlFormat = GL_LUMINANCE;
  m_GlType = GL_UNSIGNED_BYTE;
  m_InterpolationMode = GL_NEAREST;

  // Initialize the buffer pointer
  m_Buffer = NULL;

  // default is no.
  m_IsVectorOverlay = false;
}

OpenGLSliceTexture
::OpenGLSliceTexture(GLuint components, GLenum format)
{
  /*
  std::cout << "components: " << components << std::endl;
  std::cout << "format: ";
  switch(format)
  {
  case GL_LUMINANCE:
    std::cout << "luminance" << std::endl;
    break;
  case GL_RGB:
    std::cout << "rgb" << std::endl;
    break;
  case GL_RGBA:
    std::cout << "rgba" << std::endl;
    break;
  default:
    std::cout<< "unknown" << std::endl;
  }
  */

  // Set to -1 to force a call to 'generate'
  m_IsTextureInitalized = false;

  // Set the update time to -1
  m_UpdateTime = 0;

  m_GlComponents = components;
  m_GlFormat = format;
  m_GlType = GL_UNSIGNED_BYTE;
  m_InterpolationMode = GL_NEAREST;

  // Initialize the buffer pointer
  m_Buffer = NULL;
  m_IsVectorOverlay = false;
}

OpenGLSliceTexture
::~OpenGLSliceTexture()
{
  if(m_Buffer)
    delete [] reinterpret_cast<unsigned char*>(m_Buffer);
  if(m_IsTextureInitalized)
    glDeleteTextures(1,&m_TextureIndex);
}

void
OpenGLSliceTexture
::SetInterpolation(GLenum interp)
{
  assert(interp == GL_LINEAR || interp == GL_NEAREST);
  if(interp != m_InterpolationMode)
    {
    m_InterpolationMode = interp;
    m_UpdateTime = 0; // make it out-of-date
    }
}


void
OpenGLSliceTexture
::Update()
{
  // Better have an image
  assert(m_Image);

  // Update the image (necessary?)
  if(m_Image->GetSource())
  {
    m_Image->GetSource()->UpdateLargestPossibleRegion();
    if(m_Buffer) delete [] reinterpret_cast<unsigned char*>(m_Buffer);
    m_Buffer = getBuffer(m_Slice);
  }

  // Check if everything is up-to-date and no computation is needed
  if (m_IsTextureInitalized && m_UpdateTime == m_Image->GetPipelineMTime())
    return;

  // Promote the image dimensions to powers of 2
  itk::Size<2> szImage = m_Image->GetLargestPossibleRegion().GetSize();
  m_TextureSize = Vector2ui(1);

  // Use shift to quickly double the coordinates
  for (unsigned int i=0;i<2;i++)
    while (m_TextureSize(i) < szImage[i])
      m_TextureSize(i) <<= 1;

  // Create the texture index if necessary
  if(!m_IsTextureInitalized)
    {
    // Generate one texture
    glGenTextures(1,&m_TextureIndex);
    m_IsTextureInitalized = true;
    }

  // Select the texture for pixel pumping
  glBindTexture(GL_TEXTURE_2D,m_TextureIndex);

  // Properties for the texture
  glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
  glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, m_InterpolationMode );
  glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, m_InterpolationMode );

  // Turn off modulo-4 rounding in GL
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glPixelStorei(GL_PACK_ALIGNMENT, 1);

  // Allocate texture of slightly bigger size
  glTexImage2D(GL_TEXTURE_2D, 0, m_GlComponents,
    m_TextureSize(0), m_TextureSize(1),
    0, m_GlFormat, m_GlType, NULL);


  /*
  std::cout << "components: " << m_GlComponents << std::endl;
  std::cout << "format: ";
  switch(m_GlFormat)
  {
  case GL_LUMINANCE:
    std::cout << "luminance" << std::endl;
    break;
  case GL_RGB:
    std::cout << "rgb" << std::endl;
    break;
  case GL_RGBA:
    std::cout << "rgba" << std::endl;
    break;
  default:
    std::cout<< "unknown" << std::endl;
  }
  std::cout << "type: ";
  switch(m_GlType)
  {
  case GL_UNSIGNED_BYTE:
    std::cout << "unsigned byte" << std::endl;
    break;
  case GL_FLOAT:
    std::cout << "float" << std::endl;
    break;
  default:
    std::cout<< "unknown" << std::endl;
  }
  */

  // Copy a subtexture of correct size into the image
  glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, szImage[0], szImage[1],
    m_GlFormat, m_GlType, m_Buffer);

  // Remember the image's timestamp
  m_UpdateTime = m_Image->GetPipelineMTime();
}


void
OpenGLSliceTexture
::Draw(const Vector3d &clrBackground)
{
  // Update the texture
  Update();

  // Should have a texture number
  assert(m_IsTextureInitalized);

  // GL settings
  glPushAttrib(GL_TEXTURE_BIT);
  glEnable(GL_TEXTURE_2D);

  // Select our texture
  glBindTexture(GL_TEXTURE_2D,m_TextureIndex);

  // Set the color to the background color
  glColor3dv(clrBackground.data_block());

  int w = m_Image->GetBufferedRegion().GetSize()[0];
  int h = m_Image->GetBufferedRegion().GetSize()[1];
  double tx = w * 1.0 / m_TextureSize(0);
  double ty = h * 1.0 / m_TextureSize(1);

  // Draw quad 
  glBegin(GL_QUADS);
  glTexCoord2d(0,0);
  glVertex2d(0,0);
  glTexCoord2d(0,ty);
  glVertex2d(0,h);
  glTexCoord2d(tx,ty);
  glVertex2d(w,h);
  glTexCoord2d(tx,0);
  glVertex2d(w,0);
  glEnd();

  glPopAttrib();
}

void DrawLine( float ax, float ay, float bx, float by, int width, int r, int g, int b, int a ) 
{ 
    glDisable(GL_TEXTURE_2D); 
    glEnable(GL_BLEND); 
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); 
    glColor4ub( r, g, b, a); 

    glLineWidth(width); 
    glBegin(GL_LINES); 
    glVertex2f( ax, ay); 
    glVertex2f( bx, by); 
    glEnd(); 

    glDisable(GL_BLEND); 
    glEnable(GL_TEXTURE_2D); 
}  

void DrawRect( float cx, float cy, float w, float h, int r, int g, int b, int a ) 
{ 
    glDisable(GL_TEXTURE_2D); 
    glEnable(GL_BLEND); 
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); 
    glColor4ub( r, g, b, a); 

    glBegin(GL_POLYGON); 
    glVertex2f( cx-w/2, cy-h/2); 
    glVertex2f( cx+w/2, cy-h/2); 
    glVertex2f( cx+w/2, cy+h/2); 
    glVertex2f( cx-w/2, cy+h/2); 
    glEnd(); 

    glDisable(GL_BLEND); 
    glEnable(GL_TEXTURE_2D); 
}  


void
OpenGLSliceTexture
::DrawVectors(size_t x_index, size_t y_index, int x_facing, int y_facing)
{
  Update();
  if( m_Slice )
  {
    // map 2d slice coordinates to 2d gl coordinates:
    int w = m_Image->GetBufferedRegion().GetSize()[0];
    int h = m_Image->GetBufferedRegion().GetSize()[1];
    //SliceType::SizeType size = m_Slice->GetBufferedRegion().GetSize();
    SliceType::SizeType size = m_Slice->GetLargestPossibleRegion().GetSize();
    float pixelPerWidth = w / static_cast<float>(size[0]);
    float pixelPerHeight = h / static_cast<float>(size[0]);
    float xMax = pixelPerWidth / 2.f; // vector starts at center, extends to pixel edge.
    float yMax = pixelPerHeight / 2.f; // vector starts at center, extends to pixel edge.
    int r = 255;
    int g = 100;
    int b = 50;
    int a = 100;
    int line_width = 2;
    typedef itk::ImageRegionIteratorWithIndex<SliceType> ItType;
    //ItType it(m_Slice, m_Slice->GetBufferedRegion());
    ItType it(m_Slice, m_Slice->GetLargestPossibleRegion());
    for(; !it.IsAtEnd(); ++it)
    {
      SliceType::IndexType index = it.GetIndex();
      SliceType::PixelType pixel = it.Get();
      // note: just using pixel[0,1] components <==> proj_{e1,e2}(pixel)
      float ax = (.5+index[0])*pixelPerWidth; 
      float ay = (.5+index[1])*pixelPerHeight;
      float bx = ax + x_facing*pixel[x_index]*xMax; // pixel is normalized vec component.
      float by = ay + y_facing*pixel[y_index]*yMax; // pixel is normalized vec component.
      DrawLine( ax,ay, bx,by, line_width, r,g,b,a );
      DrawRect( ax,ay, 0.1,0.1, r,g,b,a );
    }
  }
}

void
OpenGLSliceTexture
::DrawTransparent(unsigned char alpha)
{
  // Update the texture
  Update();

  // Should have a texture number
  assert(m_IsTextureInitalized);

  // GL settings
  glPushAttrib(GL_TEXTURE_BIT | GL_COLOR_BUFFER_BIT);
  glEnable(GL_TEXTURE_2D);

  // Turn on alpha blending
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

  // Select our texture
  glBindTexture(GL_TEXTURE_2D,m_TextureIndex);

  // Set the color to white
  glColor4ub(255,255,255,alpha);
    
  int w = m_Image->GetBufferedRegion().GetSize()[0];
  int h = m_Image->GetBufferedRegion().GetSize()[1];
  double tx = w * 1.0 / m_TextureSize(0);
  double ty = h * 1.0 / m_TextureSize(1);

  // Draw quad 
  glBegin(GL_QUADS);
  glTexCoord2d(0,0);
  glVertex2d(0,0);
  glTexCoord2d(0,ty);
  glVertex2d(0,h);
  glTexCoord2d(tx,ty);
  glVertex2d(w,h);
  glTexCoord2d(tx,0);
  glVertex2d(w,0);
  glEnd();


  glPopAttrib();
}

