# MARK: build configuration

project(
    'portable-lce',
    ['cpp', 'c'],
    version: '0.1.0',
    meson_version: '>= 1.3',
    default_options: [
        'cpp_std=c++20',
        'warning_level=0',
        'unity=on',
        'unity_size=8',
        'buildtype=debug',
        'force_fallback_for=[\'miniaudio\',\'stb\']',
        'b_pch=true',
        'default_library=static',
    ],
)

if host_machine.system() == 'android' or get_option('renderer') == 'gles'
    meson.override_dependency('glew', declare_dependency())
    meson.override_dependency('gl', declare_dependency())
    meson.override_dependency('glu', declare_dependency())

    add_project_arguments(
        '-include', meson.project_source_root() / 'targets/platform/renderer/gl/gl_compat.h',
        language: ['cpp', 'c']
    )
endif

if host_machine.system() == 'darwin'
    add_languages('objc', native: false)
endif

if meson.get_compiler('cpp').get_id() == 'msvc'
    add_project_arguments('/bigobj', language: 'cpp')
endif

pymod = import('python')
python = pymod.find_installation('python3', required: true)

cc = meson.get_compiler('cpp')

global_cpp_args = []
global_cpp_defs = [
    '-DSPLIT_SAVES',
    '-D_LARGE_WORLDS',
    '-D_EXTENDED_ACHIEVEMENTS',
    '-DMULTITHREAD_ENABLE',
]

if host_machine.system() == 'android' or get_option('renderer') == 'gles'
    global_cpp_defs += [
        '-DGLES',
        '-DGLES3',
        '-DGL_ES',
        '-DGDRAW_GLES',
    ]
endif

if get_option('buildtype') in ['debug', 'debugoptimized']
    global_cpp_defs += [
        '-D_DEBUG',
        '-DDEBUG',
        '-D_DEBUG_MENUS_ENABLED',
    ]
endif

if get_option('buildtype') == 'debugoptimized'
    global_cpp_defs += ['-D_FORTIFY_SOURCE=2']
endif

# MARK: meson options

if get_option('enable_vsync')
    global_cpp_defs += ['-DENABLE_VSYNC']
endif

if get_option('classic_panorama')
    global_cpp_defs += '-DCLASSIC_PANORAMA'
endif

if get_option('enable_frame_profiler')
    global_cpp_defs += ['-DENABLE_FRAME_PROFILER']
endif

if get_option('ui_backend') == 'shiggy'
    global_cpp_defs += ['-D_ENABLEIGGY']
endif

if get_option('ui_backend') == 'java'
    global_cpp_defs += '-DENABLE_JAVA_GUIS'
endif

if get_option('occlusion_culling') == 'off'
    global_cpp_defs += ['-DOCCLUSION_MODE_NONE']
elif get_option('occlusion_culling') == 'frustum'
    global_cpp_defs += ['-DOCCLUSION_MODE_FRUSTUM']
elif get_option('occlusion_culling') == 'bfs'
    global_cpp_defs += ['-DOCCLUSION_MODE_BFS', '-DUSE_OCCLUSION_CULLING']
elif get_option('occlusion_culling') == 'hardware'
    global_cpp_defs += ['-DOCCLUSION_MODE_HARDWARE', '-DUSE_OCCLUSION_CULLING']
endif

add_project_arguments(global_cpp_defs, language: ['cpp', 'c'])
add_project_arguments(global_cpp_args, language: 'cpp')

subdir('targets/util')
subdir('targets/java')
subdir('targets/nbt')
subdir('targets/platform')

# MARK: platform configuration

if host_machine.system() in ['linux', 'windows', 'darwin', 'android']
    platform_fs_dep = platform_fs_std_dep
    platform_game_dep = platform_game_stub_dep
    platform_input_dep = platform_input_sdl2_dep
    platform_leaderboard_dep = platform_leaderboard_stub_dep
    platform_network_dep = platform_network_stub_dep
    platform_profile_dep = platform_profile_stub_dep
    platform_renderer_dep = platform_renderer_gl_dep
    platform_sound_dep = platform_sound_miniaudio_dep
    platform_storage_dep = platform_storage_stub_dep
    platform_thread_dep = platform_thread_dep

    if host_machine.system() == 'android'
        app_platform_sources = files(
            'targets/app/android/sdl_main_bridge.cpp',
            'targets/app/desktop/main.cpp'
        )
    else
        app_platform_sources = files('targets/app/desktop/main.cpp')
    endif
endif

subdir('targets/resources')
subdir('targets/minecraft')
subdir('targets/app')