subdir('icons')

# GSettings
gsettings_files = [
  'org.gnome.seahorse.gschema.xml',
  'org.gnome.seahorse.manager.gschema.xml',
  'org.gnome.seahorse.window.gschema.xml',
]
install_data(gsettings_files,
  install_dir: join_paths(datadir, 'glib-2.0', 'schemas'),
)

# Resources
resources_xml = join_paths(meson.source_root(), 'data', 'seahorse.gresource.xml')
resources_src = gnome.compile_resources('seahorse-resources',
  resources_xml,
)

# The appdata file
i18n.merge_file(
  input: 'org.gnome.seahorse.Application.appdata.xml.in',
  output: 'org.gnome.seahorse.Application.appdata.xml',
  type: 'xml',
  po_dir: po_dir,
  install: true,
  install_dir: join_paths(datadir, 'metainfo')
)

# The configuration for the desktop and service files
desktop_conf = configuration_data()
desktop_conf.set('bindir', bindir)

# The desktop file
configured_desktop_file = configure_file(
  input: 'org.gnome.seahorse.Application.desktop.in.in',
  output: 'org.gnome.seahorse.Application.desktop.in',
  configuration: desktop_conf,
)
i18n.merge_file(
  input: configured_desktop_file,
  output: 'org.gnome.seahorse.Application.desktop',
  type: 'desktop',
  po_dir: po_dir,
  install: true,
  install_dir: join_paths(datadir, 'applications')
)

# The service file
configure_file(
  input: 'org.gnome.seahorse.Application.service.in',
  output: 'org.gnome.seahorse.Application.service',
  configuration: desktop_conf,
  install: true,
  install_dir: join_paths(datadir, 'dbus-1', 'services'),
)

# The search provider
install_data(
  'seahorse-search-provider.ini',
  install_dir: join_paths(datadir, 'gnome-shell', 'search-providers')
)

# manpage
if with_manpage
  xsltproc = find_program('xsltproc')
  custom_target('manfile-seahorse',
    input: 'seahorse.xml',
    output: 'seahorse.1',
    install: true,
    install_dir: join_paths(get_option('mandir'), 'man1'),
    command: [
      xsltproc,
      '--nonet',
      '--stringparam', 'man.output.quietly', '1',
      '--stringparam', 'funcsynopsis.style', 'ansi',
      '--stringparam', 'man.th.extra1.suppress', '1',
      '--stringparam', 'man.authors.section.enabled', '0',
      '--stringparam', 'man.copyright.section.enabled', '0',
      '-o', '@OUTPUT@',
      'http://docbook.sourceforge.net/release/xsl/current/manpages/docbook.xsl',
      '@INPUT@'
    ]
  )
endif
