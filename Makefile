CC=gcc

SRCDIR=src
INCDIR=include
OBJDIR=obj
OUTDIR=out

SRCS=${wildcard ${SRCDIR}/*.c}
INCS=${wildcard ${INCDIR}/*.h}
OBJS=${patsubst ${SRCDIR}/%.c,${OBJDIR}/%.o,${SRCS}}

EXEC=main
TRASH=${OBJDIR} ${OUTDIR} ${EXEC} main.dSYM

CFLAGS=-I${INCDIR} -lm -lpthread -g

# Get SDL flags depending on OS
SDLFLAGS=$(shell sdl2-config --cflags)
SDLLIBS=$(shell sdl2-config --libs)

$(shell mkdir -p ${OBJDIR})
$(shell mkdir -p ${OUTDIR})

${EXEC}: ${OBJS}
	${CC} ${CFLAGS} ${SRCS} -o $@ ${SDLFLAGS} ${SDLLIBS}

${OBJDIR}/%.o: src/%.c ${HEADERS}
	${CC} ${CFLAGS} -c -o $@ $<

.PHONY: clean
clean:
	rm -rf ${TRASH}
