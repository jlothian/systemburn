# 
# Makefile module for the 'planlib' directory
# 
DIR := planlib

PLAN_SRC := $(DIR)/plan_comm.c \
	$(DIR)/plan_dstream.c \
	$(DIR)/plan_dstride.c \
	$(DIR)/plan_gups.c \
	$(DIR)/plan_lstream.c \
	$(DIR)/plan_lstride.c \
	$(DIR)/plan_misc.c \
	$(DIR)/plan_pv1.c \
	$(DIR)/plan_pv2.c \
	$(DIR)/plan_pv3.c \
	$(DIR)/plan_pv4.c \
	$(DIR)/plan_sleep.c \
	$(DIR)/plan_write.c \
	$(DIR)/plan_cba.c \
	$(DIR)/plan_tilt.c \
	$(DIR)/plan_isort.c \
	$(DIR)/brand.c

ifeq ($(ENABLE_BLAS),1)
PLAN_SRC := $(PLAN_SRC) \
	-lm $(DIR)/plan_dgemm.c \
	-lm $(DIR)/plan_rdgemm.c
endif

ifeq ($(ENABLE_FFTW),1)
PLAN_SRC := $(PLAN_SRC) \
	$(DIR)/plan_fftw1d.c \
	$(DIR)/plan_fftw2d.c
endif

ifeq ($(ENABLE_CUDA),1)
PLAN_SRC := $(PLAN_SRC) \
	$(DIR)/plan_dcublas.c \
	$(DIR)/plan_cudamem.c \
	$(DIR)/plan_scublas.c
endif

ifeq ($(ENABLE_OPENCL),1)
PLAN_SRC := $(PLAN_SRC) \
	$(DIR)/plan_openclmem.c \
	$(DIR)/plan_dopenclblas.c \
        $(DIR)/plan_sopenclblas.c
endif
