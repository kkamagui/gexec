/*
 * Name        : autocomp
 * Description : Auto-completion library
 * Version     : v0.4
 * 
 * Author      : Ferry Boender  <f.boender@electricmonk.nl>
 * Copyright   : (C) 2004, Ferry Boender
 * License     : General Public License. For more information and terms, see
 *               the COPYING file in this archive.
 *
 * Usage       : To compile:
 *                 /path-to-src/$ make
 *               To test:
 *                 /path-to-src/$ ./autocomp
 *               Or
 *                 /path-to-src/$ ./autocomp [argument]
 *                 
 *                 where argument is a (relative/absolute) command to be 
 *                 completed
 * Dependencies: glib
 * Purpose     : Provide a library which can be linked against any application
 *               and be used to perform autocompletion on file system oriented
 *               commands.
 * Revisions   : v0.2 [FB] First public release.
 *               v0.3 [FB] Whoops, the .h file got deleted,
 *               v0.4 [FB] Function names now shown in debug output,
 *                         Added ac_list_print function.
 *                         Added various sanity checks for NULL lists/etc.
 *                         Changed test cases in main().
 *               v0.5 [FB] Forgot to add ac_list_print to the .h file.
 *                         Removed the teststub from the main lib and placed it
 *                         in a seperate program called 'test'.
 * Remarks     : -
 * Todo        : -
 *
 */
#ifndef __AUTOCOMP_H__
#define __AUTOCOMP_H__

#include <stdio.h>
#include <stdlib.h>
#include <glib.h>
#include <string.h>
#include <dirent.h>

//#define _DEBUG

/* Name    : ac_path_get
 * Params  : -
 * Returns : A glib GList containing the broken down values in the PATH 
 *           environment variable.
 * Remarks : -
 */
GList *ac_path_get (void);

/* Name    : ac_list_free
 * Params  : A glib GList containing in the data section an hand-allocated 
 *           memory part.
 * Returns : NULL, the data and the GList itself are freed
 * Remarks : -
 */
GList *ac_list_free (GList *ac_list);

/* Name    : ac_list_get
 * Params  : A completion query which should be completed. Examples:
 *           "au"   (complete in current dir and path)
 *           "/et"  (complete in root dir)
 *           "../a" (comlete in parent dir)
 * Returns : A glib GList containing the possibilities for completing the qry.
 *           Example: 
 *           completion_qry = "au", current dir has file "autocomplete.c" and 
 *           "autocomplete". List is "ocomplete.c", "ocomplete".
 * Remarks : Use ac_list_free to deep-free mem used by list.
 */
GList *ac_list_get (char *completion_qry);

/* Name    : ac_list_print
 * Params  : ac_list: a glib GList containing the matches for a ompletion query
 *           completion_qry: A completion query which will only be used to 
 *           prepend it to the completion results.
 * Returns : -
 * Remarks : The matches that are printed are prepended with the command_qry, but
 *           the matches themself do not actually contain this prefix. 
 *           Ex: ac_list_print (list, "autoc") 
 *               prints 'autocomp' where 'autoc' is the completion query and 
 *               'omp' is the match.
 */
void ac_list_print (GList *ac_list, char *completion_qry);

#endif /* __AUTOCOMP_H__ */
