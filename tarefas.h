#ifndef TAREFAS_H
#define TAREFAS_H

#include <stddef.h>

#define DATA_FILE "tarefas.dat"
#define TITLE_SIZE 80
#define DESCRIPTION_SIZE 200
#define DATE_SIZE 20

typedef enum {
    CATEGORIA_DIARIA = 1,
    CATEGORIA_PROJETO = 2,
    CATEGORIA_ESTUDO = 3
} Categoria;

typedef struct {
    int id;
    Categoria categoria;
    int concluida;
    char titulo[TITLE_SIZE];
    char descricao[DESCRIPTION_SIZE];
    char prazo[DATE_SIZE];
} Tarefa;

typedef struct {
    Tarefa *itens;
    size_t quantidade;
    size_t capacidade;
    int proximo_id;
} ListaTarefas;

void inicializar_lista(ListaTarefas *lista);
void liberar_lista(ListaTarefas *lista);
int carregar_tarefas(ListaTarefas *lista, const char *caminho_arquivo);
int salvar_tarefas(const ListaTarefas *lista, const char *caminho_arquivo);
int adicionar_tarefa(
    ListaTarefas *lista,
    Categoria categoria,
    const char *titulo,
    const char *descricao,
    const char *prazo
);
Tarefa *buscar_tarefa_por_id(ListaTarefas *lista, int id);
const Tarefa *buscar_tarefa_por_id_const(const ListaTarefas *lista, int id);
int concluir_tarefa(ListaTarefas *lista, int id);
int remover_tarefa(ListaTarefas *lista, int id);
int contar_tarefas_pendentes(const ListaTarefas *lista);
int categoria_valida(Categoria categoria);
const char *categoria_para_texto(Categoria categoria);

#endif
