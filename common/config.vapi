[CCode (cprefix = "", lower_case_cprefix = "", cheader_filename = "config.h")]
namespace Config
{
	public const string PKGDATADIR;

	public const string EXECDIR;

	public const string VERSION;
	public const string PACKAGE;
	public const string GETTEXT_PACKAGE;

	public const string SSH_PATH;
	public const string SSH_KEYGEN_PATH;
}

/*
 * TODO: Temporary hacks for interfacing with some very simple C code in
 * the current C code base. In general we want to port to vala instead of
 * listing stuff here. Otherwise things will get unmanageable.
 */

namespace Seahorse {

[CCode (cheader_filename = "libseahorse/seahorse-application.h")]
namespace Application {
	public unowned Gtk.Application @get();
	public unowned GLib.Settings pgp_settings(Gtk.Application? self = null);
	public unowned GLib.Settings settings(Gtk.Application? self = null);
}

[CCode (cheader_filename = "libseahorse/seahorse-util.h")]
public static GLib.HashFunc<ulong?> ulong_hash;

[CCode (cheader_filename = "libseahorse/seahorse-util.h")]
public static GLib.EqualFunc<ulong?> ulong_equal;

[CCode (cheader_filename = "libseahorse/seahorse-interaction.h")]
public class Interaction : GLib.TlsInteraction {
	public Interaction(Gtk.Window? parent);
	public Gtk.Window? parent;
}

[CCode (cheader_filename = "libseahorse/seahorse-progress.h")]
namespace Progress {
	public void show(GLib.Cancellable? cancellable, string title, bool delayed);
}

}

namespace Egg {
	namespace TreeMultiDrag {
		public void add_drag_support(Gtk.TreeView view);
	}
}
