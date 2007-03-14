/***************************************************************************
 *
 * gExec
 *
 * A small GTK program launcher.
 *
 * Author  : Ferry Boender <f DOT boender AT electricmonk DOT nl>
 * License : GPL, General Public License
 * Todo    : - Autocomplete doesn't work mid-command
 *           - Globbing doesn't work (* and ?)
 *           - Tons of memory leaks
 *           - Ugly code
 *           
 * Copyright (C) 2004 Ferry Boender.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 * 
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <assert.h>
#include <unistd.h>

#include "autocomp.h"

#define OPT_TERM 1 // Run the command in a terminal
#define OPT_SU   2 // Run the command as root

#define DEFAULT_CMD_TERMEMU "xterm -e %s"
#define DEFAULT_CMD_SU "gksu \"%s\""
#define DEFAULT_HISTORY_MAX 20

static GtkWidget *win_main;

static gboolean keepopen = FALSE;

struct _task {
	char *command;                // Current command
	int options;                  // Options (OPT_)
	char *cmd_termemu;            // Terminal emulator 
	char *cmd_su;                 // Root privilege giver 
	int history_max;              // Maximum number of history items to keep 
	char *tabcomp_command;        // Command at time of autocompletion start 
	GList *tabcomp_matches;       // Possible completions for tabcomp_command 
	GList *tabcomp_current_match; // Current match for tabcomp_command 
	GList *history;               // List of previous commands 
};

struct _settings {
	char *cmd_termemu; // Terminal emulator string to use 
	char *cmd_su;      // SuperUser program to use 
	int history_max;   // Maximum number of commands to keep in history list 
};

struct _task *task_new() {
	struct _task *ret_task = NULL;

	ret_task = malloc(sizeof(struct _task));

	ret_task->command = NULL;
	ret_task->options = 0;
	ret_task->cmd_termemu = NULL;
	ret_task->cmd_su = NULL;
	ret_task->tabcomp_matches = NULL;
	ret_task->tabcomp_current_match = NULL;
	ret_task->history = NULL;

	return(ret_task);
}

GOptionEntry entries[] = 
{
  { "keepopen", 'k', 0, G_OPTION_ARG_NONE, &keepopen, "Keep gExec open after executing a command", NULL },
  { NULL }
};

void task_option_toggle(struct _task *task, int option) {
	task->options  ^= option;
}

void task_command_set(struct _task *task, char *command) {

	assert(task != NULL);
	assert(command != NULL);

	if (task->command != NULL) {
		free(task->command);
	}

	task->command = strdup(command);
}

void task_cmd_termemu_set(struct _task *task, char *cmd_termemu) {

	assert(task != NULL);
	assert(cmd_termemu != NULL);

	if (task->cmd_termemu != NULL) {
		free(task->cmd_termemu);
	}

	task->cmd_termemu = strdup(cmd_termemu);
}

void task_cmd_su_set(struct _task *task, char *cmd_su) {

	assert(task != NULL);
	assert(cmd_su != NULL);

	if (task->cmd_su != NULL) {
		free(task->cmd_su);
	}

	task->cmd_su = strdup(cmd_su);
}

void task_dump(struct _task *task) {
	printf("task_dump: struct _task task(@%p):\n", task);
	printf("\tcommand         : %s\n", task->command);
	printf("\toptions         : %i\n", task->options);
	printf("\tcmd_termemu     : %s\n", task->cmd_termemu);
	printf("\tcmd_su          : %s\n", task->cmd_su);
	printf("\thistory_max     : %i\n", task->history_max);
	printf("\ttabcomp_command : %s\n", task->tabcomp_command);
}

void settings_create() {
	FILE *f_out = NULL;
	char *filename = NULL;

	filename = (char *)malloc(sizeof(char) * (strlen(getenv("HOME")) + 15 + 1) );

	sprintf(filename, "%s/.gexec", getenv("HOME"));

	f_out = fopen(filename, "w");
	if (f_out != NULL) { 

		fprintf(f_out, "cmd_termemu=%s\n", DEFAULT_CMD_TERMEMU);
		fprintf(f_out, "cmd_su=%s\n", DEFAULT_CMD_SU);
		fprintf(f_out, "history_max=%i\n", DEFAULT_HISTORY_MAX);

		fclose(f_out);
	}

	free(filename);

	return;
}

struct _settings *settings_read() {
	FILE *f_in = NULL;
	char *filename = NULL;
	struct _settings *settings = NULL;
	char line[4096];
	
	settings = malloc(sizeof(struct _settings));

	settings->cmd_termemu = DEFAULT_CMD_TERMEMU;
	settings->cmd_su = DEFAULT_CMD_SU;
	settings->history_max = DEFAULT_HISTORY_MAX;
	
	filename = (char *)malloc(sizeof(char) * (strlen(getenv("HOME")) + 15 + 1) );
	sprintf(filename, "%s/.gexec", getenv("HOME"));

	f_in = fopen(filename, "r");

	if (f_in != NULL) { 
		while (fgets(line, 4096, f_in)) {
			char *newline_pos;
			char **tokens;
			
			newline_pos = strchr(line, '\n');
			newline_pos[0] = '\0';

			tokens = g_strsplit(line, "=", 2);

			if (strcmp(tokens[0], "cmd_termemu") == 0 && tokens[1] != NULL) {
				settings->cmd_termemu = strdup(tokens[1]);
			}
			if (strcmp(tokens[0], "cmd_su") == 0 && tokens[1] != NULL) {
				settings->cmd_su = strdup(tokens[1]);
			}
			if (strcmp(tokens[0], "history_max") == 0 && tokens[1] != NULL) {
				settings->history_max = atoi(tokens[1]);
			}

			g_strfreev(tokens);
		}

		fclose(f_in);
	} else {
		// No settings yet; create default settings
		settings_create();
	}

	free(filename);
	
	return(settings);
}

GList *history_read() {
	FILE *f_in = NULL;
	char *filename = NULL;
	GList *history = NULL;
	char line[4096];
	
	filename = (char *)malloc(sizeof(char) * (strlen(getenv("HOME")) + 15 + 1) );
	sprintf(filename, "%s/.gexec_history", getenv("HOME"));

	f_in = fopen(filename, "r");
	if (f_in != NULL) {

		while (fgets(line, 4096, f_in)) {
			char *newline_pos;
			
			newline_pos = strchr(line, '\n');
			newline_pos[0] = '\0';

			history = g_list_append(history, strdup(line));
		}

		fclose(f_in);
	}

	free(filename);
	
	return(history);
}

int history_write(GList *history, int history_max) {
	char *filename = NULL;
	FILE *f_out;
	GList *cur = NULL;
	int count = 0;
	int err = 0;

	filename = (char *)malloc(sizeof(char) * (strlen(getenv("HOME")) + 15 + 1) );
	sprintf(filename, "%s/.gexec_history", getenv("HOME"));

	cur = history;

	f_out = fopen(filename, "w");

	if (f_out != NULL) {
	
		while (cur != NULL && count != history_max) {
			fprintf(f_out, "%s\n", (char *)cur->data);
			cur = g_list_next(cur);
			count++;
		}

		fclose(f_out);
	} else {
		err = -1;
	}

	free(filename);

	return(err);
}

void task_history_add(struct _task *task, char *command) {
	GList *new_history = NULL;
	GList *iter = NULL;

	assert(task != NULL);
	assert(command != NULL);

	new_history = g_list_append(new_history, strdup(command));

	iter = task->history;
	while (iter != NULL) {
		// Skip duplicates of 'command'
		if (strcmp(iter->data, command) != 0) {
			new_history = g_list_append(new_history, strdup(iter->data));
		}

		free(iter->data);

		iter = g_list_next(iter);
	}

	g_list_free(task->history);
	task->history = new_history;
}

// -1 : Invalid command syntax
// -2 : Invalid termemu string syntax 
// -3 : Invalid su string syntax
// -4 : Failure to execute command (see errormsg)
// otherwise returns 0
int cmd_run(struct _task *task, char **errormsg) {
	char *final_command = NULL;
	int argc;
	char **argv;
	GError *shellparse_err = NULL;
	int err = 0;
	
	assert(task->command != NULL);
	assert(task->cmd_termemu != NULL);

	// Handle execution options. If any options are set, wrap the current
	// command in the appropriate command.
	final_command = strdup(task->command);

	if ((task->options & OPT_TERM) == OPT_TERM) {
		int buf_size = (strlen(final_command) + strlen(task->cmd_termemu));
		char *tmp_command  = malloc( sizeof(char) * (buf_size + 1) );
		
		if (snprintf(tmp_command, buf_size, task->cmd_termemu, final_command) >= buf_size) {
			err = -2; // Invalid termemu string
		} else {
			free(final_command);
			final_command = strdup(tmp_command);
		}

		free(tmp_command);
	}
	if ((task->options & OPT_SU) == OPT_SU) {
		int buf_size = (strlen(final_command) + strlen(task->cmd_su));
		char *tmp_command = malloc(sizeof(char) * (buf_size + 1) );
		
		if (snprintf(tmp_command, buf_size, task->cmd_su, final_command) >= buf_size) {
			err = -3; // Invalid su string 
		} else {
			free(final_command);
			final_command = strdup(tmp_command);
		}

		free(tmp_command);
	}


	// Split up commandline string for execution
	if (g_shell_parse_argv(final_command, &argc, &argv, &shellparse_err) == FALSE) {
		err = -1; // Invalid command syntax
	}
	
	if (err == 0) {
		GPid child_pid;
		GError *error = NULL;

		argv[argc] = NULL;

		if (!g_spawn_async (NULL, argv, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL, &child_pid, &error)) {
			err = -4;
			*errormsg = strdup(error->message);
			g_error_free(error);
		}
	}

	free(final_command);

	return(err);
}

void cmd_complete_start(struct _task *task) {
	int argc;
	char **argv;
	GError *shellparse_err = NULL;
	char *last_arg = NULL;

	if (task->command == NULL) {
		return;
	}
	
	// Split up command to get last command for completion
	if (g_shell_parse_argv(task->command, &argc, &argv, &shellparse_err) == FALSE) {
		return;
	}
	last_arg = argv[argc-1];

	task->tabcomp_command = strdup(task->command); 
	task->tabcomp_matches = ac_list_get(last_arg); // Possible appends for tabcomp_command

	if (g_list_length(task->tabcomp_matches) > 0) {
		task->tabcomp_current_match = task->tabcomp_matches;
	} else {
		task->tabcomp_matches = ac_list_free(task->tabcomp_matches);
		free(task->tabcomp_command);
	}
}

void cmd_complete_next(struct _task *task) {
	if (task->tabcomp_matches == NULL) {
		return;
	}
	
	// Cycle forwards through the list of matches
	if (task->tabcomp_current_match == g_list_last(task->tabcomp_matches)) {
		task->tabcomp_current_match = g_list_first(task->tabcomp_matches);
	} else {
		task->tabcomp_current_match = g_list_next(task->tabcomp_current_match);
	}
}

void cmd_complete_prev(struct _task *task) {
	// FIXME: Not yet completed
}

void cmd_complete_cancel(struct _task *task) {

	if (task->tabcomp_matches == NULL) {
		return;
	}
	
	free(task->tabcomp_command);
	ac_list_free(task->tabcomp_matches);
	task->tabcomp_matches = NULL;
}

void ui_error(char *message) {
	GtkWidget *dialog;
	
	dialog = gtk_message_dialog_new(NULL, 
			GTK_DIALOG_DESTROY_WITH_PARENT, 
			GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, 
			"Error: %s", 
			message);

	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
}

void ui_cmd_run(struct _task *task) {
	int err = 0;
	char *errormsg = NULL;

	err = cmd_run(task, &errormsg);

	switch (err) {
		case -1:
			ui_error("Invalid command syntax.");
			break;
		case -2:
			ui_error("Possible invalid Terminal Emulator command.");
			break;
		case -3:
			ui_error("Possible invalid SU command.");
			break;
		case -4:
			ui_error(errormsg);
			free(errormsg);
			break;
		default:
			task_history_add(task, task->command);
			history_write(task->history, task->history_max);
			if (!keepopen) {
				gtk_main_quit();
			}
			break;
	}
}

// Main window callbacks 
void ui_win_main_cb_delete_event(GtkWidget *widget, void *data) {
	gtk_main_quit();
}

// Command callbacks 
void ui_combo_command_cb_activate(GtkWidget *widget, struct _task *task) {
	task_command_set(task, (char *)gtk_entry_get_text(GTK_ENTRY(widget)));

	ui_cmd_run(task);
}

gboolean ui_combo_command_cb_key_press_event(GtkWidget *widget, 
		GdkEventKey *event, struct _task *task) {

	if (event->keyval == GDK_Escape) {
		// Abort gexec
		gtk_main_quit();
	} else
	if (event->keyval == GDK_Tab) {
		// Tab completion 

		if (task->tabcomp_matches == NULL) {
			cmd_complete_start(task);
		} else {
			cmd_complete_next(task);
		}

		if (task->tabcomp_matches != NULL) {
			char *new_entry = NULL;
			
			new_entry = malloc(sizeof(char) * (strlen(task->tabcomp_command) + strlen(task->tabcomp_current_match->data) + 1 ) );
			
			sprintf(new_entry, "%s%s", task->tabcomp_command, (char *)task->tabcomp_current_match->data);
			
			gtk_entry_set_text(GTK_ENTRY(widget), new_entry);
			gtk_entry_set_position(GTK_ENTRY(widget), -1);

			free(new_entry);
		} else {

		}
		return(TRUE); // Signal has been handled, do not pass tab key further 
	} else {
		// Normal keypresses 

		// Accept/Clear tabcompletion 
		task_command_set(task, (char *)gtk_entry_get_text(GTK_ENTRY(widget)));
		cmd_complete_cancel(task);
	}

	return(FALSE);
}
void ui_combo_command_cb_changed(GtkWidget *widget, struct _task *task) {
	task_command_set(task, (char *)gtk_entry_get_text(GTK_ENTRY(widget)));
}

// Option callbacks 
void ui_chk_term_cb_toggled(GtkWidget *widget, struct _task *task) {
	task_option_toggle(task, OPT_TERM);
}
void ui_chk_su_cb_toggled(GtkWidget *widget, struct _task *task) {
	task_option_toggle(task, OPT_SU);
}

// Button callbacks 
void ui_btn_ok_cb_clicked(GtkWidget *widget, struct _task *task) {
	ui_cmd_run(task);
}

void ui_btn_cancel_cb_clicked(GtkWidget *widget, void *data) {
	gtk_main_quit();
}

int main(int argc, char *argv[]) {
	GOptionContext *context = NULL;
	GError *error = NULL;
	struct _task *task = NULL;
	struct _settings *settings = NULL;
	GtkWidget *chk_term, *chk_su;
	GtkWidget *lbl_command;
	GtkWidget *combo_command;
	GtkWidget *vbox_main, *hbox_command, *hbox_options, *hbox_buttons;
	GtkWidget *btn_ok, *btn_cancel;
	
	// Handle commandline options
	context = g_option_context_new ("- Interactive run dialog");
	g_option_context_add_main_entries (context, entries, NULL);
	g_option_context_add_group (context, gtk_get_option_group (TRUE));
	if (!g_option_context_parse (context, &argc, &argv, &error)) {
		fprintf(stderr, "%s\n", error->message);
		g_error_free(error);
		exit(1);
	}
  
	task = task_new();
	task->history = history_read();
	
	settings = settings_read();
	task->cmd_termemu = settings->cmd_termemu;
	task->cmd_su = settings->cmd_su;
	task->history_max = settings->history_max;

	gtk_init(&argc, &argv);

	win_main = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(win_main), "gExec");
	gtk_window_set_default_size(GTK_WINDOW(win_main), 400, 77);
	gtk_signal_connect(GTK_OBJECT(win_main),
			"delete_event",
			GTK_SIGNAL_FUNC(ui_win_main_cb_delete_event),
			NULL);

	// Command 
	hbox_command = gtk_hbox_new(FALSE, 0);
	lbl_command = gtk_label_new("Run command:");
	combo_command = gtk_combo_new();
	gtk_combo_disable_activate(GTK_COMBO(combo_command));

	gtk_signal_connect(GTK_OBJECT(GTK_COMBO(combo_command)->entry),
			"activate",
			GTK_SIGNAL_FUNC(ui_combo_command_cb_activate),
			task);
	gtk_signal_connect(GTK_OBJECT(GTK_COMBO(combo_command)->entry),
			"key-press-event",
			GTK_SIGNAL_FUNC(ui_combo_command_cb_key_press_event),
			task);
	gtk_signal_connect(GTK_OBJECT(GTK_COMBO(combo_command)->entry),
			"changed",
			GTK_SIGNAL_FUNC(ui_combo_command_cb_changed),
			task);

	if (task->history != NULL) {
		gtk_combo_set_popdown_strings(GTK_COMBO(combo_command), task->history);
	}
	
	gtk_box_pack_start(GTK_BOX(hbox_command), lbl_command, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox_command), combo_command, TRUE, TRUE, 0);

	// Options 
	hbox_options = gtk_hbox_new(TRUE, 5);
	chk_term = gtk_check_button_new_with_mnemonic("Run in _terminal emulator");
	gtk_signal_connect(GTK_OBJECT(chk_term),
			"toggled",
			GTK_SIGNAL_FUNC(ui_chk_term_cb_toggled),
			task);
	gtk_box_pack_start(GTK_BOX(hbox_options), chk_term, FALSE, FALSE, 0);
	
	chk_su = gtk_check_button_new_with_mnemonic("Run as _root");
	gtk_signal_connect(GTK_OBJECT(chk_su),
			"toggled",
			GTK_SIGNAL_FUNC(ui_chk_su_cb_toggled),
			task);
	gtk_box_pack_start(GTK_BOX(hbox_options), chk_su, FALSE, FALSE, 0);

	// Buttons 
	hbox_buttons = gtk_hbox_new(FALSE, 0);

	btn_ok = gtk_button_new_from_stock(GTK_STOCK_OK);
	btn_cancel = gtk_button_new_from_stock(GTK_STOCK_CANCEL);

	gtk_signal_connect(GTK_OBJECT(btn_ok),
			"clicked",
			GTK_SIGNAL_FUNC(ui_btn_ok_cb_clicked),
			task);
	
	gtk_signal_connect(GTK_OBJECT(btn_cancel),
			"clicked",
			GTK_SIGNAL_FUNC(ui_btn_cancel_cb_clicked),
			NULL);
	gtk_box_pack_start(GTK_BOX(hbox_buttons), btn_ok, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(hbox_buttons), btn_cancel, TRUE, TRUE, 0);

	// Main 
	vbox_main = gtk_vbox_new(FALSE, 0);
	
	gtk_box_pack_start(GTK_BOX(vbox_main), hbox_command, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox_main), hbox_options, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox_main), hbox_buttons, TRUE, TRUE, 0);

	gtk_container_add(GTK_CONTAINER(win_main), vbox_main);
	gtk_container_set_border_width(GTK_CONTAINER(win_main), 5);
	
	// Booom! 
	gtk_widget_show_all(win_main);
	gtk_widget_grab_focus(combo_command);

	gtk_main();

	free(task);

	return(0);
}
