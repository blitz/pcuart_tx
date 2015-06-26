# -*- Mode: Python -*-

env = Environment(CXXFLAGS = ' -g -Wall -Ofast -std=c++11 ', LINKFLAGS = ' -g ')
env.ParseConfig('pkg-config --cflags --libs sdl2')

env.Program('pcuart_tx',
            ['tx.cpp'])


# EOF
