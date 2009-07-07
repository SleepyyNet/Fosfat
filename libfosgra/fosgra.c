/*
 * FOS libfosgra: Smaky [.IMAGE] decoder
 * Copyright (C) 2009 Mathieu Schroeter <mathieu.schroeter@gamesover.ch>
 *
 * Thanks to Pierre Arnaud <pierre.arnaud@opac.ch>
 *           for its help and the documentation
 *    And to Epsitec SA for the Smaky computers
 *
 * This file is part of Fosfat.
 *
 * This library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include "fosgra.h"

#define FOSGRA_IMAGE_HEADER_LENGTH      32
#define FOSGRA_IMAGE_HEADER_LENGTH_BIN  256
#define FOSGRA_IMAGE_HEADER_TYP         0x81 /* .IMAGE header         */
#define FOSGRA_IMAGE_HEADER_BIN         0x82 /* binary header         */
#define FOSGRA_IMAGE_HEADER_BIN_TYP     0x01 /* .IMAGE header for bin */
#define FOSGRA_IMAGE_HEADER_COD_N       0x00 /* image uncoded         */
#define FOSGRA_IMAGE_HEADER_COD_C       0x04 /* image coded           */
#define FOSGRA_IMAGE_HEADER_BIT         0x01 /* 1 bit per pixel       */
#define FOSGRA_IMAGE_HEADER_DIR         0x02 /* coord X-Y like screen */

typedef struct fosgra_image_h_s {
  uint8_t  typ;
  uint8_t  cod;
  uint8_t  bip;
  uint8_t  dir;
  uint16_t dlx;       /* Width in pixel (multiple of 8)  */
  uint16_t dly;       /* Height in pixel                 */
  uint32_t nbb;       /* Data length                     */
  uint8_t  res[160];  /* Unused                          */
} fosgra_image_h_t;


#define FOSGRA_IMAGE_GET_PIXEL                               \
  do                                                         \
  {                                                          \
    if (offset + len >= offset && ucod_size <= offset + len) \
    {                                                        \
      *it_dec = buf[size];                                   \
      len--;                                                 \
      it_dec--;                                              \
    }                                                        \
  }                                                          \
  while (0)

static uint8_t *
fosgra_image_decod (uint8_t *buf, int size, int offset, int len, int ucod_size)
{
  uint8_t idx, cnt;
  uint8_t *dec, *it_dec;

  dec = calloc (1, len);
  if (!dec)
    return NULL;

  idx = buf[size - 2] >> buf[size - 1]; /* fix useful index */
  cnt = 8 - buf[size - 1];
  size -= 3; /* first item */
  it_dec = dec + len - 1;

  for (; size >= 0 && len > 0; size--)
  {
    /* index ? */
    if (!cnt)
    {
      idx = buf[size];
      cnt = 8;
      continue;
    }

    /* pixels coded ? */
    if (idx & 0x01)
    {
      int l = buf[size - 1] ? buf[size - 1] : 256; /* 0x00 == 256 */
      for (; l && len > 0; l--, ucod_size--)
        FOSGRA_IMAGE_GET_PIXEL;
      size--;
    }
    /* pixels normal */
    else
    {
      FOSGRA_IMAGE_GET_PIXEL;
      ucod_size--;
    }

    /* next object */
    idx >>= 1;
    cnt--;
  }

  return dec;
}

#define SWAP_16(val)         \
  {                          \
    typeof (val) p = (val);  \
    val = (p >> 8 | p << 8); \
  }
#define SWAP_32(val)                                                           \
  {                                                                            \
    typeof (val) p = (val);                                                    \
    val =                                                                      \
      (p >> 24 | ((p >> 8) & 0x0000FF00) | ((p << 8) & 0x00FF0000) | p << 24); \
  }

static int
fosgra_header_open (uint8_t *buffer, fosgra_image_h_t *header)
{
  memcpy (header, buffer, FOSGRA_IMAGE_HEADER_LENGTH);

  SWAP_16 (header->dlx);
  SWAP_16 (header->dly);
  SWAP_32 (header->nbb);

  if (   header->typ != FOSGRA_IMAGE_HEADER_TYP
      && header->typ != FOSGRA_IMAGE_HEADER_BIN_TYP)
    return -1;

  if (   header->cod != FOSGRA_IMAGE_HEADER_COD_N
      && header->cod != FOSGRA_IMAGE_HEADER_COD_C)
    return -1;

  if (header->bip != FOSGRA_IMAGE_HEADER_BIT)
    return -1;

  if (header->dir != FOSGRA_IMAGE_HEADER_DIR)
    return -1;

  if (header->dlx % 8)
    return -1;

  return 0;
}

static int
fosfat_get_header (fosfat_t *fosfat, const char *path, fosgra_image_h_t *header)
{
  uint8_t *buffer;
  int res, jump = 0;

  buffer = fosfat_get_buffer (fosfat, path, 0, 1);
  if (!buffer)
    return -1;

  /* ignore BIN header if available */
  if (*buffer == FOSGRA_IMAGE_HEADER_BIN)
    jump = FOSGRA_IMAGE_HEADER_LENGTH_BIN;
  free (buffer);

  buffer = fosfat_get_buffer (fosfat, path,
                                          jump, FOSGRA_IMAGE_HEADER_LENGTH);
  if (!buffer)
    return -1;

  res = fosgra_header_open (buffer, header);
  free (buffer);
  return res;
}

uint8_t *
fosgra_get_buffer (fosfat_t *fosfat,
                   const char *path, int offset, int size)
{
  fosgra_image_h_t header;
  uint8_t *buffer;
  uint8_t *dec;
  int res;
  int jump = FOSGRA_IMAGE_HEADER_LENGTH;

  if (!fosfat || !path)
    return NULL;

  res = fosfat_get_header (fosfat, path, &header);
  if (res)
    return NULL;

  /* ignore BIN header if available */
  if (header.typ == FOSGRA_IMAGE_HEADER_BIN_TYP)
    jump += FOSGRA_IMAGE_HEADER_LENGTH_BIN;

  if (header.cod != FOSGRA_IMAGE_HEADER_COD_C)
    return fosfat_get_buffer (fosfat, path, jump + offset, size);

  buffer = fosfat_get_buffer (fosfat, path, jump, header.nbb);
  if (!buffer)
    return NULL;

  dec = fosgra_image_decod (buffer, header.nbb,
                            offset, size, header.dlx / 8 * header.dly);
  free (buffer);
  return dec;
}

void
fosgra_get_info (fosfat_t *fosfat, const char *path, uint16_t *x, uint16_t *y)
{
  fosgra_image_h_t header;
  int res;

  if (!fosfat || !path || !x || !y)
    return;

  res = fosfat_get_header (fosfat, path, &header);
  if (res)
    return;

  *x = header.dlx;
  *y = header.dly;
}

int
fosgra_is_image (fosfat_t *fosfat, const char *path)
{
  fosgra_image_h_t header;

  if (!fosfat || !path)
    return 0;

  return fosfat_get_header (fosfat, path, &header) ? 0 : 1;
}
