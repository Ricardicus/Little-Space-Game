project('pop' , ['c'], version: '1.0.0')

pop_inc = include_directories('src')
pop_src = ['src/pop.c']

thread_dep = dependency('threads')
x11_lib = dependency('X11', required : true)

pop_exe = executable(
  'pop',
  sources: pop_src,
  dependencies: [x11_lib,thread_dep],
  include_directories: pop_inc
)
