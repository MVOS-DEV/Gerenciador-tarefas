#include "tarefas.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INITIAL_CAPACITY 8

static const char *resolver_caminho(const char *caminho_arquivo) {
    if (caminho_arquivo == NULL || caminho_arquivo[0] == '\0') {
        return DATA_FILE;
    }

    return caminho_arquivo;
}

static int garantir_capacidade(ListaTarefas *lista) {
    size_t nova_capacidade = 0;
    Tarefa *novos_itens = NULL;

    if (lista->quantidade < lista->capacidade) {
        return 1;
    }

    nova_capacidade = (lista->capacidade == 0) ? INITIAL_CAPACITY : lista->capacidade * 2;
    novos_itens = realloc(lista->itens, nova_capacidade * sizeof(Tarefa));
    if (novos_itens == NULL) {
        return 0;
    }

    lista->itens = novos_itens;
    lista->capacidade = nova_capacidade;
    return 1;
}

static void copiar_texto(char *destino, size_t tamanho, const char *origem) {
    if (tamanho == 0) {
        return;
    }

    if (origem == NULL) {
        destino[0] = '\0';
        return;
    }

    strncpy(destino, origem, tamanho - 1);
    destino[tamanho - 1] = '\0';
}

static int texto_em_branco(const char *texto) {
    size_t i = 0;

    if (texto == NULL) {
        return 1;
    }

    for (i = 0; texto[i] != '\0'; i++) {
        if (!isspace((unsigned char)texto[i])) {
            return 0;
        }
    }

    return 1;
}

void inicializar_lista(ListaTarefas *lista) {
    if (lista == NULL) {
        return;
    }

    lista->itens = NULL;
    lista->quantidade = 0;
    lista->capacidade = 0;
    lista->proximo_id = 1;
}

void liberar_lista(ListaTarefas *lista) {
    if (lista == NULL) {
        return;
    }

    free(lista->itens);
    lista->itens = NULL;
    lista->quantidade = 0;
    lista->capacidade = 0;
    lista->proximo_id = 1;
}

int categoria_valida(Categoria categoria) {
    return categoria >= CATEGORIA_DIARIA && categoria <= CATEGORIA_ESTUDO;
}

const char *categoria_para_texto(Categoria categoria) {
    switch (categoria) {
        case CATEGORIA_DIARIA:
            return "Diaria";
        case CATEGORIA_PROJETO:
            return "Projeto";
        case CATEGORIA_ESTUDO:
            return "Estudo";
        default:
            return "Desconhecida";
    }
}

int salvar_tarefas(const ListaTarefas *lista, const char *caminho_arquivo) {
    const char *caminho = resolver_caminho(caminho_arquivo);
    FILE *arquivo = NULL;

    if (lista == NULL) {
        return 0;
    }

    arquivo = fopen(caminho, "wb");
    if (arquivo == NULL) {
        return 0;
    }

    if (fwrite(&lista->quantidade, sizeof(lista->quantidade), 1, arquivo) != 1) {
        fclose(arquivo);
        return 0;
    }

    if (lista->quantidade > 0 &&
        fwrite(lista->itens, sizeof(Tarefa), lista->quantidade, arquivo) != lista->quantidade) {
        fclose(arquivo);
        return 0;
    }

    fclose(arquivo);
    return 1;
}

int carregar_tarefas(ListaTarefas *lista, const char *caminho_arquivo) {
    const char *caminho = resolver_caminho(caminho_arquivo);
    FILE *arquivo = NULL;
    size_t quantidade = 0;
    size_t i = 0;

    if (lista == NULL) {
        return 0;
    }

    arquivo = fopen(caminho, "rb");
    if (arquivo == NULL) {
        return 1;
    }

    if (fread(&quantidade, sizeof(quantidade), 1, arquivo) != 1) {
        fclose(arquivo);
        return 0;
    }

    while (lista->capacidade < quantidade) {
        if (!garantir_capacidade(lista)) {
            fclose(arquivo);
            return 0;
        }
    }

    if (quantidade > 0 && fread(lista->itens, sizeof(Tarefa), quantidade, arquivo) != quantidade) {
        fclose(arquivo);
        return 0;
    }

    lista->quantidade = quantidade;
    lista->proximo_id = 1;

    for (i = 0; i < lista->quantidade; i++) {
        if (lista->itens[i].id >= lista->proximo_id) {
            lista->proximo_id = lista->itens[i].id + 1;
        }
    }

    fclose(arquivo);
    return 1;
}

int adicionar_tarefa(
    ListaTarefas *lista,
    Categoria categoria,
    const char *titulo,
    const char *descricao,
    const char *prazo
) {
    Tarefa *tarefa = NULL;

    if (lista == NULL || !categoria_valida(categoria) || texto_em_branco(titulo)) {
        return 0;
    }

    if (!garantir_capacidade(lista)) {
        return 0;
    }

    tarefa = &lista->itens[lista->quantidade];
    memset(tarefa, 0, sizeof(*tarefa));
    tarefa->id = lista->proximo_id++;
    tarefa->categoria = categoria;
    tarefa->concluida = 0;
    copiar_texto(tarefa->titulo, sizeof(tarefa->titulo), titulo);
    copiar_texto(tarefa->descricao, sizeof(tarefa->descricao), descricao);
    copiar_texto(tarefa->prazo, sizeof(tarefa->prazo), prazo);

    lista->quantidade++;
    return 1;
}

Tarefa *buscar_tarefa_por_id(ListaTarefas *lista, int id) {
    size_t i = 0;

    if (lista == NULL) {
        return NULL;
    }

    for (i = 0; i < lista->quantidade; i++) {
        if (lista->itens[i].id == id) {
            return &lista->itens[i];
        }
    }

    return NULL;
}

const Tarefa *buscar_tarefa_por_id_const(const ListaTarefas *lista, int id) {
    size_t i = 0;

    if (lista == NULL) {
        return NULL;
    }

    for (i = 0; i < lista->quantidade; i++) {
        if (lista->itens[i].id == id) {
            return &lista->itens[i];
        }
    }

    return NULL;
}

int concluir_tarefa(ListaTarefas *lista, int id) {
    Tarefa *tarefa = buscar_tarefa_por_id(lista, id);

    if (tarefa == NULL) {
        return 0;
    }

    tarefa->concluida = 1;
    return 1;
}

int remover_tarefa(ListaTarefas *lista, int id) {
    size_t i = 0;

    if (lista == NULL) {
        return 0;
    }

    for (i = 0; i < lista->quantidade; i++) {
        if (lista->itens[i].id == id) {
            size_t j = 0;

            for (j = i; j + 1 < lista->quantidade; j++) {
                lista->itens[j] = lista->itens[j + 1];
            }

            lista->quantidade--;
            return 1;
        }
    }

    return 0;
}

int contar_tarefas_pendentes(const ListaTarefas *lista) {
    size_t i = 0;
    int pendentes = 0;

    if (lista == NULL) {
        return 0;
    }

    for (i = 0; i < lista->quantidade; i++) {
        if (!lista->itens[i].concluida) {
            pendentes++;
        }
    }

    return pendentes;
}
