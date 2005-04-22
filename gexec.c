/****************************************************************************
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

#define OPT_TERM 1
#define OPT_SU   2

#define DEFAULT_CMD_TERMEMU "xterm -e %s"
#define DEFAULT_CMD_SU "gksu \"%s\""
#define DEFAULT_HISTORY_MAX 20

struct _task {
	char *command;                /* Current command */
	int options;                  /* Options (run in term, etc..) */
	char *cmd_termemu;            /* Terminal emulator */
	char *cmd_su;                 /* Root privilege giver */
	int history_max;          /* Maximum number of history items to keep */
	char *tabcomp_command;        /* Command at time of autocompletion start */
	GList *tabcomp_matches;       /* Possible completions for tabcomp_command */
	GList *tabcomp_current_match; /* Current match for tabcomp_command */
	GList *history;               /* List of previous commands */
};

struct _settings {
	char *cmd_termemu; /* Terminal emulator string to use */
	char *cmd_su;      /* SuperUser program to use */
	int history_max;   /* Maximum number of commands to keep in history list */
};

GtkWidget *win_main;

void settings_create() {
	FILE *f_out;
	char *filename = NULL;

	filename = (char *)malloc(sizeof(char) * (strlen(getenv("HOME")) + 15 + 1) );
	sprintf(filename, "%s/.gexec", getenv("HOME"));

	f_out = fopen(filename, "w");
	if (f_out == NULL) {
		free(filename);
		return;
	}

	fprintf(f_out, "cmd_termemu=%s\n", DEFAULT_CMD_TERMEMU);
	fprintf(f_out, "cmd_su=%s\n", DEFAULT_CMD_SU);
	fprintf(f_out, "history_max=%i\n", DEFAULT_HISTORY_MAX);

	fclose(f_out);

	free(filename);

	return;
}

struct _settings *settings_read() {
	FILE *f_in;
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
	if (f_in == NULL) {
		settings_create();
		free(filename);
		return(settings);
	}

			
	while (fgets(line, 4096, f_in)) {
		char *newline_pos;
		char **tokens;
		
		newline_pos = strchr(line, '\n');
		newline_pos[0] = '\0';

		tokens = g_strsplit(line, "=", 2);

		if (strcmp(tokens[0], "prog_termemu") == 0 && tokens[1] != NULL) {
			settings->cmd_termemu = strdup(tokens[1]);
		}
		if (strcmp(tokens[0], "prog_su") == 0 && tokens[1] != NULL) {
			settings->cmd_su = strdup(tokens[1]);
		}
		if (strcmp(tokens[0], "history_max") == 0 && tokens[1] != NULL) {
			settings->history_max = atoi(tokens[1]);
		}

		g_strfreev(tokens);
	}

	fclose(f_in);

	free(filename);
	
	return(settings);
}

/* History of command */
GList *history_read() {
	FILE *f_in;
	char *filename = NULL;
	GList *history = NULL;
	char line[4096];
	
	filename = (char *)malloc(sizeof(char) * (strlen(getenv("HOME")) + 15 + 1) );
	sprintf(filename, "%s/.gexec_history", getenv("HOME"));

	f_in = fopen(filename, "r");
	if (f_in == NULL) {
		free(filename);
		return(NULL);
	}

	while (fgets(line, 4096, f_in)) {
		char *newline_pos;
		
		newline_pos = strchr(line, '\n');
		newline_pos[0] = '\0';

		history = g_list_append(history, strdup(line));
	}

	fclose(f_in);

	free(filename);
	
	return(history);
}

int history_write(GList *history, int history_max) {
	char *filename = NULL;
	FILE *f_out;
	GList *cur = NULL;
	int count = 0;

	filename = (char *)malloc(sizeof(char) * (strlen(getenv("HOME")) + 15 + 1) );
	sprintf(filename, "%s/.gexec_history", getenv("HOME"));

	cur = history;

	f_out = fopen(filename, "w");
	if (f_out == NULL) {
		free(filename);
		return(-1);
	}
	
	while (cur != NULL && count != history_max) {
		fprintf(f_out, "%s\n", (char *)cur->data);
		cur = g_list_next(cur);
		count++;
	}

	fclose(f_out);

	free(filename);
	return(0);
}

GList *history_add(GList *history, char *command) {
	GList *new = NULL;
	GList *cur = NULL;
	
	new = g_list_append(new, command);
	
	/* Deep-copy history list without duplicates */
	cur = history;
	while (cur != NULL) {
		if (strcmp(cur->data, command) != 0) {
			new = g_list_append(new, cur->data);
		} else {
			free(cur->data);
		}
		cur = g_list_next(cur);
	}

	g_list_free(history);

	return(new);
}

/* -1 : invalid command; 
 * -2 : invalid termemu string
 * -3 : invalid su string
 * otherwise doesn't return */
int cmd_run(struct _task *task) {
	int argc;
	char **argv;
	GError *shellparse_err = NULL;
	
	assert(task->command != NULL);
	assert(task->cmd_termemu != NULL);

	/* Handle execution options */
	/* FIXME: This will cause problems if both options are used */
	if ((task->options & OPT_TERM) == OPT_TERM) {
		int buf_size = (strlen(task->command) + strlen(task->cmd_termemu));
		char *temp = malloc(sizeof(char) * buf_size);
		
		if (snprintf(temp, buf_size, task->cmd_termemu, task->command) >= buf_size) {
			return(-2);
		}
		
		free(task->command);
		task->command = temp;
	}
	if ((task->options & OPT_SU) == OPT_SU) {
		int buf_size = (strlen(task->command) + strlen(task->cmd_su));
		char *temp = malloc(sizeof(char) * buf_size);
		
		if (snprintf(temp, buf_size, task->cmd_su, task->command) >= buf_size) {
			return(-3);
		}

		free(task->command);
		task->command = temp;
	}

	/* Split up commandline for execvp */
	if (g_shell_parse_argv(task->command, &argc, &argv, &shellparse_err) == FALSE) {
		return(-1);
	}
	
	/* Globbing */
	/* TODO: Not implemented yet. Check out wordexp(3); */

	/* Launch! */
	argv[argc] = NULL;
	execvp(argv[0], argv);

	return(0);
}

