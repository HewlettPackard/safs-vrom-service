project('safs-vrom',
        'cpp',
        version: '1.0',
        default_options: [
          'warning_level=3',
          'werror=true',
          'cpp_std=c++23',
          'buildtype=debugoptimized',
        ]
)
cpp = meson.get_compiler('cpp')

# Include Directories
incdir = include_directories( 
  'include'
)

srcfiles_safsvrom = ['source/safs-vrom.cpp'
                    ]

systemd_dep = cpp.find_library('systemd', required : false)
phosphor_logging_dep = cpp.find_library('phosphor_logging',required : false)

dependencies_safsvrom = [systemd_dep, phosphor_logging_dep]

# Generate the safs-vrom executable
executable(
  'safs-vrom',
  srcfiles_safsvrom,
  include_directories : incdir,
  dependencies: dependencies_safsvrom,
  install: true,
  install_dir: get_option('bindir')
)

systemd = dependency('systemd')
systemd_system_unit_dir = systemd.get_pkgconfig_variable(
    'systemdsystemunitdir',
    define_variable: ['prefix', get_option('prefix')])

configure_file(
  copy: true,
  input: 'service_files/com.hpe.bmc.safsvrom.service',
  install: true,
  install_dir: systemd_system_unit_dir,
  output: 'com.hpe.bmc.safsvrom.service'
)
