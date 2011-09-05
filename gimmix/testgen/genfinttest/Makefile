NAME = genfinttest
# IMPORTANT: force gcc to use the 387 floating point unit instead of SSE
# because we control the rounding-mode of the 387 FPU
CFLAGS += -I../../include -mfpmath=387
LIBS = ../../build/libmmix.a ../../build/libtest.a

-include ../DefMakefile.mk
