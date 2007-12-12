/*
 * FOS fosread: tool in read-only for Smaky file system
 * Copyright (C) 2006-2007 Mathieu Schroeter <mathieu.schroeter@gamesover.ch>
 *
 * Thanks to Pierre Arnaud <pierre.arnaud@opac.ch>
 *           for its help and the documentation
 *    And to Epsitec SA for the Smaky computers
 *
 * This file is part of Fosfat.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h> /* strcmp strcpy */
#include <getopt.h>

#include "fosfat.h"

typedef struct ginfo {
  char name[FOSFAT_NAMELGT];
} s_global_info;

#define HELP_TEXT \
"Tool for a read-only access on a Smaky disk. fosfat-" VERSION "\n\n" \
"Usage: fosread [options] device mode [node] [path]\n\n" \
" -h --help             this help\n" \
" -v --version          version\n" \
" -a --harddisk         force an hard disk (default autodetect)\n" \
" -f --floppydisk       force a floppy disk (default autodetect)\n" \
" -l --fos-logger       that will turn on the FOS logger\n\n" \
" device                /dev/fd0 : floppy disk\n" \
"                       /dev/sda : hard disk, etc\n" \
" mode\n" \
"   list                list the content of a node\n" \
"   get                 copy a file from the Smaky's disk in a" \
" local directory\n" \
" node                  the tree with the file (or folder)" \
" for 'get' or 'list'\n" \
"                       example: foo/bar/toto.text\n" \
" path                  you can specify a path for save" \
" the file (with get mode)\n" \
"\nPlease, report bugs to <fosfat-devel@gamesover.ch>.\n"

#define VERSION_TEXT "fosread-" VERSION "\n"


/**
 * \brief Get info from the disk.
 *
 * \param fosfat the main structure
 * \return info
 */
s_global_info *get_ginfo(s_fosfat *fosfat) {
  s_global_info *ginfo;
  char *name;

  if ((name = fosfat_diskname(fosfat))) {
    ginfo = malloc(sizeof(s_global_info));
    strncpy(ginfo->name, name, FOSFAT_NAMELGT - 1);
    ginfo->name[FOSFAT_NAMELGT - 1] = '\0';
    free(name);
  }
  else {
    printf("ERROR: I can't read the name of this disk!\n");
    ginfo = NULL;
  }

  return ginfo;
}

/**
 * \brief Print date and hour.
 *
 * \param time date and hour
 */
void print_date(s_fosfat_time *time) {
  printf(" %4i-%02i-%02i %02i:%02i", time->year, time->month,
                                     time->day, time->hour, time->minute);
}

/**
 * \brief Print a file in the list.
 *
 * \param file description
 */
void print_file(s_fosfat_file *file) {
  char filename[FOSFAT_NAMELGT];

  if (file->att.isdir)
    snprintf(filename, strrchr(file->name, '.') - file->name + 1,
             "%s", file->name);
  else
    strncpy(filename, file->name, FOSFAT_NAMELGT - 1);

  filename[FOSFAT_NAMELGT - 1] = '\0';

  printf("%c%c%c %11i", file->att.isdir ? 'd' : '-',
                        file->att.isvisible ? '-' : 'h',
                        file->att.isencoded ? 'e' : '-',
                        file->size);
  print_date(&file->time_c);
  print_date(&file->time_w);
  print_date(&file->time_r);
  printf(" %s\n", filename);
}

/**
 * \brief List the content of a directory.
 *
 * \param fosfat the main structure
 * \param path where in the tree
 * \return true if it is ok
 */
int list_dir(s_fosfat *fosfat, const char *path) {
  s_fosfat_file *files, *first_file;

  if ((files = fosfat_list_dir(fosfat, path))) {
    first_file = files;
    printf("path: %s\n\n", path);
    printf("           size creation         last change");
    printf("      last view        filename\n");
    printf("           ---- --------         -----------");
    printf("      ---------        --------\n");

    do {
      print_file(files);
    } while ((files = files->next_file));

    printf("\nd:directory  h:hidden  e:encoded\n");
    fosfat_free_listdir(first_file);
  }
  else {
    printf("ERROR: I can't found this path!\n");
    return 0;
  }

  return 1;
}

