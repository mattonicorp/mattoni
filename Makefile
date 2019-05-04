CC=gcc

SRCDIR=src
INCDIR=include
OBJDIR=obj

SRCS=${wildcard ${SRCDIR}/*.c}
INCS=${wildcard ${INCDIR}/*.h}
OBJS=${patsubst ${SRCDIR}/%.c,${OBJDIR}/%.o,${SRCS}}

EXEC=main
TRASH=${OBJDIR} ${EXEC}

CFLAGS=-I${INCDIR} -lm

$(shell mkdir -p ${OBJDIR})

${EXEC}: ${OBJS}
	${CC} ${CFLAGS} ${SRCS} -o $@ -l SDL2

${OBJDIR}/%.o: src/%.c ${HEADERS}
	${CC} ${CFLAGS} -c -o $@ $<

.PHONY: clean
clean:
	rm -rf ${TRASH}
