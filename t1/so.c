#include "so.h"
#include "irq.h"
#include "programa.h"
#include "processo.h"
#include "instrucao.h"

#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

// intervalo entre interrupções do relógio
#define INTERVALO_INTERRUPCAO 50 // em instruções executadas

int id_processo_executando = -1;

struct so_t
{
  cpu_t* cpu;
  mem_t* mem;
  console_t* console;
  relogio_t* relogio;
  tabela_processos_t* tabela_processos;
};

// função de tratamento de interrupção (entrada no SO)
static err_t so_trata_interrupcao(void* argC, int reg_A);

// funções auxiliares
static int so_carrega_programa(so_t* self, char* nome_do_executavel);
static bool copia_str_da_mem(int tam, char str[tam], mem_t* mem, int ender);

so_t* so_cria(cpu_t* cpu, mem_t* mem, console_t* console, relogio_t* relogio)
{
  so_t* self = malloc(sizeof(*self));
  if (self == NULL)
    return NULL;

  self->cpu = cpu;
  self->mem = mem;
  self->console = console;
  self->relogio = relogio;
  self->tabela_processos = inicia_tabela_processos();

  // quando a CPU executar uma instrução CHAMAC, deve chamar a função
  //   so_trata_interrupcao
  cpu_define_chamaC(self->cpu, so_trata_interrupcao, self);

  // coloca o tratador de interrupção na memória
  // quando a CPU aceita uma interrupção, passa para modo supervisor,
  //   salva seu estado à partir do endereço 0, e desvia para o endereço 10
  // colocamos no endereço 10 a instrução CHAMAC, que vai chamar
  //   so_trata_interrupcao (conforme foi definido acima) e no endereço 11
  //   colocamos a instrução RETI, para que a CPU retorne da interrupção
  //   (recuperando seu estado no endereço 0) depois que o SO retornar de
  //   so_trata_interrupcao.
  mem_escreve(self->mem, 10, CHAMAC);
  mem_escreve(self->mem, 11, RETI);

  // programa o relógio para gerar uma interrupção após INTERVALO_INTERRUPCAO
  rel_escr(self->relogio, 2, INTERVALO_INTERRUPCAO);

  return self;
}

void so_destroi(so_t* self)
{
  cpu_define_chamaC(self->cpu, NULL, NULL);
  free(self);
}

// Tratamento de interrupção

// funções auxiliares para tratar cada tipo de interrupção
static err_t so_trata_irq(so_t* self, int irq);
static err_t so_trata_irq_reset(so_t* self);
static err_t so_trata_irq_err_cpu(so_t* self);
static err_t so_trata_irq_relogio(so_t* self);
static err_t so_trata_irq_desconhecida(so_t* self, int irq);
static err_t so_trata_chamada_sistema(so_t* self);

// funções auxiliares para o tratamento de interrupção
// static void so_salva_estado_da_cpu(so_t* self);
static void so_trata_pendencias(so_t* self);
static void so_escalona(so_t* self);

// funções Pedro Ramos :)
processo_t* so_cria_processo(so_t* self, char nome[100]);
void so_salva_estado_cpu_no_processo(so_t* self);
void so_carrega_estado_processo_na_cpu(so_t* self);

// função a ser chamada pela CPU quando executa a instrução CHAMAC
// essa instrução só deve ser executada quando for tratar uma interrupção
// o primeiro argumento é um ponteiro para o SO, o segundo é a identificação
//   da interrupção
// na inicialização do SO é colocada no endereço 10 uma rotina que executa
//   CHAMAC; quando recebe uma interrupção, a CPU salva os registradores
//   no endereço 0, e desvia para o endereço 10
static err_t so_trata_interrupcao(void* argC, int reg_A)
{
  so_t* self = argC;
  irq_t irq = reg_A;
  err_t err;
  console_printf(self->console, "SO: recebi IRQ %d (%s)", irq, irq_nome(irq));
  // salva o estado da cpu no descritor do processo que foi interrompido
  // so_salva_estado_da_cpu(self);
  so_salva_estado_cpu_no_processo(self);
  // faz o atendimento da interrupção
  err = so_trata_irq(self, irq);
  // faz o processamento independente da interrupção
  so_trata_pendencias(self);
  // escolhe o próximo processo a executar
  so_escalona(self);
  // recupera o estado do processo escolhido
  so_carrega_estado_processo_na_cpu(self);
  return err;
}

