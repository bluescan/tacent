/* APNG Disassembler 2.9
 *
 * Deconstructs APNG files into individual frames.
 *
 * http://apngdis.sourceforge.net
 *
 * Copyright (c) 2010-2017 Max Stepin
 * maxst at users.sourceforge.net
 *
 * zlib license
 * ------------
 *
 * This software is provided 'as-is', without any express or implied
 * warranty.  In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 *
 */
//////////////////////////////////////////////////////////////////////////////////
// This is a modified version of apngdis.cpp
//
// The modifications were made by Tristan Grimmer and are primarily to remove
// main so the functionality can be called directly from other source files.
// A header file has been created to allow external access. Search for @tacent
// to see where modifications have been made.
//
// All modifications should be considered to be covered by the zlib license above.
//////////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include "png.h"     /* original (unpatched) libpng is ok */
#include "zlib.h"
#include "apngdis.h"	// @tacent

#define notabc(c) ((c) < 65 || (c) > 122 || ((c) > 90 && (c) < 97))

#define id_IHDR 0x52444849
#define id_acTL 0x4C546361
#define id_fcTL 0x4C546366
#define id_IDAT 0x54414449
#define id_fdAT 0x54416466
#define id_IEND 0x444E4549

#ifndef PNG_USER_CHUNK_MALLOC_MAX
#define PNG_USER_CHUNK_MALLOC_MAX 8000000
#endif


