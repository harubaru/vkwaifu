project('vkwaifu', ['c', 'cpp'], version : 'v0.10', meson_version : '>= 0.47')

compiler = meson.get_compiler('cpp')

if target_machine.system() == 'windows'
    add_global_link_arguments('-static', language: ['cpp', 'c'])
    include_dir = include_directories('dep/inc')
    lib_dir = meson.source_root() + '/dep/lib'
	lib_glfw = compiler.find_library('glfw3', dirs : lib_dir)
else
    add_global_link_arguments('-ldl', language: ['cpp', 'c'])
    include_dir = include_directories('/usr/include/GLFW','/usr/include/vulkan')
    lib_dir = '/usr/include'
    lib_glfw = compiler.find_library('glfw', dirs : lib_dir)
endif

subdir('src')

executable('vkwaifu', src, dependencies : [lib_glfw], include_directories : [include_dir])
