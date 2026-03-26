#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DATA_FILE "tarefas.dat"
#define INITIAL_CAPACITY 8
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

static void limpar_buffer_entrada(void) {
    int c = 0;

    while ((c = getchar()) != '\n' && c != EOF) {
    }
}

static void ler_linha(const char *mensagem, char *destino, size_t tamanho) {
    size_t comprimento = 0;

    printf("%s", mensagem);
    if (fgets(destino, (int)tamanho, stdin) == NULL) {
        destino[0] = '\0';
        return;
    }

    comprimento = strlen(destino);
    if (comprimento > 0 && destino[comprimento - 1] == '\n') {
        destino[comprimento - 1] = '\0';
    } else {
        limpar_buffer_entrada();
    }
}

static int ler_inteiro(const char *mensagem) {
    char linha[32];
    char *fim = NULL;
    long valor = 0;

    for (;;) {
        ler_linha(mensagem, linha, sizeof(linha));
        if (linha[0] == '\0') {
            printf("Digite um numero valido.\n");
            continue;
        }

        valor = strtol(linha, &fim, 10);
        if (*fim == '\0') {
            return (int)valor;
        }

        printf("Entrada invalida. Tente novamente.\n");
    }
}

static void inicializar_lista(ListaTarefas *lista) {
    lista->itens = NULL;
    lista->quantidade = 0;
    lista->capacidade = 0;
    lista->proximo_id = 1;
}

static void liberar_lista(ListaTarefas *lista) {
    free(lista->itens);
    lista->itens = NULL;
    lista->quantidade = 0;
    lista->capacidade = 0;
    lista->proximo_id = 1;
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
        printf("Nao foi possivel reservar memoria.\n");
        return 0;
    }

    lista->itens = novos_itens;
    lista->capacidade = nova_capacidade;
    return 1;
}