// @tacent Wrapped in a namespace.
namespace APngDis
{


struct CHUNK { unsigned char * p; unsigned int size; };
const unsigned long cMaxPNGSize = 16384UL;

void info_fn(png_structp png_ptr, png_infop info_ptr)
{
  png_set_expand(png_ptr);
  png_set_strip_16(png_ptr);
  png_set_gray_to_rgb(png_ptr);
  png_set_add_alpha(png_ptr, 0xff, PNG_FILLER_AFTER);
  (void)png_set_interlace_handling(png_ptr);
  png_read_update_info(png_ptr, info_ptr);
}

void row_fn(png_structp png_ptr, png_bytep new_row, png_uint_32 row_num, int pass)
{
  Image * image = (Image *)png_get_progressive_ptr(png_ptr);
  png_progressive_combine_row(png_ptr, image->rows[row_num], new_row);
}

void compose_frame(unsigned char ** rows_dst, unsigned char ** rows_src, unsigned char bop, unsigned int x, unsigned int y, unsigned int w, unsigned int h)
{
  unsigned int  i, j;
  int u, v, al;

  for (j=0; j<h; j++)
  {
    unsigned char * sp = rows_src[j];
    unsigned char * dp = rows_dst[j+y] + x*4;

    if (bop == 0)
      memcpy(dp, sp, w*4);
    else
    for (i=0; i<w; i++, sp+=4, dp+=4)
    {
      if (sp[3] == 255)
        memcpy(dp, sp, 4);
      else
      if (sp[3] != 0)
      {
        if (dp[3] != 0)
        {
          u = sp[3]*255;
          v = (255-sp[3])*dp[3];
          al = u + v;
          dp[0] = (sp[0]*u + dp[0]*v)/al;
          dp[1] = (sp[1]*u + dp[1]*v)/al;
          dp[2] = (sp[2]*u + dp[2]*v)/al;
          dp[3] = al/255;
        }
        else
          memcpy(dp, sp, 4);
      }
    }
  }
}

inline unsigned int read_chunk(FILE * f, CHUNK * pChunk)
{
  unsigned char len[4];
  pChunk->size = 0;
  pChunk->p = 0;
  if (fread(&len, 4, 1, f) == 1)
  {
    pChunk->size = png_get_uint_32(len);
    if (pChunk->size > PNG_USER_CHUNK_MALLOC_MAX)
      return 0;
    pChunk->size += 12;
    pChunk->p = new unsigned char[pChunk->size];
    memcpy(pChunk->p, len, 4);
    if (fread(pChunk->p + 4, pChunk->size - 4, 1, f) == 1)
      return *(unsigned int *)(pChunk->p + 4);
  }
  return 0;
}

int processing_start(png_structp & png_ptr, png_infop & info_ptr, void * frame_ptr, bool hasInfo, CHUNK & chunkIHDR, std::vector<CHUNK>& chunksInfo)
{
  unsigned char header[8] = {137, 80, 78, 71, 13, 10, 26, 10};

  png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  info_ptr = png_create_info_struct(png_ptr);
  if (!png_ptr || !info_ptr)
    return 1;

  if (setjmp(png_jmpbuf(png_ptr)))
  {
    png_destroy_read_struct(&png_ptr, &info_ptr, 0);
    return 1;
  }

  png_set_crc_action(png_ptr, PNG_CRC_QUIET_USE, PNG_CRC_QUIET_USE);
  png_set_progressive_read_fn(png_ptr, frame_ptr, info_fn, row_fn, NULL);

  png_process_data(png_ptr, info_ptr, header, 8);
  png_process_data(png_ptr, info_ptr, chunkIHDR.p, chunkIHDR.size);

  if (hasInfo)
    for (unsigned int i=0; i<chunksInfo.size(); i++)
      png_process_data(png_ptr, info_ptr, chunksInfo[i].p, chunksInfo[i].size);

  return 0;
}

int processing_data(png_structp png_ptr, png_infop info_ptr, unsigned char * p, unsigned int size)
{
  if (!png_ptr || !info_ptr)
    return 1;

  if (setjmp(png_jmpbuf(png_ptr)))
  {
    png_destroy_read_struct(&png_ptr, &info_ptr, 0);
    return 1;
  }

  png_process_data(png_ptr, info_ptr, p, size);
  return 0;
}

int processing_finish(png_structp png_ptr, png_infop info_ptr)
{
  unsigned char footer[12] = {0, 0, 0, 0, 73, 69, 78, 68, 174, 66, 96, 130};

  if (!png_ptr || !info_ptr)
    return 1;

  if (setjmp(png_jmpbuf(png_ptr)))
  {
    png_destroy_read_struct(&png_ptr, &info_ptr, 0);
    return 1;
  }

  png_process_data(png_ptr, info_ptr, footer, 12);
  png_destroy_read_struct(&png_ptr, &info_ptr, 0);

  return 0;
}

int load_apng(const char * szIn, std::vector<Image>& img)
{
  FILE * fileHandle;
  unsigned int id, i, j, w, h, w0, h0, x0, y0;
  unsigned int delay_num, delay_den, dop, bop, imagesize;
  unsigned char sig[8];
  png_structp png_ptr;
  png_infop info_ptr;
  CHUNK chunk;
  CHUNK chunkIHDR;
  std::vector<CHUNK> chunksInfo;
  bool isAnimated = false;
  bool skipFirst = false;
  bool hasInfo = false;
  Image frameRaw;
  Image frameCur;
  Image frameNext;
  int res = -1;

  // @tacent printf("Reading '%s'...\n", szIn);

  if ((fileHandle = fopen(szIn, "rb")) != 0)
  {
    if (fread(sig, 1, 8, fileHandle) == 8 && png_sig_cmp(sig, 0, 8) == 0)
    {
      id = read_chunk(fileHandle, &chunkIHDR);
      if (!id)
      {
        fclose(fileHandle);
        return res;
      }

      if (id == id_IHDR && chunkIHDR.size == 25)
      {
        w0 = w = png_get_uint_32(chunkIHDR.p + 8);
        h0 = h = png_get_uint_32(chunkIHDR.p + 12);

        if (!w || w > cMaxPNGSize || !h || h > cMaxPNGSize)
        {
          fclose(fileHandle);
          return res;
        }

        x0 = 0;
        y0 = 0;
        delay_num = 1;
        delay_den = 10;
        dop = 0;
        bop = 0;
        imagesize = w * h * 4;

        frameRaw.init(w, h, 4);

        if (!processing_start(png_ptr, info_ptr, (void *)&frameRaw, hasInfo, chunkIHDR, chunksInfo))
        {
          frameCur.init(w, h, 4);

          while ( !feof(fileHandle) )
          {
            id = read_chunk(fileHandle, &chunk);
            if (!id)
              break;

            if (id == id_acTL && !hasInfo && !isAnimated)
            {
              isAnimated = true;
              skipFirst = true;
            }
            else
            if (id == id_fcTL && (!hasInfo || isAnimated))
            {
              if (hasInfo)
              {
                if (!processing_finish(png_ptr, info_ptr))
                {
                  frameNext.init(w, h, 4);

                  if (dop == 2)
                    memcpy(frameNext.p, frameCur.p, imagesize);

                  compose_frame(frameCur.rows, frameRaw.rows, bop, x0, y0, w0, h0);
                  frameCur.delay_num = delay_num;
                  frameCur.delay_den = delay_den;
                  img.push_back(frameCur);

                  if (dop != 2)
                  {
                    memcpy(frameNext.p, frameCur.p, imagesize);
                    if (dop == 1)
                      for (j=0; j<h0; j++)
                        memset(frameNext.rows[y0 + j] + x0*4, 0, w0*4);
                  }
                  frameCur.p = frameNext.p;
                  frameCur.rows = frameNext.rows;
                }
                else
                {
                  frameCur.free();
                  delete[] chunk.p;
                  break;
                }
              }

              // At this point the old frame is done. Let's start a new one.
              w0 = png_get_uint_32(chunk.p + 12);
              h0 = png_get_uint_32(chunk.p + 16);
              x0 = png_get_uint_32(chunk.p + 20);
              y0 = png_get_uint_32(chunk.p + 24);
              delay_num = png_get_uint_16(chunk.p + 28);
              delay_den = png_get_uint_16(chunk.p + 30);
              dop = chunk.p[32];
              bop = chunk.p[33];

              if (!w0 || w0 > cMaxPNGSize || !h0 || h0 > cMaxPNGSize
                  || x0 + w0 > w || y0 + h0 > h || dop > 2 || bop > 1)
              {
                frameCur.free();
                delete[] chunk.p;
                break;
              }

              if (hasInfo)
              {
                memcpy(chunkIHDR.p + 8, chunk.p + 12, 8);
                if (processing_start(png_ptr, info_ptr, (void *)&frameRaw, hasInfo, chunkIHDR, chunksInfo))
                {
                  frameCur.free();
                  delete[] chunk.p;
                  break;
                }
              }
              else
                skipFirst = false;

              if (img.size() == (skipFirst ? 1 : 0))
              {
                bop = 0;
                if (dop == 2)
                  dop = 1;
              }
            }
            else
            if (id == id_IDAT)
            {
              hasInfo = true;
              if (processing_data(png_ptr, info_ptr, chunk.p, chunk.size))
              {
                frameCur.free();
                delete[] chunk.p;
                break;
              }
            }
            else
            if (id == id_fdAT && isAnimated)
            {
              png_save_uint_32(chunk.p + 4, chunk.size - 16);
              memcpy(chunk.p + 8, "IDAT", 4);
              if (processing_data(png_ptr, info_ptr, chunk.p + 4, chunk.size - 4))
              {
                frameCur.free();
                delete[] chunk.p;
                break;
              }
            }
            else
            if (id == id_IEND)
            {
              if (hasInfo && !processing_finish(png_ptr, info_ptr))
              {
                compose_frame(frameCur.rows, frameRaw.rows, bop, x0, y0, w0, h0);
                frameCur.delay_num = delay_num;
                frameCur.delay_den = delay_den;
                img.push_back(frameCur);
              }
              else
                frameCur.free();

              delete[] chunk.p;
              break;
            }
            else
            if (notabc(chunk.p[4]) || notabc(chunk.p[5]) || notabc(chunk.p[6]) || notabc(chunk.p[7]))
            {
              delete[] chunk.p;
              break;
            }
            else
            if (!hasInfo)
            {
              if (processing_data(png_ptr, info_ptr, chunk.p, chunk.size))
              {
                frameCur.free();
                delete[] chunk.p;
                break;
              }
              chunksInfo.push_back(chunk);
              continue;
            }
            delete[] chunk.p;
          }
        }
        frameRaw.free();

        if (!img.empty())
          res = (skipFirst) ? 0 : 1;
      }

      for (i=0; i<chunksInfo.size(); i++)
        delete[] chunksInfo[i].p;

      chunksInfo.clear();
      delete[] chunkIHDR.p;
    }
    fclose(fileHandle);
  }

  return res;
}

void save_strip_png(char * szOut, std::vector<Image>& img)
{
  FILE * f;
  png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  png_infop info_ptr = png_create_info_struct(png_ptr);

  if (!png_ptr || !info_ptr)
    return;

  if (setjmp(png_jmpbuf(png_ptr)))
  {
    png_destroy_write_struct(&png_ptr, &info_ptr);
    return;
  }

  if ((f = fopen(szOut, "wb")) != 0)
  {
    unsigned int w = img[0].w;
    unsigned int h = img[0].h * (unsigned int)img.size();
    png_init_io(png_ptr, f);
    png_set_compression_level(png_ptr, 9);
    png_set_IHDR(png_ptr, info_ptr, w, h, 8, 6, 0, 0, 0);
    png_write_info(png_ptr, info_ptr);
    for (size_t i=0; i<img.size(); i++)
    {
      for (unsigned int j=0; j<img[i].h; j++)
        png_write_row(png_ptr, img[i].rows[j]);
    }
    png_write_end(png_ptr, info_ptr);
    fclose(f);
  }
  png_destroy_write_struct(&png_ptr, &info_ptr);
}

void save_png(char * szOut, Image * image)
{
  FILE * f;
  png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  png_infop info_ptr = png_create_info_struct(png_ptr);

  if (!png_ptr || !info_ptr)
    return;

  if (setjmp(png_jmpbuf(png_ptr)))
  {
    png_destroy_write_struct(&png_ptr, &info_ptr);
    return;
  }

  if ((f = fopen(szOut, "wb")) != 0)
  {
    png_init_io(png_ptr, f);
    png_set_compression_level(png_ptr, 9);
    png_set_IHDR(png_ptr, info_ptr, image->w, image->h, 8, 6, 0, 0, 0);
    png_write_info(png_ptr, info_ptr);
    png_write_image(png_ptr, image->rows);
    png_write_end(png_ptr, info_ptr);
    fclose(f);
  }
  png_destroy_write_struct(&png_ptr, &info_ptr);
}

void save_txt(char * szOut, Image * image)
{
  FILE * f;
  if ((f = fopen(szOut, "wt")) != 0)
  {
    fprintf(f, "delay=%d/%d\n", image->delay_num, image->delay_den);
    fclose(f);
  }
}


}


