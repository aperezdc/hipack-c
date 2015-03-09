CFLAGS += -std=c99
hipack_PATH ?= .
hipack_OBJS = ${hipack_PATH}/hipack-parser.o \
			  ${hipack_PATH}/hipack-string.o \
			  ${hipack_PATH}/hipack-list.o \
			  ${hipack_PATH}/hipack-dict.o
hipack = ${hipack_PATH}/libhipack.a

hipack: ${hipack}
hipack-clean:
	${RM} ${hipack} ${hipack_OBJS}

${hipack_OBJS}: ${hipack_PATH}/hipack.h
${hipack}: ${hipack_OBJS}
	${AR} rcu ${hipack} ${hipack_OBJS}

.PHONY: hipack hipack-objs