static const char *categoria_para_texto(Categoria categoria) {
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

static Categoria escolher_categoria(void) {
    int opcao = 0;

    for (;;) {
        printf("\nCategorias disponiveis:\n");
        printf("1. Diaria\n");
        printf("2. Projeto\n");
        printf("3. Estudo\n");
        opcao = ler_inteiro("Escolha a categoria: ");

        if (opcao >= 1 && opcao <= 3) {
            return (Categoria)opcao;
        }

        printf("Categoria invalida.\n");
    }
}

static int salvar_tarefas(const ListaTarefas *lista) {
    FILE *arquivo = fopen(DATA_FILE, "wb");

    if (arquivo == NULL) {
        printf("Nao foi possivel salvar as tarefas em '%s'.\n", DATA_FILE);
        return 0;
    }

    if (fwrite(&lista->quantidade, sizeof(lista->quantidade), 1, arquivo) != 1) {
        fclose(arquivo);
        printf("Falha ao salvar a quantidade de tarefas.\n");
        return 0;
    }

    if (lista->quantidade > 0 &&
        fwrite(lista->itens, sizeof(Tarefa), lista->quantidade, arquivo) != lista->quantidade) {
        fclose(arquivo);
        printf("Falha ao salvar os dados das tarefas.\n");
        return 0;
    }

    fclose(arquivo);
    return 1;
}

static int carregar_tarefas(ListaTarefas *lista) {
    FILE *arquivo = fopen(DATA_FILE, "rb");
    size_t quantidade = 0;
    size_t i = 0;

    if (arquivo == NULL) {
        return 1;
    }

    if (fread(&quantidade, sizeof(quantidade), 1, arquivo) != 1) {
        fclose(arquivo);
        printf("Arquivo de dados invalido. Iniciando lista vazia.\n");
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
        printf("Falha ao ler tarefas salvas.\n");
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

static void imprimir_tarefa(const Tarefa *tarefa) {
    printf("\n[%d] %s\n", tarefa->id, tarefa->titulo);
    printf("Categoria : %s\n", categoria_para_texto(tarefa->categoria));
    printf("Status    : %s\n", tarefa->concluida ? "Concluida" : "Pendente");
    printf("Prazo     : %s\n", tarefa->prazo[0] == '\0' ? "Nao definido" : tarefa->prazo);
    printf("Descricao : %s\n", tarefa->descricao[0] == '\0' ? "Sem descricao" : tarefa->descricao);
}

static void listar_tarefas(const ListaTarefas *lista, int somente_pendentes, Categoria filtro_categoria) {
    size_t i = 0;
    int encontrou = 0;

    printf("\n========== LISTA DE TAREFAS ==========\n");
    for (i = 0; i < lista->quantidade; i++) {
        const Tarefa *tarefa = &lista->itens[i];

        if (somente_pendentes && tarefa->concluida) {
            continue;
        }

        if (filtro_categoria != 0 && tarefa->categoria != filtro_categoria) {
            continue;
        }

        imprimir_tarefa(tarefa);
        encontrou = 1;
    }

    if (!encontrou) {
        printf("Nenhuma tarefa encontrada com esse filtro.\n");
    }
}

static Tarefa *buscar_por_id(ListaTarefas *lista, int id) {
    size_t i = 0;

    for (i = 0; i < lista->quantidade; i++) {
        if (lista->itens[i].id == id) {
            return &lista->itens[i];
        }
    }

    return NULL;
}

static void adicionar_tarefa(ListaTarefas *lista) {
    Tarefa nova_tarefa;

    if (!garantir_capacidade(lista)) {
        return;
    }

    memset(&nova_tarefa, 0, sizeof(nova_tarefa));
    nova_tarefa.id = lista->proximo_id++;
    nova_tarefa.categoria = escolher_categoria();
    ler_linha("Titulo: ", nova_tarefa.titulo, sizeof(nova_tarefa.titulo));
    ler_linha("Descricao: ", nova_tarefa.descricao, sizeof(nova_tarefa.descricao));
    ler_linha("Prazo (ex: 2026-04-15 ou deixe vazio): ", nova_tarefa.prazo, sizeof(nova_tarefa.prazo));
    nova_tarefa.concluida = 0;

    if (nova_tarefa.titulo[0] == '\0') {
        printf("O titulo nao pode ficar vazio.\n");
        lista->proximo_id--;
        return;
    }

    lista->itens[lista->quantidade++] = nova_tarefa;
    if (salvar_tarefas(lista)) {
        printf("Tarefa adicionada com sucesso.\n");
    }
}

static void concluir_tarefa(ListaTarefas *lista) {
    int id = ler_inteiro("Digite o ID da tarefa concluida: ");
    Tarefa *tarefa = buscar_por_id(lista, id);

    if (tarefa == NULL) {
        printf("Nenhuma tarefa encontrada com esse ID.\n");
        return;
    }

    if (tarefa->concluida) {
        printf("Essa tarefa ja esta concluida.\n");
        return;
    }

    tarefa->concluida = 1;
    if (salvar_tarefas(lista)) {
        printf("Tarefa marcada como concluida.\n");
    }
}

static void remover_tarefa(ListaTarefas *lista) {
    int id = ler_inteiro("Digite o ID da tarefa para remover: ");
    size_t i = 0;

    for (i = 0; i < lista->quantidade; i++) {
        if (lista->itens[i].id == id) {
            size_t j = 0;

            for (j = i; j + 1 < lista->quantidade; j++) {
                lista->itens[j] = lista->itens[j + 1];
            }

            lista->quantidade--;
            if (salvar_tarefas(lista)) {
                printf("Tarefa removida com sucesso.\n");
            }
            return;
        }
    }

    printf("Nenhuma tarefa encontrada com esse ID.\n");
}

static void listar_por_categoria(const ListaTarefas *lista) {
    Categoria categoria = escolher_categoria();
    listar_tarefas(lista, 0, categoria);
}

static void mostrar_menu(void) {
    printf("\n========== GERENCIADOR DE TAREFAS ==========\n");
    printf("1. Adicionar tarefa\n");
    printf("2. Listar todas as tarefas\n");
    printf("3. Listar tarefas pendentes\n");
    printf("4. Listar por categoria\n");
    printf("5. Marcar tarefa como concluida\n");
    printf("6. Remover tarefa\n");
    printf("0. Sair\n");
}

int main(void) {
    ListaTarefas lista;
    int opcao = -1;

    inicializar_lista(&lista);
    carregar_tarefas(&lista);

    printf("Bem-vindo ao seu gerenciador de tarefas em C.\n");

    while (opcao != 0) {
        mostrar_menu();
        opcao = ler_inteiro("Escolha uma opcao: ");

        switch (opcao) {
            case 1:
                adicionar_tarefa(&lista);
                break;
            case 2:
                listar_tarefas(&lista, 0, 0);
                break;
            case 3:
                listar_tarefas(&lista, 1, 0);
                break;
            case 4:
                listar_por_categoria(&lista);
                break;
            case 5:
                concluir_tarefa(&lista);
                break;
            case 6:
                remover_tarefa(&lista);
                break;
            case 0:
                printf("Encerrando o programa.\n");
                break;
            default:
                printf("Opcao invalida.\n");
        }
    }

    liberar_lista(&lista);
    return 0;
}