void so_salva_estado_cpu_no_processo(so_t* self) {
  processo_t* processo_atual = encontrar_processo_por_pid(self->tabela_processos, id_processo_executando);
  if (processo_atual == NULL) return;
  console_printf(self->console, "SO: Salva estado da cpu no processo %s", processo_atual->nome);
  mem_le(self->mem, IRQ_END_X, &processo_atual->estado_cpu.registradorX);
  mem_le(self->mem, IRQ_END_A, &processo_atual->estado_cpu.registradorA);
  mem_le(self->mem, IRQ_END_PC, &processo_atual->estado_cpu.registradorPC);
  mem_le(self->mem, IRQ_END_complemento, &processo_atual->estado_cpu.complemento);
  int leitura_memoria;
  mem_le(self->mem, IRQ_END_erro, &leitura_memoria);
  processo_atual->estado_cpu.erro = leitura_memoria;
}

void so_carrega_estado_processo_na_cpu(so_t* self) {
  processo_t* processo_atual = encontrar_processo_por_pid(self->tabela_processos, id_processo_executando);
  if (processo_atual == NULL) {
    mem_escreve(self->mem, IRQ_END_erro, ERR_CPU_PARADA);
    return;
  }
  console_printf(self->console, "SO: Carrega estado do processo %s na cpu", processo_atual->nome);
  mem_escreve(self->mem, IRQ_END_X, processo_atual->estado_cpu.registradorX);
  mem_escreve(self->mem, IRQ_END_A, processo_atual->estado_cpu.registradorA);
  mem_escreve(self->mem, IRQ_END_PC, processo_atual->estado_cpu.registradorPC);
  mem_escreve(self->mem, IRQ_END_erro, processo_atual->estado_cpu.erro);
  mem_escreve(self->mem, IRQ_END_complemento, processo_atual->estado_cpu.complemento);
}

static void so_trata_pendencias(so_t* self)
{
  // realiza ações que não são diretamente ligadas com a interrupção que
  // está sendo atendida:
  // - E/S pendente
  // - desbloqueio de processos
  // - contabilidades

  // Verifica se há E/S pendente

  // Verifica se há processos bloqueados
  // for (int i = 0; i < self->tabela_processos->quantidade_processos; i++)
  // {
  //   processo_t* processo = &self->tabela_processos->processos[i];
  //   if (processo->estado == 2)
  //   {
  //     // Se houver, verifica se o processo pode ser desbloqueado
  //     if (pode_desbloquear(processo))
  //     {
  //       // Se o processo pode ser desbloqueado, atualiza o seu estado
  //       processo->estado = 1;
  //     }
  //   }
  // }

  // Atualiza as contabilidades
}
static void so_escalona(so_t* self)
{
  processo_t* processo_executando = encontrar_processo_por_pid(self->tabela_processos, id_processo_executando);

  if (processo_executando == NULL)
  {
    if (self->tabela_processos->quantidade_processos == 0)
    {
      id_processo_executando = -1;
      return;
    }
    processo_t* proximo_processo = pega_proximo_processo_disponivel(self->tabela_processos);
    id_processo_executando = proximo_processo->pid;
    proximo_processo->estado = EXECUTANDO;
  }
  else
  {
    processo_t* copia_processo = cria_processo(processo_executando->pid, processo_executando->nome, processo_executando->estado);
    if (copia_processo->estado == PRONTO)
    {
      char so_message[200];
      sprintf(so_message, "SO: quantum do processo: %s expirado, escalonando processo...", copia_processo->nome);
      console_printf(self->console, so_message);
      // joga o processo pro fim da fila
      remove_processo_tabela(self->tabela_processos, copia_processo->pid);
      adiciona_processo_na_tabela(self->tabela_processos, copia_processo);
      processo_t* proximo_processo = pega_proximo_processo_disponivel(self->tabela_processos);
      id_processo_executando = proximo_processo->pid;
      proximo_processo->estado = EXECUTANDO;
    }
    so_salva_estado_cpu_no_processo(self);
  }
}

