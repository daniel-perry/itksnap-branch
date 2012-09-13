/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: OpenGLSliceTexture.h,v $
  Language:  C++
  Date:      $Date: 2009/08/25 19:44:25 $
  Version:   $Revision: 1.9 $
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
#ifndef __OpenGLSliceTexture_h_
#define __OpenGLSliceTexture_h_

#include "SNAPCommon.h"
#include "SNAPOpenGL.h"

#ifndef _WIN32
#ifndef GLU_VERSION_1_2
#define GLU_VERSION_1_2 1
#endif
#endif

#include "itkOrientedImage.h"

/**
 * \class OpenGLSliceTexture
 * \brief This class is used to turn a 2D ITK image of (arbitrary) type
 * into a GL texture.  
 *
 * The calls to Update will make sure that the texture is up to date.  
 */
class OpenGLSliceTexture 
{
public:
  // Image typedefs
  typedef itk::ImageBase<2> ImageBaseType;
  typedef itk::SmartPointer<ImageBaseType> ImageBasePointer;

  // see SpeedImageWrapper.h: SpeedImageWrapper::OverlaySliceType
  typedef itk::RGBAPixel<float> OverlayPixelType;
  typedef itk::Image<OverlayPixelType,2> SliceType;

  /** Constructor, initializes the texture object */
  OpenGLSliceTexture();
  OpenGLSliceTexture(GLuint, GLenum);

  /** Destructor, deallocates texture memory */
  virtual ~OpenGLSliceTexture();
  
  template<class TPixel> 
  unsigned char * getBuffer(itk::Image<TPixel,2> *inImage)
  {
    // TODO: figure out a better way to do this..
    // need to interpret data as floats for vector orientation image,
    // but need unsigned char data for the opengl texture of the image.
    // this is very innefficient...
    typedef itk::Image<TPixel,2> ImageType;
    typename ImageType::RegionType region = inImage->GetLargestPossibleRegion();
    //typename ImageType::RegionType region = inImage->GetBufferedRegion();
    typename ImageType::SizeType size = region.GetSize();
    typename ImageType::IndexType regionIndex = region.GetIndex();
    unsigned char * buffer = new unsigned char [ size[0] * size[1] * m_GlComponents ];
    for(size_t i=0; i<size[0]; ++i)
    {
      for(size_t j=0; j<size[1]; ++j)
      {
        typename ImageType::IndexType index;
        index[0] = regionIndex[0] + i;
        index[1] = regionIndex[1] + j;
        typename ImageType::PixelType pixel = inImage->GetPixel(index);
        size_t b = m_GlComponents*size[0]*j+i*m_GlComponents;
        for(size_t k=0; k<m_GlComponents; ++k)
        {
          buffer[b+k] = static_cast<unsigned char>(pixel[k]);
        }
      }
    }
    return buffer;
  }

  /** Pass in a pointer to a 2D image */
  template<class TPixel> void SetImage(itk::Image<TPixel,2> *inImage)
  {
    m_Image = inImage;
    m_Image->GetSource()->UpdateLargestPossibleRegion();
    if(m_Buffer) delete [] reinterpret_cast<unsigned char*>(m_Buffer);
    m_Buffer = getBuffer(inImage);
    //m_Buffer = inImage->GetBufferPointer();
    m_Slice = dynamic_cast<SliceType*>(inImage);
    if(!m_Slice)
    {
      std::cerr << "Error: could not convert image pointer." << std::endl;
    }
    m_UpdateTime = 0;
  }

  /** Get the dimensions of the texture image, which are powers of 2 */
  irisGetMacro(TextureSize,Vector2ui);

  /** Get the GL texture number automatically allocated by this object */
  irisGetMacro(TextureIndex,int);

  /** Set the number of components used in call to glTextureImage */
  irisSetMacro(GlComponents,GLuint);

  /** Get the format (e.g. GL_LUMINANCE) in call to glTextureImage */
  irisSetMacro(GlFormat,GLenum);

  /** Get the type (e.g. GL_UNSIGNED_INT) in call to glTextureImage */
  irisSetMacro(GlType,GLenum);

  irisGetMacro(IsVectorOverlay,bool);
  irisSetMacro(IsVectorOverlay,bool);

  /**
   * Make sure that the texture is up to date (reflects the image)
   */
  void Update();

  /**
   * Set the interpolation mode for the texture. If the interpolation mode
   * is changed, Update() will be called on the next Draw() command. The value
   * must be GL_NEAREST or GL_LINEAR
   */
  void SetInterpolation(GLenum newmode);

  /**
   * Draw the texture in the current OpenGL context on a polygon with vertices
   * (0,0) - (size_x,size_y). Paramters are the background color of the polygon
   */
  void Draw(const Vector3d &clrBackground);

  /**
   * Draw vectors over the current slice.
   */
  void DrawVectors();

  /**
   * Draw the texture in transparent mode, with given level of alpha blending.
   */
  void DrawTransparent(unsigned char alpha);

private:

  // hack for overlay vectors..
  bool m_IsVectorOverlay;
  
  // The dimensions of the texture as stored in memory
  Vector2ui m_TextureSize;

  // The pointer to the image from which the texture is computed
  ImageBasePointer m_Image;

  // Pointer to the image's data buffer (this should have been provided by ImageBase)
  void *m_Buffer;

  SliceType * m_Slice;

  // The texture number (index)
  GLuint m_TextureIndex;

  // Has the texture been initialized?
  bool m_IsTextureInitalized;

  // The pipeline time of the source image (vs. our pipeline time)
  unsigned long m_UpdateTime;

  // The number of components for Gl op
  GLuint m_GlComponents;

  // The format for Gl op
  GLenum m_GlFormat;

  // The type for Gl op
  GLenum m_GlType;

  // Interpolation mode
  GLenum m_InterpolationMode;
};

#endif // __OpenGLSliceTexture_h_
