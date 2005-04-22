/*
 * Name        : autocomp
 * Description : Auto-completion library
 * Version     : v0.5
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

#include "autocomp.h"

//#define _DEBUG

/* Name    : ac_path_get
 * Params  : -
 * Returns : A glib GList containing the broken down values in the PATH 
 *           environment variable.
 * Remarks : -
 */
GList *ac_path_get (void) {
	gchar *env_path = NULL;
	gchar **split_env_paths = NULL;
	gchar *path = NULL;
	GList *paths = NULL;
	int i = 0;
	
	if ((env_path = getenv("PATH")) == NULL) {
		return (NULL);
	}
	
	split_env_paths = g_strsplit (env_path, ":", -1);

	while ((path = split_env_paths[i]) != NULL) {
		paths = g_list_append (paths, strdup(path));

		i++;
	}

	g_strfreev (split_env_paths);

	return (paths);
}

/* Name    : ac_list_free
 * Params  : A glib GList containing in the data section an hand-allocated 
 *           memory part.
 * Returns : NULL, the data and the GList itself are freed
 * Remarks : -
 */
GList *ac_list_free (GList *ac_list) {
	GList *cur = NULL;

	if (ac_list == NULL) {
#ifdef _DEBUG
	printf ("ac_list_free: ac_list = NULL\n");
#endif
		return (NULL);
	}
	
	cur = ac_list;
	while (cur != NULL) {
		free (cur->data);
		cur = g_list_next(cur);
	}

	g_list_free (ac_list);
	
	return (NULL);
}

/* Name    : ac_list_get
 * Params  : A completion query which should be completed. Examples:
 *           "au"   (complete in current dir and path)
 *           "/et"  (complete in root dir)
 *           "../a" (comlete in parent dir)
 *           completionn_qry must be a pointer to a valid zero-terminated string.
 * Returns : A glib GList containing the possibilities for completing the qry.
 *           Example: 
 *           completion_qry = "au", current dir has file "autocomplete.c" and 
 *           "autocomplete". List is "ocomplete.c", "ocomplete".
 * Remarks : Use ac_list_free to deep-free mem used by list.
 */
GList *ac_list_get (char *completion_qry) {
	GList *paths = NULL;
	GList *cur = NULL;
	GList *match = NULL;
	char *command;
	
	if (completion_qry == NULL) {
#ifdef _DEBUG
	printf ("ac_list_get: completion_qry = NULL\n");
#endif
		return (NULL);
	}
	
	if (strchr(completion_qry, '/')) {
		/* Absolute or relative path: Search relative path */
		char *path_and_file = NULL;
		char *path = NULL;
		char *file = NULL;
		char *last_slash = NULL;

		path_and_file = strdup(completion_qry);
		
		last_slash = strrchr(path_and_file, '/');
		last_slash[0] = '\0';

		if (completion_qry[0] == '/' && strlen(path_and_file) == 0) {
			path = strdup("/");
		} else {
			path = strdup(path_and_file); /* Before last slash */
		}
		file = strdup(++last_slash);  /* After last slash */

		free (path_and_file);

		paths = g_list_append (paths, strdup(path));
		command = strdup(file);

		free (file);
		free (path);
		
	} else {
		/* Non-relative path: search current dir and PATH */
		command = strdup(completion_qry);
		
		/* Get all paths to search in */
		paths = ac_path_get();
		paths = g_list_append (paths, strdup("."));
	}
	
#ifdef _DEBUG
	printf ("ac_list_get: command: %s\n", command);

	cur = g_list_first (paths);
	while (cur != NULL) {
		printf ("ac_list_get: paths: %s\n", (char *)cur->data);
		cur = cur->next;
	}
	cur = NULL;
#endif
	
	/* Check all paths to see if they have matching commands */
	cur = g_list_first(paths);
	while (cur != NULL) {
		DIR *dir = NULL;
		struct dirent *direntry;

		dir = opendir ((char *)cur->data);
		if (dir != NULL) {
			while ((direntry = readdir(dir)) != NULL) {
				if ((strcmp(direntry->d_name, ".") != 0) &&
					(strcmp(direntry->d_name, "..") != 0) &&
					(strncmp(direntry->d_name, command, strlen(command)) == 0)) {
					
#ifdef _DEBUG
	printf ("ac_list_get: match: %s\n", direntry->d_name);
#endif
					match = g_list_append (match, g_strdup(direntry->d_name+strlen(command)));
				}
			}
		}
		
		free (cur->data); /* Free data in list */
		cur = g_list_next(cur);
	}

	g_list_free (paths); /* Free list */
	
	free (command);
	
	return (match);
}

void ac_list_print (GList *ac_list, char *completion_qry) {
	GList *cur = NULL;

	if (ac_list == NULL) {
		return;
	}
	
	cur = g_list_first(ac_list);

	while (cur != NULL) {
		printf ("ac_list_print: %s%s\n", completion_qry, (char *)cur->data);
		cur = g_list_next(cur);
	}
}
