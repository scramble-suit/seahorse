/*
 * Seahorse
 *
 * Copyright (C) 2003 Jacob Perkins
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the
 * Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <gnome.h>

#include "seahorse-recipients.h"
#include "seahorse-widget.h"
#include "seahorse-ops-key.h"
#include "seahorse-validity.h"
#include "seahorse-encrypt-recipients-store.h"

#define ALL "all_keys"
#define RECIPIENTS "recipient_keys"

enum {
	PROP_0,
	PROP_VALIDITY
};

static void	seahorse_recipients_class_init		(SeahorseRecipientsClass	*klass);
static void	seahorse_recipients_init		(SeahorseRecipients		*srecips);
static void	seahorse_recipients_finalize		(GObject			*gobject);
static void	seahorse_recipients_set_property	(GObject			*object,
							 guint				prop_id,
							 const GValue			*value,
							 GParamSpec			*pspec);
static void	seahorse_recipients_get_property	(GObject			*object,
							 guint				prop_id,
							 GValue				*value,
							 GParamSpec			*pspec);

static SeahorseWidgetClass	*parent_class		= NULL;

GType
seahorse_recipients_get_type (void)
{
	static GType recipients_type = 0;
	
	if (!recipients_type) {
		static const GTypeInfo recipients_info =
		{
			sizeof (SeahorseRecipientsClass),
			NULL, NULL,
			(GClassInitFunc) seahorse_recipients_class_init,
			NULL, NULL,
			sizeof (SeahorseRecipients),
			0,
			(GInstanceInitFunc) seahorse_recipients_init
		};
		
		recipients_type = g_type_register_static (SEAHORSE_TYPE_WIDGET,
			"SeahorseRecipients", &recipients_info, 0);
	}
	
	return recipients_type;
}

static void
seahorse_recipients_class_init (SeahorseRecipientsClass *klass)
{
	GObjectClass *gobject_class;
	
	parent_class = g_type_class_peek_parent (klass);
	gobject_class = G_OBJECT_CLASS (klass);
	
	gobject_class->set_property = seahorse_recipients_set_property;
	gobject_class->get_property = seahorse_recipients_get_property;
	gobject_class->finalize = seahorse_recipients_finalize;
	
	g_object_class_install_property (gobject_class, PROP_VALIDITY,
		g_param_spec_boolean ("validity", "If need validity",
				      "TRUE if recipients need to have full validity",
				      FALSE, G_PARAM_READWRITE));
}

/* Initialize recipient list */
static void
seahorse_recipients_init (SeahorseRecipients *srecips)
{
	if (gpgme_recipients_new (&(srecips->recips)) != GPGME_No_Error)
		srecips->recips = NULL;
}

static void
seahorse_recipients_finalize (GObject *gobject)
{
	SeahorseRecipients *srecips;
	
	srecips = SEAHORSE_RECIPIENTS (gobject);
	seahorse_key_store_destroy (srecips->all_keys);
	seahorse_key_store_destroy (srecips->recipient_keys);
	
	G_OBJECT_CLASS (parent_class)->finalize (gobject);
}

static void
seahorse_recipients_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
	SeahorseRecipients *srecips;
	
	srecips = SEAHORSE_RECIPIENTS (object);
	
	switch (prop_id) {
		case PROP_VALIDITY:
			srecips->need_validity = g_value_get_boolean (value);
			break;
		default:
			break;
	}
}

static void
seahorse_recipients_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
	SeahorseRecipients *srecips;
	
	srecips = SEAHORSE_RECIPIENTS (object);
	
	switch (prop_id) {
		case PROP_VALIDITY:
			g_value_set_boolean (value, srecips->need_validity);
			break;
		default:
			break;
	}
}

static void
transfer (SeahorseKeyStore *lhs, SeahorseKeyStore *rhs, GtkTreePath *path)
{
	SeahorseKey *skey;
	
	skey = seahorse_key_store_get_key_from_model (GTK_TREE_MODEL (lhs), path);
	seahorse_key_store_remove (lhs, path);
	seahorse_key_store_append (rhs, skey);
}

