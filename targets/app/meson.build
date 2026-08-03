fs = import('fs')

app_common_sources = files(fs.read('common_sources.txt').strip().split('\n'))
app_dependencies = []
app_link_args = []

mimalloc_dep = dependency('mimalloc', required: get_option('enable_mimalloc'))
if mimalloc_dep.found()
    app_dependencies += mimalloc_dep
endif

app_dependencies += [
    dependency('sdl2'),
    dependency('miniaudio'),
    dependency('stb'),
    dependency('simdutf'),
    java_dep,
    nbt_dep,
    util_dep,
    minecraft_dep,
    assets_localisation_dep,
    platform_fs_dep,
    platform_game_dep,
    platform_input_dep,
    platform_leaderboard_dep,
    platform_network_dep,
    platform_profile_dep,
    platform_renderer_dep,
    platform_sound_dep,
    platform_storage_dep,
    platform_thread_dep,
]

if host_machine.system() == 'android'
    app = shared_library(
        'main',
        app_common_sources + app_platform_sources + localisation[1],
        include_directories: include_directories('..'),
        dependencies: app_dependencies,
        cpp_args: global_cpp_args + global_cpp_defs,
        link_args: app_link_args,
        install: true,
        install_dir: '',
        name_prefix: 'lib'
    )
else
    app = executable(
        'Minecraft.Client',
        app_common_sources + app_platform_sources + localisation[1],
        include_directories: include_directories('..'),
        dependencies: app_dependencies,
        cpp_args: global_cpp_args + global_cpp_defs,
        link_args: app_link_args,
        install: true,
        install_dir: '',
    )
endif

custom_target(
    'copy_assets_to_app',
    input: [app, media_archive],
    output: 'assets.stamp',
    command: [
        python,
        meson.project_source_root() / 'scripts/copy_assets_to_app.py',
        meson.project_source_root(),
        meson.project_build_root(),
        meson.current_build_dir(),
        '@INPUT1@',
        '@OUTPUT@',
    ],
    build_by_default: true,
)