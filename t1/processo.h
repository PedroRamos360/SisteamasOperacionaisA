typedef struct tabela_processos_t tabela_processos_t;
typedef struct processo_t processo_t;

struct tabela_processos_t *inicia_tabela_processos();
struct processo_t *cria_processo(int pid, char nome[32], int estado);