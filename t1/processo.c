#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uuid/uuid.h>
#include <stdbool.h>
#include "err.h"

int QUANTUM = 1;

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
  struct processo_t* esperando_processo;

  estado_cpu* estado_cpu;
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

void generate_uuid_str(char* uuid_str)
{
  uuid_t uuid;
  uuid_generate(uuid);

  uuid_unparse(uuid, uuid_str);
}

processo_t* cria_processo(int pid, char nome[100], int estado)
{
  processo_t* novo_processo = (processo_t*)malloc(sizeof(processo_t));
  if (novo_processo == NULL)
  {
    return NULL;
  }

  strncpy(novo_processo->nome, nome, sizeof(novo_processo->nome));
  novo_processo->pid = pid;
  novo_processo->estado = estado;
  novo_processo->quantum = QUANTUM;
  return novo_processo;
}

tabela_processos_t* inicia_tabela_processos()
{
  tabela_processos_t* tabela_processos = (tabela_processos_t*)malloc(sizeof(tabela_processos_t));
  tabela_processos->processos = NULL;
  tabela_processos->quantidade_processos = 0;

  return tabela_processos;
}


void adiciona_processo_na_tabela(tabela_processos_t* tabela_processos, char nome[100])
{

  if (tabela_processos == NULL)
  {
    return;
  }

  int pid;

  if (tabela_processos->quantidade_processos == 0)
  {
    pid = 0;
  }
  else
  {
    pid = tabela_processos->processos[tabela_processos->quantidade_processos - 1].pid + 1;
  }

  processo_t* novo_processo = cria_processo(pid, nome, EXECUTANDO);
  if (novo_processo == NULL)
  {
    return;
  }

  processo_t* novo_array_processos = (processo_t*)realloc(tabela_processos->processos, (tabela_processos->quantidade_processos + 1) * sizeof(processo_t));
  if (novo_array_processos == NULL)
  {
    return;
  }

  tabela_processos->processos = novo_array_processos;
  tabela_processos->processos[tabela_processos->quantidade_processos] = *novo_processo;
  tabela_processos->quantidade_processos++;
}

processo_t* encontrar_processo_por_pid(tabela_processos_t* tabela, int targetPID)
{
  for (int i = 0; i < tabela->quantidade_processos; i++)
  {
    if (tabela->processos[i].pid == targetPID)
    {
      return &(tabela->processos[i]);
    }
  }

  return NULL;
}

bool remove_processo_tabela(tabela_processos_t* tabela, int targetPID)
{
  for (int i = 0; i < tabela->quantidade_processos; i++)
  {
    if (tabela->processos[i].pid == targetPID)
    {
      for (int j = i; j < tabela->quantidade_processos - 1; j++)
      {
        tabela->processos[j] = tabela->processos[j + 1];
      }
      tabela->quantidade_processos--;
      return true;
    }
  }

  return false;
}

fila_t* inicia_fila() {
  fila_t* fila = (fila_t*)malloc(sizeof(fila_t));
  fila->processos = NULL;
  fila->quantidade_processos = 0;

  return fila;
}

void adiciona_processo_na_fila(fila_t* fila, int pid) {
  int* novo_array_processos = (int*)realloc(fila->processos, (fila->quantidade_processos + 1) * sizeof(int));
  if (novo_array_processos == NULL)
  {
    return;
  }

  fila->processos = novo_array_processos;
  fila->processos[fila->quantidade_processos] = pid;
  fila->quantidade_processos++;
}

void remove_processo_da_fila(fila_t* fila, int pid) {
  for (int i = 0; i < fila->quantidade_processos; i++)
  {
    if (fila->processos[i] == pid)
    {
      for (int j = i; j < fila->quantidade_processos - 1; j++)
      {
        fila->processos[j] = fila->processos[j + 1];
      }
      fila->quantidade_processos--;
      return;
    }
  }
}

int quantum() {
  return QUANTUM;
}
