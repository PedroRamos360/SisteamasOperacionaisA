#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

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

processo_t *cria_processo(int pid, char nome[100], int estado)
{
  processo_t *novo_processo = (processo_t *)malloc(sizeof(processo_t));
  // Tratamento de erro de alocação, diz o GPT
  if (novo_processo == NULL)
  {
    return NULL;
  }
  novo_processo->pid = pid;
  strncpy(novo_processo->nome, nome, sizeof(novo_processo->nome));
  novo_processo->estado = estado;
  return novo_processo;
}

tabela_processos_t *inicia_tabela_processos()
{
  tabela_processos_t *tabela_processos = (tabela_processos_t *)malloc(sizeof(tabela_processos_t));
  tabela_processos->processos = NULL;
  tabela_processos->quantidade_processos = 0;

  return tabela_processos;
}

void adiciona_processo_na_tabela(tabela_processos_t *tabela_processos, char nome[100])
{

  if (tabela_processos == NULL)
  {
    return;
  }
  processo_t *novo_processo = cria_processo(tabela_processos->quantidade_processos + 1, nome, 0);
  if (novo_processo == NULL)
  {
    return;
  }

  processo_t *novo_array_processos = (processo_t *)realloc(tabela_processos->processos, (tabela_processos->quantidade_processos + 1) * sizeof(processo_t));
  if (novo_array_processos == NULL)
  {
    return;
  }

  tabela_processos->processos = novo_array_processos;
  tabela_processos->processos[tabela_processos->quantidade_processos] = *novo_processo;
  tabela_processos->quantidade_processos++;
}

processo_t *encontrar_processo_por_pid(tabela_processos_t *tabela, int targetPID)
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

bool remove_processo_tabela(tabela_processos_t *tabela, int targetPID)
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