static void
set_all_buttons (SeahorseWidget *swidget)
{
	SeahorseRecipients *srecips;
	GtkTreeIter iter;
	gboolean sensitive;
	
	srecips = SEAHORSE_RECIPIENTS (swidget);
	
	gtk_widget_set_sensitive (glade_xml_get_widget (swidget->xml, "add_all"),
		gtk_tree_model_get_iter_first (GTK_TREE_MODEL (srecips->all_keys), &iter));
	sensitive = gtk_tree_model_get_iter_first (GTK_TREE_MODEL (
		srecips->recipient_keys), &iter);
	gtk_widget_set_sensitive (glade_xml_get_widget (
		swidget->xml, "remove_all"), sensitive);
	gtk_widget_set_sensitive (glade_xml_get_widget (
		swidget->xml, "ok"), sensitive);
}

/* Moves key iter from right (non-recips) to left (recips) *
static void
add_recip (SeahorseWidget *swidget, GtkTreePath *path)
{
	SeahorseRecipients *srecips;
	GpgmeValidity validity;
	GtkWidget *question;
	gint response;
	SeahorseKey *skey;
	
	srecips = SEAHORSE_RECIPIENTS (swidget);
	skey = seahorse_key_store_get_key_from_path (GTK_TREE_VIEW (
		glade_xml_get_widget (swidget->xml, ALL)), path);
	
	validity = gpgme_key_get_ulong_attr (skey->key, GPGME_ATTR_VALIDITY, NULL, 0);
	/* Check if need key to be fully valid *
	if (srecips->need_validity && validity < GPGME_VALIDITY_FULL) {
		question = gtk_message_dialog_new (
			GTK_WINDOW (glade_xml_get_widget (swidget->xml, swidget->name)),
			GTK_DIALOG_MODAL, GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO,
			"%s\nis not a fully valid key.\n"
			"Would you like to temporarily give it full validity for this operation?",
			seahorse_key_get_userid (skey, 0));
		
		response = gtk_dialog_run (GTK_DIALOG (question));
		gtk_widget_destroy (question);
		
		/* If don't want key, return *
		if (response != GTK_RESPONSE_YES)
			return;
	}

	seahorse_key_store_remove (srecips->all_keys, path);
	seahorse_key_store_append (srecips->recipient_keys, skey);
	
	if (gpgme_recipients_count (srecips->recips) == 0)
		gtk_widget_set_sensitive (glade_xml_get_widget (swidget->xml, "ok"), TRUE);
	
	seahorse_ops_key_recips_add (srecips->recips, skey);
}

/* Moves key from left (recips) to right (non-recips) *
static void
remove_recip (SeahorseWidget *swidget, GtkTreePath *path)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	SeahorseRecipients *srecips;
	SeahorseKey *skey;
	
	srecips = SEAHORSE_RECIPIENTS (swidget);
	skey = seahorse_key_store_get_key_from_path (GTK_TREE_VIEW (
		glade_xml_get_widget (swidget->xml, RECIPIENTS)), path);
	seahorse_key_store_remove (srecips->recipient_keys, path);
	seahorse_key_store_append (srecips->all_keys, skey);
	
	model = GTK_TREE_MODEL (srecips->recipient_keys);
	
	/* Need to rebuild recipients *
	gpgme_recipients_release (srecips->recips);
	g_return_if_fail (gpgme_recipients_new (&(srecips->recips)) == GPGME_No_Error);
	
	if (gtk_tree_model_get_iter_first (model, &iter)) {
		do {
			path = gtk_tree_model_get_path (model, &iter);
			skey = seahorse_key_store_get_key_from_path (GTK_TREE_VIEW (
				glade_xml_get_widget (swidget->xml, RECIPIENTS)), path);
			seahorse_ops_key_recips_add (srecips->recips, skey);
		} while (gtk_tree_model_iter_next (model, &iter));
	}
	
	if (gpgme_recipients_count (srecips->recips) == 0)
		gtk_widget_set_sensitive (glade_xml_get_widget (swidget->xml, "ok"), FALSE);
}
*/
/* Add recipient button clicked */
static void
add_clicked (GtkButton *button, SeahorseWidget *swidget)
{
	GtkTreeSelection *selection;
	GList *list = NULL;
	SeahorseRecipients *srecips;
	GtkTreeView *view;
	
	view = GTK_TREE_VIEW (glade_xml_get_widget (swidget->xml, ALL));
	selection = gtk_tree_view_get_selection (view);
	list = gtk_tree_selection_get_selected_rows (selection, NULL);
	srecips = SEAHORSE_RECIPIENTS (swidget);
	
	while (list != NULL) {
		transfer (srecips->all_keys, srecips->recipient_keys, list->data);
		//gtk_tree_path_free (list->data);
		list = g_list_next (list);
	}
	g_list_free (list);
	
	set_all_buttons (swidget);
}

