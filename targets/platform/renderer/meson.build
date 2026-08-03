cc = meson.get_compiler('cpp')

platform_renderer_gl_dependencies = [
    dependency('sdl2'),
    dependency('glm'),
    dependency('stb'),
    java_dep,
]
platform_renderer_gl_defs = []

if get_option('renderer') == 'gles' or host_machine.system() == 'android'
    platform_renderer_gl_dependencies += [
        cc.find_library('GLESv3', required: true),
        cc.find_library('GLESv2', required: false),
        cc.find_library('EGL', required: true),
        cc.find_library('log', required: true),
    ]
    platform_renderer_gl_defs = ['-DGLES', '-DGLES3', '-DGL_ES', '-D__ANDROID__']
else
    platform_renderer_gl_dependencies += [dependency('gl'), dependency('glew')]
    platform_renderer_gl_defs = []
endif

lib_platform_renderer_gl = static_library(
    'platform_renderer_gl',
    files('gl/GLRenderer.cpp', 'gl/render_stubs.cpp'),
    include_directories: include_directories('../../'),
    dependencies: platform_renderer_gl_dependencies,
    cpp_args: platform_renderer_gl_defs + global_cpp_args + global_cpp_defs,
)

platform_renderer_gl_dep = declare_dependency(
    link_with: lib_platform_renderer_gl,
)