static err_t so_trata_irq(so_t* self, int irq)
{
  err_t err;
  console_printf(self->console, "SO: recebi IRQ %d (%s)", irq, irq_nome(irq));
  switch (irq)
  {
  case IRQ_RESET:
    err = so_trata_irq_reset(self);
    break;
  case IRQ_ERR_CPU:
    err = so_trata_irq_err_cpu(self);
    break;
  case IRQ_SISTEMA:
    err = so_trata_chamada_sistema(self);
    break;
  case IRQ_RELOGIO:
    err = so_trata_irq_relogio(self);
    break;
  default:
    err = so_trata_irq_desconhecida(self, irq);
  }
  return err;
}

static err_t so_trata_irq_reset(so_t* self)
{
  // coloca um programa na memória
  char nome_programa[100] = "init.maq";
  int ender = so_carrega_programa(self, nome_programa);
  processo_t* processo_adicionado = so_cria_processo(self, nome_programa);

  if (ender != 100)
  {
    console_printf(self->console, "SO: problema na carga do programa inicial");
    return ERR_CPU_PARADA;
  }
  processo_adicionado->estado_cpu.registradorPC = ender;

  mem_escreve(self->mem, IRQ_END_modo, usuario);
  return ERR_OK;
}

static err_t so_trata_irq_err_cpu(so_t* self)
{
  // Ocorreu um erro interno na CPU
  // O erro está codificado em IRQ_END_erro
  // Em geral, causa a morte do processo que causou o erro
  // Ainda não temos processos, causa a parada da CPU
  int err_int;
  // com suporte a processos, deveria pegar o valor do registrador erro
  //   no descritor do processo corrente, e reagir de acordo com esse erro
  //   (em geral, matando o processo)

  // COMENTÁRIO PEDRO: Meus processos não dão erro, então vou deixar pra implementar depois
  mem_le(self->mem, IRQ_END_erro, &err_int);
  err_t err = err_int;
  console_printf(self->console,
    "SO: IRQ não tratada -- erro na CPU: %s", err_nome(err));
  return ERR_CPU_PARADA;
}

static err_t so_trata_irq_relogio(so_t* self)
{
  // ocorreu uma interrupção do relógio
  // rearma o interruptor do relógio e reinicializa o timer para a próxima interrupção
  rel_escr(self->relogio, 3, 0); // desliga o sinalizador de interrupção
  rel_escr(self->relogio, 2, INTERVALO_INTERRUPCAO);
  // trata a interrupção
  // por exemplo, decrementa o quantum do processo corrente, quando se tem
  // um escalonador com quantum
  processo_t* processo_atual = encontrar_processo_por_pid(self->tabela_processos, id_processo_executando);
  if (processo_atual != NULL)
  {
    processo_atual->quantum--;
    if (processo_atual->quantum == 0)
    {
      processo_atual->estado = PRONTO;
      processo_atual->quantum = quantum();
    }
  }
  console_printf(self->console, "SO: interrupção do relógio (não tratada)");
  return ERR_OK;
}

static err_t so_trata_irq_desconhecida(so_t* self, int irq)
{
  console_printf(self->console,
    "SO: não sei tratar IRQ %d (%s)", irq, irq_nome(irq));
  return ERR_CPU_PARADA;
}

// Chamadas de sistema

