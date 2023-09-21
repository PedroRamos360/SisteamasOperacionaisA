#include <stdio.h>
#include <stdlib.h>
#include <string.h>


struct processo_t {
  int pid;
  char nome[32];
  int estado;
};

struct tabela_processos_t {
  struct processo_t *processos;
  int quantidade_processos;
};

struct processo_t *cria_processo_na_tabela(int pid, char nome[32], int estado) {
  struct processo_t *novo_processo = (struct processo_t*)malloc(sizeof(struct processo_t));
  // Tratamento de erro de alocação, diz o GPT
  if (novo_processo == NULL) {
    return NULL;
  }
  novo_processo->pid = pid;
  strncpy(novo_processo->nome, nome, sizeof(novo_processo->nome));
  novo_processo->estado = estado;
  return novo_processo;
}

struct tabela_processos_t *inicia_tabela_processos() {
  struct tabela_processos_t *tabela_processos = (struct tabela_processos_t*)malloc(sizeof(struct tabela_processos_t));
  tabela_processos->processos = NULL;
  tabela_processos->quantidade_processos = 0;

  return tabela_processos;
}

void adiciona_processo(struct tabela_processos_t *tabela_processos, struct processo_t *novo_processo) {
  if (tabela_processos == NULL || novo_processo == NULL) {
    return;
  }

  
  struct processo_t *novo_array_processos = (struct processo_t*)realloc(tabela_processos->processos, (tabela_processos->quantidade_processos + 1) * sizeof(struct processo_t));
  if (novo_array_processos == NULL) {
    return;
  }

  tabela_processos->processos = novo_array_processos;
  tabela_processos->processos[tabela_processos->quantidade_processos] = *novo_processo;
  tabela_processos->quantidade_processos++;
}

