/*
   gammatab.h

   Header file for gammatab.c (gamma table handling for mtekscan).

   Copyright (c) 1996,1997 Jan Schoenepauck / Fast Forward Productions
   <schoenep@uni-wuppertal.de>
            
   $Id: gammatab.h 1.1 1997/09/13 02:45:10 parent Exp $
*/

#ifndef _GAMMATAB_H
#define _GAMMATAB_H

#define RBUFSIZE 4096   /* Line buffer for read_gamma_tables() */

/* One gamma table for each color component */
extern unsigned char *gt_r;
extern unsigned char *gt_g;
extern unsigned char *gt_b;

extern int gt_entries;

extern void init_gamma_tables (int entries);
extern void free_gamma_tables (void);
extern void invert_gamma_table (unsigned char *gtable);
extern void create_gamma_table (unsigned char *gtable, float gamma);
extern int write_gamma_tables (char *path);
extern int read_gamma_tables (char *path);
#endif
