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

#ifndef FOSGRA_H
#define FOSGRA_H

/**
 * \file fosgra.h
 *
 * libfosgra public API header.
 */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <inttypes.h>
#include <fosfat.h>

/**
 * \brief Get decoded .IMAGE buffer.
 *
 * \param[in] fosfat     disk handle.
 * \param[in] path       location on the FOS disk.
 * \param[in] offset     from where (in bytes) in the data.
 * \param[in] size       how many bytes.
 * \return NULL if error or return the buffer.
 */
uint8_t *fosgra_get_buffer (fosfat_t *fosfat,
                            const char *path, int offset, int size);

/**
 * \brief Get informations on the .IMAGE.
 *
 * \param[in] fosfat     disk handle.
 * \param[in] path       location on the FOS disk.
 * \param[out] x         image width.
 * \param[out] y         image height.
 */
void fosgra_get_info (fosfat_t *fosfat,
                      const char *path, uint16_t *x, uint16_t *y);

/**
 * \brief Test if the file is a .IMAGE.
 *
 * \param[in] fosfat     disk handle.
 * \param[in] path       location on the FOS disk.
 */
int fosgra_is_image (fosfat_t *fosfat, const char *path);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* FOSGRA_H */