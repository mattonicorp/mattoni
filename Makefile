CC=gcc

SRCDIR=src
INCDIR=include
OBJDIR=obj

SRCS=${wildcard ${SRCDIR}/*.c}
INCS=${wildcard ${INCDIR}/*.h}
OBJS=${patsubst ${SRCDIR}/%.c,${OBJDIR}/%.o,${SRCS}}

EXEC=main
TRASH=${OBJDIR} ${EXEC}

CFLAGS=-I${INCDIR} -lm -lpthread

# Get SDL flags depending on OS
SDLFLAGS=$(shell sdl2-config --cflags)
SDLLIBS=$(shell sdl2-config --libs)

$(shell mkdir -p ${OBJDIR})

${EXEC}: ${OBJS}
	${CC} ${CFLAGS} ${SRCS} -o $@ ${SDLFLAGS} ${SDLLIBS}

${OBJDIR}/%.o: src/%.c ${HEADERS}
	${CC} ${CFLAGS} -c -o $@ $<

.PHONY: clean
clean:
	rm -rf ${TRASH}