/* Remove recipient button clicked */
static void
remove_clicked (GtkButton *button, SeahorseWidget *swidget)
{
	GtkTreeSelection *selection;
	GList *list = NULL;
	SeahorseRecipients *srecips;
	GtkTreeView *view;
	
	view = GTK_TREE_VIEW (glade_xml_get_widget (swidget->xml, RECIPIENTS));
	selection = gtk_tree_view_get_selection (view);
	list = gtk_tree_selection_get_selected_rows (selection, NULL);
	srecips = SEAHORSE_RECIPIENTS (swidget);
	
	while (list != NULL) {
		transfer (srecips->recipient_keys, srecips->all_keys, list->data);
		//gtk_tree_path_free (list->data);
		list = g_list_next (list);
	}
	g_list_free (list);
	
	set_all_buttons (swidget);
}

/* Right (non-recip) key activated */
static void
add_row_activated (GtkTreeView *view, GtkTreePath *path,
		   GtkTreeViewColumn *arg2, SeahorseRecipients *srecips)
{
	transfer (srecips->all_keys, srecips->recipient_keys, path);
	set_all_buttons (SEAHORSE_WIDGET (srecips));
}

/* Left (recip) key activated */
static void
remove_row_activated (GtkTreeView *view, GtkTreePath *path,
		      GtkTreeViewColumn *arg2, SeahorseRecipients *srecips)
{
	transfer (srecips->recipient_keys, srecips->all_keys, path);
	set_all_buttons (SEAHORSE_WIDGET (srecips));
}

static gboolean
add_all_foreach (GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, SeahorseWidget *swidget)
{
	SeahorseRecipients *srecips;
	
	srecips = SEAHORSE_RECIPIENTS (swidget);
	transfer (srecips->all_keys, srecips->recipient_keys, path);
	//return FALSE;
}

static void
add_all_clicked (GtkButton *button, SeahorseWidget *swidget)
{
	gtk_tree_model_foreach (GTK_TREE_MODEL (SEAHORSE_RECIPIENTS (swidget)->all_keys),
		(GtkTreeModelForeachFunc)add_all_foreach, swidget);
	set_all_buttons (swidget);
}

static gboolean
remove_all_foreach (GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, SeahorseWidget *swidget)
{
	SeahorseRecipients *srecips;
	
	srecips = SEAHORSE_RECIPIENTS (swidget);
	transfer (srecips->recipient_keys, srecips->all_keys, path);
	return FALSE;
}

static void
remove_all_clicked (GtkButton *button, SeahorseWidget *swidget)
{
	gtk_tree_model_foreach (GTK_TREE_MODEL (SEAHORSE_RECIPIENTS (swidget)->recipient_keys),
		(GtkTreeModelForeachFunc)remove_all_foreach, swidget);
	set_all_buttons (swidget);
}

static void
selection_changed (GtkTreeSelection *selection, GtkWidget *widget)
{
	gtk_widget_set_sensitive (widget, gtk_tree_selection_count_selected_rows (selection) > 0);
}

/* Creates a new #SeahorseRecipients given @sctx and whether doing
 * encrypt recipients.
 */
