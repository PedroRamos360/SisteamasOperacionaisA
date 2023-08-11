#ifndef RANDOM_H
#define RANDOM_H

// registra valores aleatórios

#include "err.h"

typedef struct random_t random_t;

// cria e inicializa um dispositivo random
// retorna NULL em caso de erro
random_t *random_cria(void);

// destrói um dispositivo random
// nenhuma outra operação pode ser realizada no relógio após esta chamada
void random_destroi(random_t *self);

// gera valor aleatório
// esta função é chamada pelo controlador após a execução de cada instrução
void random_generate_value(random_t *self);

// retorna o valor aleatório
int random_valor(random_t *self);

// Funções para acessar o random como um dispositivo de E/S
//   só tem leitura, e um dispositivo, '0' para ler o valor random
err_t random_le(void *disp, int id, int *pvalor);
#endif // RANDOM_H
