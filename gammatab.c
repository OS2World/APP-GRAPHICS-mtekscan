/*
   gammatab.c

   Gamma table handling for mtekscan.

   Copyright (c) 1996,1997 Jan Schoenepauck / Fast Forward Productions
   <schoenep@uni-wuppertal.de>
            
   $Id: gammatab.c 1.1 1997/09/13 02:45:10 parent Exp $
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#include "config.h"
#include "global.h"
#include "options.h"
#include "gammatab.h"

/* One gamma table for each color component */
unsigned char *gt_r;
unsigned char *gt_g;
unsigned char *gt_b;

int gt_entries = 256;      /* number of entries in gamma table; valid     */
                           /* values are 256, 1024, 4096 or 65536         */

/*------------------------------------------------------------------------*/
/* void init_gamma_tables (int entries)                                   */
/* Allocate gamma table memory and initialize tables to a linear mapping. */
/* <entries> specifies number of values in gamma table.                   */
/*------------------------------------------------------------------------*/
void init_gamma_tables (int entries) {
  int gt_entrylength = (gt_entries == 256) ? 1 : 2;
  int i;
  gt_entries = entries;
  gt_r = (unsigned char *)malloc(gt_entrylength * gt_entries);
  gt_g = (unsigned char *)malloc(gt_entrylength * gt_entries);
  gt_b = (unsigned char *)malloc(gt_entrylength * gt_entries);
  for (i = 0; i < gt_entries; i++) {
    if (gt_entries == 256) {
      gt_r[i] = (unsigned char)i;
      gt_g[i] = (unsigned char)i;
      gt_b[i] = (unsigned char)i;
    }
    else {
      gt_r[i * 2] = (i & 0xff00) >> 8;
      gt_r[i * 2 + 1] = (i & 0xff);
      gt_g[i * 2] = (i & 0xff00) >> 8;
      gt_g[i * 2 + 1] = (i & 0xff);
      gt_b[i * 2] = (i & 0xff00) >> 8;
      gt_b[i * 2 + 1] = (i & 0xff);
    }
  }
}


/*------------------------------------------------------------------------*/
/* void free_gamma_tables (void)                                          */
/* Free the memory allocated for the gamma tables.                        */
/*------------------------------------------------------------------------*/
void free_gamma_tables (void) {
  free(gt_r);
  free(gt_g);
  free(gt_b);
}


/*------------------------------------------------------------------------*/
/* Invert the data in <gtable> (for negative scanning)                    */
/*------------------------------------------------------------------------*/
void invert_gamma_table (unsigned char *gtable) {
  int i, g;
  int gt_entrylength = (gt_entries == 256) ? 1 : 2;
  for (i = 0; i < gt_entries * gt_entrylength; i += gt_entrylength) {
    if (gt_entries == 256) {
      gtable[i] = 255 - gtable[i];
    }
    else {
      g = mt_GammaValueMax - (256 * gtable[i] + gtable[i + 1]);
      gtable[i] = (g & 0xff00) >> 8;
      gtable[i + 1] = g & 0xff;
    }
  }
}


/*------------------------------------------------------------------------*/
void create_gamma_table (unsigned char *gtable, float gamma) {
  float maxval = (float)(gt_entries - 1);
  float raw, invgamma;
  int i, cnv;
  int negative = 0;

  if (gamma < 0) {
    gamma = -gamma;
    negative = 1;
  }
  invgamma = 1.0 / gamma;
  for (i = 0; i < gt_entries; i++) {
    raw = (float)i / maxval;
    cnv = (int)(maxval * pow(raw, invgamma));
    if (cnv < 0) cnv = 0;
    if (cnv > (gt_entries - 1)) cnv = gt_entries - 1;

    if (gt_entries == 256)
      gtable[i] = (unsigned char)cnv;
    else {
      gtable[i] = (cnv & 0xff00) >> 8;
      gtable[i + 1] = cnv & 0xff;
    }
  }
  if (negative) invert_gamma_table(gtable);
}


/*------------------------------------------------------------------------*/
int write_gamma_tables (char *path) {
  FILE *f;
  int i, j, gamma;
  unsigned char *gtable;

  f = fopen(path, "w");
  if (f == NULL) {
    perror("write_gamma_table");
    return(RET_FAIL);
  }
  fprintf(f, "GT\n# mtekscan gamma table\nRGB\n%d\n", gt_entries);
  for (i = 0; i < 3; i++) {
    switch (i) {
    case 0:
      gtable = gt_r;  break;
    case 1:
      gtable = gt_g;  break;
    case 2:
    default :
      gtable = gt_b; break;
    }
    for (j = 0; j < gt_entries; j++) {
      gamma = (gt_entries == 256) ? gtable[j] :
              (256 * gtable[j * 2] + gtable[j * 2 + 1]);
      fprintf(f, "%d%s", gamma, (j == gt_entries - 1) ? "" : ",");
      if (j % 10 == 9) fprintf(f, "\n");
    }
    fprintf(f, "\n\n");
  }
  fclose(f);
  return(RET_SUCCESS);
}