// @tacent
#if 0
int main(int argc, char** argv)
{
  unsigned int i, j;
  unsigned int strip = 0;
  char * szInput;
  char   szPath[256];
  char   szOut[256];
  std::vector<Image> img;

  printf("\nAPNG Disassembler 2.9\n\n");

  if (argc > 1 && strlen(argv[1]) < 256)
    szInput = argv[1];
  else
  {
    printf("Usage: apngdis anim.png [name]\n");
    return 1;
  }

  strcpy(szPath, szInput);
  for (i=j=0; szPath[i]!=0; i++)
  {
    if (szPath[i] == '\\' || szPath[i] == '/' || szPath[i] == ':')
      j = i+1;
  }
  szPath[j] = 0;

  if (argc > 2)
  {
    char * szOption = argv[2];

    if (szOption[0] == '/' || szOption[0] == '-')
    {
      if (szOption[1] == 's' || szOption[1] == 'S')
        strip = 1;
    }
    else
    {
      for (i=j=0; szOption[i]!=0; i++)
      {
        if (szOption[i] == '\\' || szOption[i] == '/' || szOption[i] == ':')
          j = i+1;
        if (szOption[i] == '.')
          szOption[i] = 0;
      }
      strcat(szPath, szOption+j);
    }
  }
  else
    strcat(szPath, "apngframe");

  int res = load_apng(szInput, img);
  if (res < 0)
  {
    printf("load_apng() failed: '%s'\n", szInput);
    return 1;
  }

  if (strip == 1)
  {
    strcpy(szOut, szInput);
    char * szExt = strrchr(szOut, '.');
    if (szExt != 0) *szExt = 0;
    strcat(szOut, "_strip.png");
    save_strip_png(szOut, img);
  }
  else
  {
    unsigned int num_frames = (unsigned int)img.size();
    unsigned int len = sprintf(szOut, "%d", num_frames);

    for (i=0; i<num_frames; i++)
    {
      printf("extracting frame %d of %d\n", i+1, num_frames);

      sprintf(szOut, "%s%.*d.png", szPath, len, i+res);
      save_png(szOut, &img[i]);

      sprintf(szOut, "%s%.*d.txt", szPath, len, i+res);
      save_txt(szOut, &img[i]);
    }
  }

  for (i=0; i<img.size(); i++)
    img[i].free();

  img.clear();

  printf("all done\n");

  return 0;
}
#endif