static void so_chamada_le(so_t* self);
static void so_chamada_escr(so_t* self);
static void so_chamada_cria_proc(so_t* self);
static void so_chamada_mata_proc(so_t* self);
static void so_chamada_espera_proc(so_t* self);

static err_t so_trata_chamada_sistema(so_t* self)
{
  // com processos, a identificação da chamada está no reg A no descritor
  //   do processo
  processo_t* processo_atual = encontrar_processo_por_pid(self->tabela_processos, id_processo_executando);
  if (processo_atual == NULL) {
    return ERR_CPU_PARADA;
  }
  int id_chamada = processo_atual->estado_cpu.registradorA;
  console_printf(self->console,
    "SO: chamada de sistema %d", id_chamada);
  switch (id_chamada)
  {
  case SO_LE:
    so_chamada_le(self);
    break;
  case SO_ESCR:
    so_chamada_escr(self);
    break;
  case SO_CRIA_PROC:
    so_chamada_cria_proc(self);
    break;
  case SO_MATA_PROC:
    so_chamada_mata_proc(self);
    break;
  case SO_ESPERA_PROC:
    so_chamada_espera_proc(self);
    break;
  default:
    console_printf(self->console,
      "SO: chamada de sistema desconhecida (%d)", id_chamada);
    return ERR_CPU_PARADA;
  }
  return ERR_OK;
}

static void so_chamada_le(so_t* self)
{
  // implementação com espera ocupada
  //   deveria bloquear o processo se leitura não disponível.
  //   no caso de bloqueio do processo, a leitura (e desbloqueio) deverá
  //   ser feita mais tarde, em tratamentos pendentes em outra interrupção,
  //   ou diretamente em uma interrupção específica do dispositivo, se for
  //   o caso
  // implementação lendo direto do terminal A
  //   deveria usar dispositivo corrente de entrada do processo

  // leitura
  int terminal_dl = (id_processo_executando * 4) + 0;
  // estado leitura
  int terminal_el = (id_processo_executando * 4) + 1;
  for (;;)
  {
    int estado;
    term_le(self->console, terminal_el, &estado);
    if (estado != 0)
      break;
    // como não está saindo do SO, o laço do processador não tá rodando
    // esta gambiarra faz o console andar
    // com a implementação de bloqueio de processo, esta gambiarra não
    //   deve mais existir.
    console_tictac(self->console);
    console_atualiza(self->console);
  }
  int dado;
  term_le(self->console, terminal_dl, &dado);
  // com processo, deveria escrever no reg A do processo
  processo_t* processo_atual = encontrar_processo_por_pid(self->tabela_processos, id_processo_executando);
  processo_atual->estado_cpu.registradorA = dado;
}

static void so_chamada_escr(so_t* self)
{
  // implementação com espera ocupada
  //   deveria bloquear o processo se dispositivo ocupado
  // implementação escrevendo direto do terminal A
  //   deveria usar dispositivo corrente de saída do processo

  // escrita
  int terminal_de = (id_processo_executando * 4) + 2;
  // estado escrita
  int terminal_ee = (id_processo_executando * 4) + 3;

  for (;;)
  {
    int estado;
    term_le(self->console, terminal_ee, &estado);
    if (estado != 0)
      break;
    // como não está saindo do SO, o laço do processador não tá rodando
    // esta gambiarra faz o console andar
    console_tictac(self->console);
    console_atualiza(self->console);
  }
  processo_t* processo_atual = encontrar_processo_por_pid(self->tabela_processos, id_processo_executando);
  if (processo_atual == NULL) {
    return;
  }
  term_escr(self->console, terminal_de, processo_atual->estado_cpu.registradorX);
  processo_atual->estado_cpu.registradorA = 0;
}

