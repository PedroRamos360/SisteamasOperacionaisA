CC = gcc
CFLAGS = -Wall -Werror
LDLIBS = -lcurses

OBJS = cpu.o es.o memoria.o relogio.o console.o instrucao.o err.o \
			 teste.o programa.o controle.o random.o
OBJS_MONT = instrucao.o err.o montador.o
MAQS = ex1.maq ex2.maq ex3.maq ex4.maq ex5.maq ex6.maq t0.maq
TARGETS = teste montador ${MAQS}

all: ${TARGETS}

# para gerar o montador, precisa de todos os .o do montador
montador: ${OBJS_MONT}

# para gerar o programa de teste, precisa de todos os .o)
teste: ${OBJS}

# para transformar um .asm em .maq, precisamos do montador
%.maq: %.asm montador
	./montador $*.asm > $*.maq

# apaga os arquivos gerados
clean:
	rm -f ${OBJS} ${OBJS_MONT} ${TARGETS} ${MAQS} ${OBJS:.o=.d}

# para calcular as dependências de cada arquivo .c (e colocar no .d)
%.d: %.c
	@set -e; rm -f $@; \
	 $(CC) -MM $(CPPFLAGS) $< > /tmp/$@.$$$$; \
	 sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < /tmp/$@.$$$$ > $@; \
	 rm -f /tmp/$@.$$$$

# inclui as dependências
include $(OBJS:.o=.d)
