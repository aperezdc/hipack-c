CFLAGS += -std=c99
hipack_PATH ?= .
hipack_OBJS = ${hipack_PATH}/hipack-parser.o
hipack = ${hipack_PATH}/libhipack.a

hipack: ${hipack}
hipack-clean:
	${RM} ${hipack} ${hipack_OBJS}

${hipack_OBJS}: ${hipack_PATH}/hipack.h
${hipack}: ${hipack_OBJS}
	${AR} rcu ${hipack} ${hipack_OBJS}

.PHONY: hipack hipack-objs