processo_t* so_cria_processo(so_t* self, char nome[100])
{
  adiciona_novo_processo_na_tabela(self->tabela_processos, nome);
  processo_t* processo_carregado = &self->tabela_processos->processos[self->tabela_processos->quantidade_processos - 1];

  char so_message[200];
  sprintf(so_message, "SO: Processo criado Nome: %s PID: %d", processo_carregado->nome, processo_carregado->pid);

  console_printf(self->console, so_message);
  return processo_carregado;
}

static void so_chamada_cria_proc(so_t* self)
{
  console_printf(self->console, "SO: chamada cria processo");
  // em X está o endereço onde está o nome do arquivo
  int ender_proc;
  // deveria ler o X do descritor do processo criador
  if (mem_le(self->mem, IRQ_END_X, &ender_proc) == ERR_OK)
  {
    char nome[100];
    if (copia_str_da_mem(100, nome, self->mem, ender_proc))
    {
      int ender_carga = so_carrega_programa(self, nome);
      so_cria_processo(self, nome);

      if (ender_carga > 0)
      {
        // deveria escrever no PC do descritor do processo criado
        mem_escreve(self->mem, IRQ_END_PC, ender_carga);
        return;
      }
    }
  }
  // deveria escrever -1 (se erro) ou 0 (se OK) no reg A do processo que
  //   pediu a criação
  mem_escreve(self->mem, IRQ_END_A, -1);
}

static void so_chamada_espera_proc(so_t* self) {
  console_printf(self->console, "SO: chamada espera processo");
  processo_t* processo_esperador = encontrar_processo_por_pid(self->tabela_processos, id_processo_executando);
  processo_t* processo_esperado = &self->tabela_processos->processos[processo_esperador->estado_cpu.registradorX];

  processo_esperador->esperando_processo = processo_esperado;
  processo_esperador->estado = BLOQUEADO;
  console_printf(self->console, "SO: processo %s BLOQUEADO, esperando processo %s", processo_esperador->nome, processo_esperado->nome);
}

static void so_chamada_mata_proc(so_t* self)
{
  processo_t* processo_atual = encontrar_processo_por_pid(self->tabela_processos, id_processo_executando);
  if (processo_atual == NULL) {
    return;
  }
  console_printf(self->console, "SO: Removendo processo %s PID: %d da tabela", processo_atual->nome, processo_atual->pid);
  remove_processo_tabela(self->tabela_processos, id_processo_executando);
}

// carrega o programa na memória
// retorna o endereço de carga ou -1
static int so_carrega_programa(so_t* self, char* nome_do_executavel)
{
  // programa para executar na nossa CPU
  programa_t* prog = prog_cria(nome_do_executavel);
  if (prog == NULL)
  {
    console_printf(self->console,
      "Erro na leitura do programa '%s'\n", nome_do_executavel);
    return -1;
  }

  int end_ini = prog_end_carga(prog);
  int end_fim = end_ini + prog_tamanho(prog);

  for (int end = end_ini; end < end_fim; end++)
  {
    if (mem_escreve(self->mem, end, prog_dado(prog, end)) != ERR_OK)
    {
      console_printf(self->console,
        "Erro na carga da memória, endereco %d\n", end);
      return -1;
    }
  }
  prog_destroi(prog);
  console_printf(self->console,
    "SO: carga de '%s' em %d-%d", nome_do_executavel, end_ini, end_fim);
  return end_ini;
}

// copia uma string da memória do simulador para o vetor str.
// retorna false se erro (string maior que vetor, valor não ascii na memória,
//   erro de acesso à memória)
static bool copia_str_da_mem(int tam, char str[tam], mem_t* mem, int ender)
{
  for (int indice_str = 0; indice_str < tam; indice_str++)
  {
    int caractere;
    if (mem_le(mem, ender + indice_str, &caractere) != ERR_OK)
    {
      return false;
    }
    if (caractere < 0 || caractere > 255)
    {
      return false;
    }
    str[indice_str] = caractere;
    if (caractere == 0)
    {
      return true;
    }
  }
  // estourou o tamanho de str
  return false;
}