static SeahorseWidget*
seahorse_recipients_new (SeahorseContext *sctx, gboolean use_encrypt)
{
	SeahorseRecipients *srecips;
	SeahorseWidget *swidget;
	GtkTreeView *all_keys;
	GtkTreeView *recips_keys;
	GtkTreeSelection *selection;
	
	srecips = g_object_new (SEAHORSE_TYPE_RECIPIENTS, "name", "recipients", "ctx", sctx,
		"validity", use_encrypt, NULL);
	swidget = SEAHORSE_WIDGET (srecips);
	
	glade_xml_signal_connect_data (swidget->xml, "add_clicked",
		G_CALLBACK (add_clicked), swidget);
	glade_xml_signal_connect_data (swidget->xml, "remove_clicked",
		G_CALLBACK (remove_clicked), swidget);
	glade_xml_signal_connect_data (swidget->xml, "add_row_activated",
		G_CALLBACK (add_row_activated), swidget);
	glade_xml_signal_connect_data (swidget->xml, "remove_row_activated",
		G_CALLBACK (remove_row_activated), swidget);
	glade_xml_signal_connect_data (swidget->xml, "add_all_clicked",
		G_CALLBACK (add_all_clicked), swidget);
	glade_xml_signal_connect_data (swidget->xml, "remove_all_clicked",
		G_CALLBACK (remove_all_clicked), swidget);
	
	all_keys = GTK_TREE_VIEW (glade_xml_get_widget (swidget->xml, ALL));
	recips_keys = GTK_TREE_VIEW (glade_xml_get_widget (swidget->xml, RECIPIENTS));

	if (use_encrypt) {
		srecips->all_keys = seahorse_encrypt_recipients_store_new (sctx, all_keys);
		srecips->recipient_keys = seahorse_encrypt_recipients_store_new (sctx, recips_keys);
	}
	else {
		srecips->all_keys = seahorse_recipients_store_new (sctx, all_keys);
		srecips->recipient_keys = seahorse_recipients_store_new (sctx, recips_keys);
	}

	seahorse_key_store_populate (srecips->all_keys);
	gtk_widget_show_all (glade_xml_get_widget (swidget->xml, swidget->name));
	/* listen to selection change */
	selection = gtk_tree_view_get_selection (all_keys);
	gtk_tree_selection_set_mode (selection, GTK_SELECTION_MULTIPLE);
	g_signal_connect_after (selection, "changed", G_CALLBACK (selection_changed),
		glade_xml_get_widget (swidget->xml, "add"));
	selection = gtk_tree_view_get_selection (recips_keys);
	gtk_tree_selection_set_mode (selection, GTK_SELECTION_MULTIPLE);
	g_signal_connect_after (selection, "changed", G_CALLBACK (selection_changed),
		glade_xml_get_widget (swidget->xml, "remove"));

	return swidget;
}

/**
 * seahorse_export_recipients_new:
 * @sctx: Current #SeahorseContext
 *
 * Creates a new #SeahorseRecipients for exporting keys.
 *
 * Returns: The new #SeahorseWidget
 **/
SeahorseWidget*
seahorse_export_recipients_new (SeahorseContext *sctx)
{
	return seahorse_recipients_new (sctx, FALSE);
}

/**
 * seahorse_encrypt_recipients_new:
 * @sctx: Current #SeahorseContext
 *
 * Creates a new #SeahorseRecipients for encrypting data with keys.
 *
 * Returns: The new #SeahorseWidget
 **/
SeahorseWidget*
seahorse_encrypt_recipients_new (SeahorseContext *sctx)
{
	return seahorse_recipients_new (sctx, TRUE);
}

static gboolean
recipients_foreach (GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, GpgmeRecipients recips)
{
	seahorse_ops_key_recips_add (recips, seahorse_key_store_get_key_from_model (model, path));
	return FALSE;
}

/**
 * seahorse_recipients_run:
 * @srecips: #SeahorseRecipients to run
 *
 * Runs the #SeahorseRecipients dialog until user finishes.
 * @srecips must be destroyed after returning.
 *
 * Returns: The selected recipients, or NULL if none
 **/
GpgmeRecipients
seahorse_recipients_run (SeahorseRecipients *srecips)
{	
	SeahorseWidget *swidget;
	gint response;
	gboolean done = FALSE;
	GpgmeRecipients recips;

	swidget = SEAHORSE_WIDGET (srecips);
	g_return_val_if_fail (gpgme_recipients_new (&recips) == GPGME_No_Error, NULL);
	
	while (!done) {
		response = gtk_dialog_run (GTK_DIALOG (glade_xml_get_widget (
			swidget->xml, swidget->name)));
		
		switch (response) {
			case GTK_RESPONSE_HELP:
				break;
			case GTK_RESPONSE_OK:
				done = TRUE;
				gtk_tree_model_foreach (GTK_TREE_MODEL (srecips->recipient_keys),
					(GtkTreeModelForeachFunc)recipients_foreach, recips);
				break;
			default:
				gpgme_recipients_release (recips);
				srecips->recips = NULL;
				done = TRUE;
				break;
		}
	}
	
	return recips;
}
