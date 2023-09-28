#ifndef PROCESSO_H
#define PROCESSO_H

typedef struct processo_t
{
  int pid;
  char nome[100];
  int estado;
} processo_t;

typedef struct tabela_processos_t
{
  processo_t *processos;
  int quantidade_processos;
} tabela_processos_t;

tabela_processos_t *inicia_tabela_processos();
void adiciona_processo_na_tabela(tabela_processos_t *tabela_processos, char nome[100]);
processo_t *encontrar_processo_por_pid(tabela_processos_t *tabela, int targetPID);
bool remove_processo_tabela(tabela_processos_t *tabela, int targetPID);

#endif // PROCESSO_H