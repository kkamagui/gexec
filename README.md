gExec
=====

![Status: Stable](https://img.shields.io/badge/status-stable-green.svg)
![Activity: Maintenance only](https://img.shields.io/badge/activity-maintenance%20only-yellowgreen.svg)


Introduction
------------

gExec is a tool for running commands from a GUI. It presents the user with
an entry field into which they can type their command and some toggle 
buttons for specifying how the command should be run. (I.e. in a terminal).
	
It features (normal, understandable, working) tab-completion, history list
and options for running commands in a terminal emulator and/or the root 
user (via gksu).

gExec was created because I mainly use GTK programs, but I do not run
Gnome (or at least not the panel). Instead I use WindowMaker. WindowMaker
does have it's own run dialog, but it doesn't have tab-completion nor does
it have a history, which is quite annoying. There are a bunch of 
third-party program lauchers available, but at the moment of writing none
of them do what I want. So came about gExec.

![](https://raw.githubusercontent.com/fboender/gexec/master/contrib/screenshot.png)


Installation
------------

### Requirements

In order to compile gExec, you will need:

*   Gtk 2.0+ development headers
*   Glib 2.0+ development headers

In order to run gExec, you will need:

*   Gtk 2.0+ dynamically loadable libraries.


### Compiling

Compiling gExec is easy. Just type 'make' at the command line in the 
source directory.

### Installation

gExec is composed of only a single binary. You can copy it to any 
destination on your system. May I suggest some directory that is in your
path, like /usr/local/bin?

Typing 'make install' at the command line in the source directory will do
just that. It copies the binary to /usr/local/bin. That's it. Nothing more,
nothing less.
	

### Running

From the --help output:

	Usage:
	  gexec [OPTION...] - Interactive run dialog

	Help Options:
	  -?, --help               Show help options
	  --help-all               Show all help options
	  --help-gtk               Show GTK+ Options

	Application Options:
	  -k, --keepopen           Keep gExec open after executing a command
	  --display=DISPLAY        X display to use

Configuration
-------------

gExec has a few configuration options which are stored in the file

    $HOME/.gexec

This file will automically be created if not present the first time
gExec is run. It can contain three options:

	cmd_termemu=STRING   (default:  xterm -e %s  )
	cmd_su=STRING        (default:  gksu "%s"  )
	history_max=INT      (default:  20  )

These options should be self-evident. Including a %s in the command
will replace it with the command entered by the user in the program.
	
Please be careful about the	construction of the commands for the 
terminal emulator and the SU program. Not all programs will work 
nicely together and some of	the commands need to be carefully 
crafted taking in account quotes and other stuff. Frankly, changing
any of these commands might open up a whole can of worms (which is 
why there isn't a configuration display). Use at your own risk.
	
Feedback / Contributing
-----------------------

Questions? Problems? Ideas? Visit this gExec's project page, where you can
submit bugs/feature requests/questions, at:

https://github.com/fboender/gexec
	
### Known bugs

This program is full of known bugs. Here are some of them:
	
*   Running a command in both a terminal and as root fails (depending on the
    commands used).
*   Usage of deprecated gtk\_combo.
*   Failure to execute a command via terminal or as root will go unnoticed.
	
Most of these bugs have a reason for not being fixed (and not even 
because I'm just a lazy bastard). Mostly the reason is that the 
bugs don't really matter in such a small program, solving the 
problem opens up new problems or the problem isn't gExec's in the
first place.
	

License
-------

gExec is Copyright by Ferry Boender, licensed under the General Public
License (GPL)

	Copyright (C), 2004 by Ferry Boender

	This program is free software; you can redistribute it and/or modify it
	under the terms of the GNU General Public License as published by the Free
	Software Foundation; either version 2 of the License, or (at your option)
	any later version.

	This program is distributed in the hope that it will be useful, but
	WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
	or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
	for more details.

	You should have received a copy of the GNU General Public License along
	with this program; if not, write to the Free Software Foundation, Inc.,
	675 Mass Ave, Cambridge, MA 02139, USA.

	For more information, see the COPYING file supplied with this program.

