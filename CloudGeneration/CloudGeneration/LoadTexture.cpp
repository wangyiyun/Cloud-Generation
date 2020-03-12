#include "LoadTexture.h"
#include "FreeImage.h"


GLuint LoadTexture(const char* fname)
{
   GLuint tex_id;

   FIBITMAP* tempImg = FreeImage_Load(FreeImage_GetFileType(fname, 0), fname);
   FIBITMAP* img = FreeImage_ConvertTo32Bits(tempImg);

   FreeImage_Unload(tempImg);

   GLuint w = FreeImage_GetWidth(img);
   GLuint h = FreeImage_GetHeight(img);
   GLuint scanW = FreeImage_GetPitch(img);

   GLubyte* byteImg = new GLubyte[h*scanW];
   FreeImage_ConvertToRawBits(byteImg, img, scanW, 32, FI_RGBA_RED_MASK, FI_RGBA_GREEN_MASK, FI_RGBA_BLUE_MASK, FALSE);
   FreeImage_Unload(img);

   glGenTextures(1, &tex_id);
   glBindTexture(GL_TEXTURE_2D, tex_id);
   glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_BGRA, GL_UNSIGNED_BYTE, byteImg);
   glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
   glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
   glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
   glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

   delete byteImg;

   return tex_id;
}