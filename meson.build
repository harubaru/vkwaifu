project('vkwaifu', ['c', 'cpp'], version : 'v0.10', meson_version : '>= 0.47')

compiler = meson.get_compiler('cpp')
include_dir = include_directories('dep/inc')
lib_dir = meson.source_root() + '/dep/lib'

add_global_link_arguments('-static', language: ['cpp', 'c'])

if target_machine.system() == 'windows'
	lib_glfw = compiler.find_library('glfw3', dirs : lib_dir)
else
	lib_glfw = compiler.find_library('glfw')
endif

subdir('src')

executable('vkwaifu', src, dependencies : [lib_glfw], include_directories : [include_dir])
