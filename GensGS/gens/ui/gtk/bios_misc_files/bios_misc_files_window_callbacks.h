/***************************************************************************
 * Gens: (GTK+) BIOS/Misc Files Window - Callback Functions.               *
 *                                                                         *
 * Copyright (c) 1999-2002 by Stéphane Dallongeville                       *
 * Copyright (c) 2003-2004 by Stéphane Akhoun                              *
 * Copyright (c) 2008 by David Korth                                       *
 *                                                                         *
 * This program is free software; you can redistribute it and/or modify it *
 * under the terms of the GNU General Public License as published by the   *
 * Free Software Foundation; either version 2 of the License, or (at your  *
 * option) any later version.                                              *
 *                                                                         *
 * This program is distributed in the hope that it will be useful, but     *
 * WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 * GNU General Public License for more details.                            *
 *                                                                         *
 * You should have received a copy of the GNU General Public License along *
 * with this program; if not, write to the Free Software Foundation, Inc., *
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.           *
 ***************************************************************************/

#ifndef GTK_BIOS_MISC_FILES_WINDOW_CALLBACKS_H
#define GTK_BIOS_MISC_FILES_WINDOW_CALLBACKS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <gtk/gtk.h>

gboolean on_bios_misc_files_window_close(GtkWidget *widget, GdkEvent *event, gpointer user_data);
void on_button_bmf_Change_clicked(GtkButton *button, gpointer user_data);
void on_button_bmf_Cancel_clicked(GtkButton *button, gpointer user_data);
void on_button_bmf_Apply_clicked(GtkButton *button, gpointer user_data);
void on_button_bmf_Save_clicked(GtkButton *button, gpointer user_data);

#ifdef __cplusplus
}
#endif

#endif