void cmd_complete_start(struct _task *task) {
	int argc;
	char **argv;
	GError *shellparse_err = NULL;
	char *last_arg = NULL;

	if (task->command == NULL) {
		return;
	}
	
	/* Split up command to get last command for completion */
	if (g_shell_parse_argv(task->command, &argc, &argv, &shellparse_err) == FALSE) {
		return;
	}
	last_arg = argv[argc-1];

	task->tabcomp_command = strdup(task->command); 
	task->tabcomp_matches = ac_list_get(last_arg); /* Possible appends for tabcomp_command */

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
	
	/* Cycle forwards through the list of matches */
	if (task->tabcomp_current_match == g_list_last(task->tabcomp_matches)) {
		task->tabcomp_current_match = g_list_first(task->tabcomp_matches);
	} else {
		task->tabcomp_current_match = g_list_next(task->tabcomp_current_match);
	}
}

void cmd_complete_prev(struct _task *task) {
	/* FIXME: Not yet completed */
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
	
	task->history = history_add(task->history, task->command);
	history_write(task->history, task->history_max);

	err = cmd_run(task); /* Wont return unless error */

	switch (err) {
		case -1:
			ui_error("Couldn't execute the command");
			break;
		case -2:
			ui_error("Possible invalid Terminal Emulator command");
			break;
		case -3:
			ui_error("Possible invalid SU command");
			break;
		default:
			ui_error("C'est impossible! Je ne comprande pas. Aborting");
	}
	
	gtk_main_quit();
}

/* Main window callbacks */
void ui_win_main_cb_delete_event(GtkWidget *widget, void *data) {
	gtk_main_quit();
}

/* Command callbacks */
void ui_combo_command_cb_activate(GtkWidget *widget, struct _task *task) {
	task->command = (char *)gtk_entry_get_text GTK_ENTRY(widget);

	gtk_widget_hide(win_main);
	while (gtk_events_pending()) /* Cause we don't get back to main gtk loop */
	  gtk_main_iteration();

	ui_cmd_run(task);
}

gboolean ui_combo_command_cb_key_press_event(GtkWidget *widget, 
		GdkEventKey *event, struct _task *task) {
	if (event->keyval == GDK_Escape) {
		/* Abort! Abort! */
		gtk_main_quit();
	} else
	if (event->keyval == GDK_Tab) {
		/* Tab completion */

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
		return(TRUE); /* Signal has been handled, do not pass tab key further */
	} else {
		/* Normal keypresses */

		/* Accpept/Clear tabcompletion */
		task->command = (char *)gtk_entry_get_text(GTK_ENTRY(widget));
		cmd_complete_cancel(task);
	}

	return(FALSE);
}
void ui_combo_command_cb_changed(GtkWidget *widget, struct _task *task) {
	task->command = (char *)gtk_entry_get_text(GTK_ENTRY(widget));
}

/* Option callbacks */
void ui_chk_term_cb_toggled(GtkWidget *widget, struct _task *task) {
	task->options ^= OPT_TERM;
}
void ui_chk_su_cb_toggled(GtkWidget *widget, struct _task *task) {
	task->options ^= OPT_SU;
}

/* Button callbacks */
void ui_btn_ok_cb_clicked(GtkWidget *widget, struct _task *task) {
	gtk_widget_hide(win_main);
	while (gtk_events_pending()) /* Cause we don't get back to main gtk loop */
	  gtk_main_iteration();

	ui_cmd_run(task);
}

void ui_btn_cancel_cb_clicked(GtkWidget *widget, void *data) {
	gtk_main_quit();
}

int main(int argc, char *argv[]) {
	struct _task *task = NULL;
	struct _settings *settings = NULL;
	GtkWidget *chk_term, *chk_su;
	GtkWidget *lbl_command;
	GtkWidget *combo_command;
	GtkWidget *vbox_main, *hbox_command, *hbox_options, *hbox_buttons;
	GtkWidget *btn_ok, *btn_cancel;
	
	task = malloc(sizeof(struct _task));
	task->command = NULL;
	task->options = 0;
	task->cmd_termemu = NULL;
	task->cmd_su = NULL;
	task->tabcomp_matches = NULL;
	task->tabcomp_current_match = NULL;
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

	/* Command */
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

	/* Options */
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

	/* Buttons */
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

	/* Main */
	vbox_main = gtk_vbox_new(FALSE, 0);
	
	gtk_box_pack_start(GTK_BOX(vbox_main), hbox_command, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox_main), hbox_options, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox_main), hbox_buttons, TRUE, TRUE, 0);

	gtk_container_add(GTK_CONTAINER(win_main), vbox_main);
	gtk_container_set_border_width(GTK_CONTAINER(win_main), 5);
	
	/* Booom! */
	gtk_widget_show_all(win_main);
	gtk_widget_grab_focus(combo_command);

	gtk_main();

	if (history_write(task->history, task->history_max) != 0) {
		perror("Couldn't write history file");
	};

	free(task);

	return(0);
}
