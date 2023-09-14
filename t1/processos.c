#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct process_t {
    int pid;
    char name[50];
    int state;
	int priority;
};

#define MAX_PROCESSES 10

struct process_t processTable[MAX_PROCESSES];

struct process_t* runningProcess = NULL;

void saveProcessState(struct process_t* process) {
    if (process != NULL) {
        process->state = 0;
    }
}

void restoreProcessState(struct process_t* process) {
    if (process != NULL) {
        process->state = 1;
    }
}

// Função de escalonamento simples (escolhe o próximo processo pronto)
struct process_t* scheduler() {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (processTable[i].state == 0) {
            return &processTable[i];
        }
    }
    return NULL; // Nenhum processo pronto
}

// Função para criar um novo processo
struct process_t* createProcess(int pid, const char* name) {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (processTable[i].pid == -1) { // Encontrou uma entrada vazia na tabela
            struct process_t* newProcess = &processTable[i];
            newProcess->pid = pid;
            strcpy(newProcess->name, name);
            saveProcessState(newProcess);
            return newProcess;
        }
    }
    return NULL; // Tabela de processos cheia
}

// Função para encerrar um processo
void killProcess(struct process_t* process) {
    if (process != NULL) {
        process->pid = -1; // Marca a entrada na tabela como vazia
    }
}

// Função para tratamento de interrupção
void interruptHandler(int interruptType) {
    // Salva o estado do processo em execução
    saveProcessState(runningProcess);
    
    // Executa o tratamento correspondente à interrupção
    // (implementação depende das interrupções específicas)
    
    // Executa uma função para tratar de pendências (se necessário)
    
    // Executa o escalonador para escolher o próximo processo a ser executado
    runningProcess = scheduler();
    
    // Restaura o estado do novo processo em execução
    restoreProcessState(runningProcess);
}

int main() {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        processTable[i].pid = -1; // Inicializa com -1 para indicar entradas vazias
    }
    
    // Cria processos de exemplo
	printf("Processo 1\n");
    struct process_t* process1 = createProcess(1, "Process1");
	printf("Processo 2\n");
    struct process_t* process2 = createProcess(2, "Process2");
    
    // Inicialmente, nenhum processo está em execução
    runningProcess = NULL;
    
    // Simulação de tratamento de interrupção
	printf("Interrupção 0\n");
    interruptHandler(0);
    
    // Simulação de tratamento de interrupção
	printf("Interrupção 1\n");
    interruptHandler(1);
    
    // Encerra os processos de exemplo
	printf("Mata processos\n");
    killProcess(process1);
    killProcess(process2);
    
    return 0;
}
