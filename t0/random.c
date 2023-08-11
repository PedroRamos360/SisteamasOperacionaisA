#include "random.h"
#include <stdlib.h>
#include <time.h>
#include <stdio.h>

struct random_t {
  int valor;
};

random_t *random_cria(void)
{
    srand(time(NULL));
    random_t *self;
    self = malloc(sizeof(random_t));
    if (self != NULL) {
        self->valor = 0;
    }
    return self;
}

void random_destroi(random_t *self)
{
    free(self);
}

void random_generate_value(random_t *self)
{
    int randomValue = rand() % 10;
    self->valor = randomValue;
}

int random_valor(random_t *self)
{
  return self->valor;
}

err_t random_le(void *disp, int id, int *pvalor)
{
  random_t *self = disp;
  err_t err = ERR_OK;
  switch (id) {
    case 0:
      *pvalor = self->valor;
      break;
    default: 
      err = ERR_END_INV;
  }
  return err;
}
