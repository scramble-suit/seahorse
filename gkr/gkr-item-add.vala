/*
 * Seahorse
 *
 * Copyright (C) 2008 Stefan Walter
 * Copyright (C) 2018 Niels De Graef
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
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

[GtkTemplate (ui = "/org/gnome/Seahorse/seahorse-gkr-add-item.ui")]
public class Seahorse.Gkr.ItemAdd : Gtk.Dialog {
    [GtkChild]
    private Gtk.ComboBox item_keyring_combo;
    [GtkChild]
    private Gtk.Container password_area;
    private Gtk.Entry password_entry;
    [GtkChild]
    private Gtk.Entry item_entry;
    [GtkChild]
    private Gtk.CheckButton show_password_checkbutton;

    construct {
        // Load up a list of all the keyrings, and select the default
        var store = new Gtk.ListStore(2, typeof(string), typeof(Secret.Collection));
        this.item_keyring_combo.set_model(store);

        var cell = new Gtk.CellRendererText();
        this.item_keyring_combo.pack_start(cell, true);
        this.item_keyring_combo.add_attribute(cell, "text", 0);

        foreach (var keyring in Backend.instance().get_keyrings()) {
            Gtk.TreeIter iter;
            store.append(out iter);
            store.set(iter, 0, keyring.label,
                            1, keyring);
            if (keyring.is_default)
                this.item_keyring_combo.set_active_iter(iter);
        }

        set_response_sensitive(Gtk.ResponseType.ACCEPT, false);

        var buffer = new Gcr.SecureEntryBuffer();
        this.password_entry = new Gtk.Entry.with_buffer(buffer);
        this.password_entry.visibility = false;
        this.password_area.add(this.password_entry);
        this.password_entry.show();

        this.show_password_checkbutton.toggled.connect(() => {
            this.password_entry.visibility = this.show_password_checkbutton.active;
        });
    }

    public ItemAdd(Gtk.Window? parent) {
        GLib.Object(
            transient_for: parent,
            use_header_bar: 1
        );
        show();
        present();
    }

    [GtkCallback]
    private void on_add_item_entry_changed (Gtk.Editable entry) {
        set_response_sensitive(Gtk.ResponseType.ACCEPT, this.item_entry.text != "");
    }

    public override void response(int resp) {
        if (resp != Gtk.ResponseType.ACCEPT) {
            this.destroy();
            return;
        }

        Gtk.TreeIter iter;
        if (!this.item_keyring_combo.get_active_iter(out iter))
            return;

        Secret.Collection collection;
        this.item_keyring_combo.model.get(iter, 1, out collection, -1);

        var secret = new Secret.Value(this.password_entry.text, -1, "text/plain");
        var cancellable = Dialog.begin_request(this);
        var attributes = new HashTable<string, string>(GLib.str_hash, GLib.str_equal);

        /* TODO: Workaround for https://bugzilla.gnome.org/show_bug.cgi?id=697681 */
        var schema = new Secret.Schema("org.gnome.keyring.Note", Secret.SchemaFlags.NONE);

        Secret.Item.create.begin(collection, schema, attributes,
                                 this.item_entry.text, secret, Secret.ItemCreateFlags.NONE,
                                 cancellable, (obj, res) => {
            try {
                /* Clear the operation without cancelling it since it is complete */
                Dialog.complete_request(this, false);

                Secret.Item.create.end(res);
            } catch (GLib.Error err) {
                Util.show_error(this, _("Couldn’t add item"), err.message);
            }

            this.destroy();
        });
    }
}
