project('mai2io_dll', 'c', version: '0.1.0')


add_project_arguments(
    '-DCOBJMACROS',
    '-DDIRECTINPUT_VERSION=0x0800',
    '-DWIN32_LEAN_AND_MEAN',
    '-D_WIN32_WINNT=_WIN32_WINNT_WIN7',
    '-DMINGW_HAS_SECURE_API=1',
    language: 'c',
)

# Use get_argument_syntax() instead once Meson 0.49.0 releases
if meson.get_compiler('c').get_id() != 'msvc'
    add_project_arguments(
        '-ffunction-sections',
        '-fdata-sections',
        language: 'c',
    )

    add_project_link_arguments(
        '-Wl,--enable-stdcall-fixup',
        '-Wl,--exclude-all-symbols',
        '-Wl,--gc-sections',
        '-static-libgcc',
        language: 'c',
    )
endif

cc = meson.get_compiler('c')
hid_lib = cc.find_library('hid')
setupapi_lib = cc.find_library('setupapi')

inc = include_directories('.')


mai2io_lib = shared_library(
    'mai2io',
    name_prefix : '',
    include_directories : [inc, '../'],
    vs_module_defs : 'mai2io.def',
    implicit_include_directories : false,
    dependencies : [
        hid_lib,
        setupapi_lib,
    ],
    sources : [
        'mai2io.c',
        'mai2io.h',
        'config.c',
        'config.h',
    ],
)
