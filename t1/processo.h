#ifndef PROCESSO_H
#define PROCESSO_H
#include "err.h"

typedef enum estado_processo
{
  BLOQUEADO,
  PRONTO,
  EXECUTANDO
} estado_processo;

typedef struct estado_cpu
{
  int registradorX;
  int registradorA;
  int registradorPC;
  err_t erro;
  int complemento;
} estado_cpu;

typedef struct processo_t
{
  int pid;
  char nome[100];
  estado_processo estado;
  int quantum;

  estado_cpu estado_cpu;
} processo_t;

typedef struct tabela_processos_t
{
  processo_t* processos;
  int quantidade_processos;
  estado_cpu estado_cpu;
} tabela_processos_t;

typedef struct fila_t
{
  int* processos;
  int quantidade_processos;
} fila_t;

tabela_processos_t* inicia_tabela_processos();
void adiciona_processo_na_tabela(tabela_processos_t* tabela_processos, char nome[100]);
processo_t* encontrar_processo_por_pid(tabela_processos_t* tabela, int targetPID);
bool remove_processo_tabela(tabela_processos_t* tabela, int targetPID);
fila_t* inicia_fila();
void adiciona_processo_na_fila(fila_t* fila, int pid);
void remove_processo_da_fila(fila_t* fila, int pid);
int quantum();

#endif // PROCESSO_H