/*------------------------------------------------------------------------*/
/* Grab next line from file <f>, ignore blank lines and leading #'s,      */
/* return data in <buffer> with leading spaces skipped and trailing \n    */
/* set to 0.                                                              */
/*------------------------------------------------------------------------*/
int get_next_line (FILE *f, char *buffer) {
  char *ret;
  char tbuffer [RBUFSIZE];
  int i;
  do {
    ret = fgets(tbuffer, RBUFSIZE, f);
    if (ret == NULL) return(RET_FAIL);
    for (i = 0; isspace(ret[i]) && (i <= strlen(ret)); i++);
  } while ((ret[i] == '#') || (ret[i] == '\n') || (ret[i] == 0));
  strncpy(buffer, &ret[i], strlen(&ret[i]));
  for (i = 0; buffer[i] != '\n'; i++);
  buffer[i] = 0;
  return(RET_SUCCESS);
}


/*------------------------------------------------------------------------*/
int read_gamma_tables (char *path) {
  char buffer [RBUFSIZE];
  FILE *f;
  int i, j;
  int gamma;
  int ntables;
  int size;
  char *substr;
  unsigned char *gtable;

  f = fopen(path, "r");
  if (f == NULL) {
    perror("read_gamma_table");
    return(RET_FAIL);
  }
  if (get_next_line(f, buffer) == RET_FAIL) {
    fprintf(stderr, "read_gamma_table: Unexpected end of file.\n");
    fclose(f);
    return(RET_FAIL);
  }
  if (strcmp(buffer, "GT")) {
    fprintf(stderr, "read_gamma_table: Invalid file type.\n");
    fclose(f);
    return(RET_FAIL);
  }
  if (get_next_line(f, buffer) == RET_FAIL) {
    fprintf(stderr, "read_gamma_table: Unexpected end of file.\n");
    fclose(f);
    return(RET_FAIL);
  }
  if (strcmp(buffer, "RGB") == 0)    ntables = 3;
  else if (strcmp(buffer, "C") == 0) ntables = 1;
  else {
    fprintf(stderr, "read_gamma_table: Invalid file type.\n");
    fclose(f);
    return(RET_FAIL);
  }
  if (get_next_line(f, buffer) == RET_FAIL) {
    fprintf(stderr, "read_gamma_table: Unexpected end of file.\n");
    fclose(f);
    return(RET_FAIL);
  }
  size = atoi(buffer);
  if ((size != 256) && (size != 1024) && (size != 4096) && (size != 65536)) {
    fprintf(stderr, "read_gamma_table: Illegal table size value (%d).\n", 
                     size);
    fclose(f);
    return(RET_FAIL);
  }
  if (size > mt_MaxLookupTableSize) {
    fprintf(stderr, "read_gamma_table: Table size of %d not supported by "
                    "scanner.\n", size);
    fclose(f);
    return(RET_FAIL);
  }
  init_gamma_tables(size);;
  for (i = 0; i < ntables; i++) {
    switch (i) {
    case 0 :
      gtable = gt_r;  break;
    case 1 :
      gtable = gt_g;  break;
    case 2 :
    default :
      gtable = gt_b;  break;
    }
    if (get_next_line(f, buffer) == RET_FAIL) {
      fprintf(stderr, "read_gamma_table: Unexpected end of file.\n");
      fclose(f);
      free_gamma_tables();
      return(RET_FAIL);
    }
    substr = strtok(buffer, ",");
    for (j = 0; j < size; j++) {
      while (substr == NULL) {
        if (get_next_line(f, buffer) == RET_FAIL) {
          fprintf(stderr, "read_gamma_table: Unexpected end of file.\n");
          fclose(f);
          free_gamma_tables();
          return(RET_FAIL);
        }
        substr = strtok(buffer, ",");
      }
      gamma = atoi(substr);
      if ((gamma < 0) || (gamma > gt_entries - 1)) {
        fprintf(stderr, "read_gamma_table: Gamma value out of range (%d).\n",
                gamma);
        fclose(f);
        free_gamma_tables();
        return(RET_FAIL);
      }
      if (gt_entries == 256)
        gtable[j] = gamma & 0xff;
      else {
        gtable[j * 2] = (gamma & 0xff00) >> 8;
        gtable[j * 2 + 1] = gamma & 0xff;
      }
      substr = strtok(NULL, ",");
    }
  }
  fclose(f);
  if (ntables == 1) {
    memcpy(gt_g, gt_r, gt_entries * ((gt_entries == 256) ? 1 : 2));
    memcpy(gt_b, gt_r, gt_entries * ((gt_entries == 256) ? 1 : 2));
  }
  return(RET_SUCCESS);
}
