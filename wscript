top = '.'
out = 'build'

def options(opt):
    opt.load('compiler_cxx')

def configure(conf):
    conf.find_program('clang++-3.6', var='CXX', mandatory=True)
    conf.load('compiler_cxx')
    conf.env.CXXFLAGS = ['-std=c++11', '-O3', '-g', '-Wall']

def build(bld):
    bld.program(source='experiments/memory_order/memory_order.cpp',
                target='experiments/test_memory_order',
                lib='pthread',
                stlib='benchmark')

    bld.program(source='experiments/memory_reordering/memory_reordering.cpp',
                target='experiments/test_memory_reordering',
                lib='pthread')
