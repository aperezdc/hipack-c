CFLAGS += -std=c99
hipack_PATH ?= .
hipack_OBJS = ${hipack_PATH}/hipack-parser.o \
			  ${hipack_PATH}/hipack-writer.o \
			  ${hipack_PATH}/hipack-string.o \
			  ${hipack_PATH}/hipack-alloc.o \
			  ${hipack_PATH}/hipack-list.o \
			  ${hipack_PATH}/hipack-dict.o \
			  ${hipack_PATH}/hipack-misc.o
hipack = ${hipack_PATH}/libhipack.a

hipack: ${hipack}
hipack-clean:
	${RM} ${hipack} ${hipack_OBJS}
	${RM} ${hipack_PATH}/tools/*.o \
		${hipack_PATH}/tools/hipack-cat \
		${hipack_PATH}/tools/hipack-get \
		${hipack_PATH}/tools/hipack-parse \
		${hipack_PATH}/tools/hipack-roundtrip \
		${hipack_PATH}/tools/hipack-test-api

${hipack_OBJS}: ${hipack_PATH}/hipack.h
${hipack}: ${hipack_OBJS}
	${AR} rc ${hipack} ${hipack_OBJS}

hipack-tools: \
	${hipack_PATH}/tools/hipack-cat \
	${hipack_PATH}/tools/hipack-get \
	${hipack_PATH}/tools/hipack-parse \
	${hipack_PATH}/tools/hipack-roundtrip \
	${hipack_PATH}/tools/hipack-test-api

${hipack_PATH}/tools/hipack-cat: \
	${hipack_PATH}/tools/hipack-cat.o ${hipack}

${hipack_PATH}/tools/hipack-get: \
	${hipack_PATH}/tools/hipack-get.o ${hipack}

${hipack_PATH}/tools/hipack-parse: \
	${hipack_PATH}/tools/hipack-parse.o ${hipack}

${hipack_PATH}/tools/hipack-roundtrip: \
	${hipack_PATH}/tools/hipack-roundtrip.o ${hipack}

${hipack_PATH}/tools/hipack-test-api: \
	${hipack_PATH}/tools/hipack-test-api.o ${hipack}

hipack-check: hipack-tools
	@${hipack_PATH}/tools/hipack-test-api
	@bash --norc ${hipack_PATH}/tools/run-tests

${hipack_PATH}/hipack-writer.o: ${hipack_PATH}/fpconv/src/fpconv.c
${hipack_PATH}/fpconv/src/fpconv.c: ${hipack_PATH}/.gitmodules
	cd ${hipack_PATH} && git submodule init fpconv
	cd ${hipack_PATH} && git submodule update fpconv
	touch $@

${hipack_PATH}/doc/apiref.rst: ${hipack_PATH}/hipack.h ${hipack_PATH}/tools/extract-docs.awk
	awk -f ${hipack_PATH}/tools/extract-docs.awk $< > $@

doc: ${hipack_PATH}/doc/apiref.rst
	${MAKE} -C ${hipack_PATH}/doc html

.PHONY: hipack hipack-objs hipack-tools hipack-check hipack-clean doc