/**
 * \brief Copy a file from the disk.
 *
 * \param fosfat the main structure
 * \param path where in the tree
 * \param dst where in local
 * \return true if it is ok
 */
int get_file(s_fosfat *fosfat, const char *path, const char *dst) {
  int res = 0;
  char *new_file;

  if (!strcasecmp(dst, "./"))
    new_file = strdup((strrchr(path, '/') ? strrchr(path, '/') + 1 : path));
  else
    new_file = strdup(dst);

  printf("File \"%s\" is copying ...\n", path);

  if (fosfat_get_file(fosfat, path, new_file, 1)) {
    res = 1;
    printf("Okay..\n");
  }
  else
    printf("ERROR: I can't copy the file!\n");

  free(new_file);
  return res;
}

/** Print help. */
void print_info(void) {
  printf(HELP_TEXT);
}

/** Print version. */
void print_version(void) {
  printf(VERSION_TEXT);
}

int main(int argc, char **argv) {
  int res = 0, i, next_option;
  e_fosfat_disk type = eDAUTO;
  char *device = NULL, *mode = NULL, *node = NULL, *path = NULL;
  s_fosfat *fosfat;
  s_global_info *ginfo = NULL;

  const char *const short_options = "afhlv";

  const struct option long_options[] = {
    { "harddisk",     0, NULL, 'a' },
    { "floppydisk",   0, NULL, 'f' },
    { "help",         0, NULL, 'h' },
    { "fos-logger",   0, NULL, 'l' },
    { "version",      0, NULL, 'v' },
    { NULL,           0, NULL,  0  }
  };

  /* check options */
  do {
    next_option = getopt_long(argc, argv, short_options, long_options, NULL);
    switch (next_option) {
      default :           /* unknown */
      case '?':           /* invalid option */
      case 'h':           /* -h or --help */
        print_info();
        return -1;
      case 'v':           /* -v or --version */
        print_version();
        return -1;
      case 'a':           /* -a or --harddisk */
        type = eHD;
        break ;
      case 'f':           /* -f or --floppydisk */
        type = eFD;
        break ;
      case 'l':           /* -l or --fos-logger */
        fosfat_logger(1);
        break ;
      case -1:            /* end */
        break ;
    }
  } while (next_option != -1);

  if (argc < optind + 2) {
    print_info();
    return -1;
  }

  for (i = optind; i < argc; i++) {
    if (i == optind)
      device = strdup(argv[optind]);
    else if (i == optind + 1)
      mode = strdup(argv[optind + 1]);
    else if (i == optind + 2)
      node = strdup(argv[optind + 2]);
    else if (i == optind + 3)
      path = strdup(argv[optind + 3]);
  }

  /* Open the floppy disk (or hard disk) */
  if (!(fosfat = fosfat_open(device, type))) {
    printf("Could not open %s for reading!\n", device);
    res = -1;
  }

  /* Get globals informations on the disk */
  if (!res && (ginfo = get_ginfo(fosfat))) {
    printf("Smaky disk %s\n", ginfo->name);

    /* Show the list of a directory */
    if (!strcasecmp(mode, "list")) {
      if (!node) {
        if (!list_dir(fosfat, "/"))
          res = -1;
      }
      else if (node) {
        if (!list_dir(fosfat, node))
          res = -1;
        free(node);
      }
    }

    /* Get a file from the disk */
    else if (!strcmp(mode, "get") && node) {
      get_file(fosfat, node, path ? path : "./");
      free(node);
      free(path);
    }
    else
      print_info();

    free(ginfo);
  }

  /* Close the disk */
  fosfat_close(fosfat);
  free(device);
  free(mode);

  return res;
}
