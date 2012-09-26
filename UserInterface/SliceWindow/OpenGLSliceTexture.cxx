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
  m_vectorOverlayList = glGenLists(1);
  m_overlayChanged = true;
}

OpenGLSliceTexture
::OpenGLSliceTexture(GLuint components, GLenum format)
{
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
  m_vectorOverlayList = glGenLists(1);
  m_overlayChanged = true;
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

  if(m_Image->GetSource())
  {
    m_Image->GetSource()->UpdateLargestPossibleRegion();
  }

  // Check if everything is up-to-date and no computation is needed
  if (m_IsTextureInitalized && m_UpdateTime == m_Image->GetPipelineMTime())
    return;

  if(m_Buffer) delete [] reinterpret_cast<unsigned char*>(m_Buffer);
  m_Buffer = getBuffer(m_Slice);
  m_overlayChanged = true;
    
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
    if(m_overlayChanged) // recreate display list..
    {
      m_overlayChanged = false;
      glNewList(m_vectorOverlayList, GL_COMPILE_AND_EXECUTE);
      // mapping from 2d slice coordinates to 2d gl coordinates is 1-to-1:
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
        float scale = 0.7; // scales the vector to leave a border at the edges of pixel "box".
        float cx = (.5+index[0]);
        float cy = (.5+index[1]);
        float ax = cx - x_facing*pixel[x_index]*.5*scale; // pixel is normalized vec component.
        float ay = cy - y_facing*pixel[y_index]*.5*scale; // pixel is normalized vec component.
        float bx = cx + x_facing*pixel[x_index]*.5*scale; // pixel is normalized vec component.
        float by = cy + y_facing*pixel[y_index]*.5*scale; // pixel is normalized vec component.
        DrawLine( ax,ay, bx,by, line_width, r,g,b,a );
        //DrawRect( ax,ay, 0.2,0.2, r,g,b,a );
      }
      glEndList();
    }
    else
    {
      glCallList(m_vectorOverlayList);
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

