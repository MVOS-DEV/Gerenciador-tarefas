#include <gtk/gtk.h>
#include <stdio.h>

#include "tarefas.h"

#if GLIB_CHECK_VERSION(2, 74, 0)
#define APP_FLAGS G_APPLICATION_DEFAULT_FLAGS
#else
#define APP_FLAGS G_APPLICATION_FLAGS_NONE
#endif

enum {
    COLUNA_ID = 0,
    COLUNA_TITULO,
    COLUNA_CATEGORIA,
    COLUNA_PRAZO,
    COLUNA_STATUS,
    TOTAL_COLUNAS
};

typedef struct {
    ListaTarefas lista;
    GtkWidget *janela;
    GtkWidget *entrada_titulo;
    GtkWidget *entrada_prazo;
    GtkWidget *texto_descricao;
    GtkWidget *combo_categoria;
    GtkWidget *combo_filtro;
    GtkWidget *rotulo_resumo;
    GtkWidget *rotulo_status;
    GtkWidget *rotulo_detalhes;
    GtkWidget *arvore_tarefas;
    GtkListStore *modelo;
} AppGtk;

static void mostrar_dialogo(GtkWindow *janela, GtkMessageType tipo, const char *titulo, const char *mensagem) {
    GtkWidget *dialogo = gtk_message_dialog_new(
        janela,
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        tipo,
        GTK_BUTTONS_OK,
        "%s",
        mensagem
    );

    gtk_window_set_title(GTK_WINDOW(dialogo), titulo);
    gtk_dialog_run(GTK_DIALOG(dialogo));
    gtk_widget_destroy(dialogo);
}

static void aplicar_estilo(void) {
    GtkCssProvider *provedor = gtk_css_provider_new();
    const gchar *css =
        "window { background: #f5f0e7; }\n"
        "#titulo-app { font-size: 28px; font-weight: 800; color: #17372d; }\n"
        "#resumo-app { color: #4f635b; font-weight: 600; }\n"
        "#status-app { color: #5d4a1a; font-weight: 600; }\n"
        "frame { background: rgba(255,255,255,0.92); border-radius: 14px; padding: 8px; }\n"
        "treeview.view { font-size: 13px; }\n";

    gtk_css_provider_load_from_data(provedor, css, -1, NULL);
    gtk_style_context_add_provider_for_screen(
        gdk_screen_get_default(),
        GTK_STYLE_PROVIDER(provedor),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
    );
    g_object_unref(provedor);
}

static void atualizar_status(AppGtk *app, const char *mensagem) {
    gtk_label_set_text(GTK_LABEL(app->rotulo_status), mensagem);
}

static void atualizar_resumo(AppGtk *app) {
    char resumo[128];

    g_snprintf(
        resumo,
        sizeof(resumo),
        "%lu tarefas registradas | %d pendentes",
        (unsigned long)app->lista.quantidade,
        contar_tarefas_pendentes(&app->lista)
    );
    gtk_label_set_text(GTK_LABEL(app->rotulo_resumo), resumo);
}

static void atualizar_detalhes(AppGtk *app, const Tarefa *tarefa) {
    char texto[512];

    if (tarefa == NULL) {
        gtk_label_set_text(
            GTK_LABEL(app->rotulo_detalhes),
            "Selecione uma tarefa para ver a descricao completa."
        );
        return;
    }

    g_snprintf(
        texto,
        sizeof(texto),
        "Titulo: %s\nCategoria: %s\nStatus: %s\nPrazo: %s\n\nDescricao:\n%s",
        tarefa->titulo,
        categoria_para_texto(tarefa->categoria),
        tarefa->concluida ? "Concluida" : "Pendente",
        tarefa->prazo[0] == '\0' ? "Nao definido" : tarefa->prazo,
        tarefa->descricao[0] == '\0' ? "Sem descricao." : tarefa->descricao
    );
    gtk_label_set_text(GTK_LABEL(app->rotulo_detalhes), texto);
}

static gboolean tarefa_passa_filtro(const Tarefa *tarefa, gint filtro) {
    switch (filtro) {
        case 0:
            return TRUE;
        case 1:
            return !tarefa->concluida;
        case 2:
            return tarefa->categoria == CATEGORIA_DIARIA;
        case 3:
            return tarefa->categoria == CATEGORIA_PROJETO;
        case 4:
            return tarefa->categoria == CATEGORIA_ESTUDO;
        default:
            return TRUE;
    }
}

static gboolean obter_id_selecionado(AppGtk *app, int *id_saida) {
    GtkTreeSelection *selecao = gtk_tree_view_get_selection(GTK_TREE_VIEW(app->arvore_tarefas));
    GtkTreeIter iter;
    GtkTreeModel *modelo = NULL;

    if (!gtk_tree_selection_get_selected(selecao, &modelo, &iter)) {
        return FALSE;
    }

    gtk_tree_model_get(modelo, &iter, COLUNA_ID, id_saida, -1);
    return TRUE;
}

static void atualizar_lista_visual(AppGtk *app) {
    size_t i = 0;
    gint filtro = gtk_combo_box_get_active(GTK_COMBO_BOX(app->combo_filtro));

    gtk_list_store_clear(app->modelo);

    for (i = 0; i < app->lista.quantidade; i++) {
        const Tarefa *tarefa = &app->lista.itens[i];
        GtkTreeIter iter;

        if (!tarefa_passa_filtro(tarefa, filtro)) {
            continue;
        }

        gtk_list_store_append(app->modelo, &iter);
        gtk_list_store_set(
            app->modelo,
            &iter,
            COLUNA_ID,
            tarefa->id,
            COLUNA_TITULO,
            tarefa->titulo,
            COLUNA_CATEGORIA,
            categoria_para_texto(tarefa->categoria),
            COLUNA_PRAZO,
            tarefa->prazo[0] == '\0' ? "Nao definido" : tarefa->prazo,
            COLUNA_STATUS,
            tarefa->concluida ? "Concluida" : "Pendente",
            -1
        );
    }

    atualizar_resumo(app);
    atualizar_detalhes(app, NULL);
}

static void limpar_formulario(AppGtk *app) {
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(app->texto_descricao));

    gtk_entry_set_text(GTK_ENTRY(app->entrada_titulo), "");
    gtk_entry_set_text(GTK_ENTRY(app->entrada_prazo), "");
    gtk_combo_box_set_active(GTK_COMBO_BOX(app->combo_categoria), 0);
    gtk_text_buffer_set_text(buffer, "", -1);
}

static void selecionar_tarefa_na_lista(GtkTreeSelection *selecao, gpointer user_data) {
    AppGtk *app = user_data;
    int id = 0;
    const Tarefa *tarefa = NULL;

    (void)selecao;

    if (!obter_id_selecionado(app, &id)) {
        atualizar_detalhes(app, NULL);
        return;
    }

    tarefa = buscar_tarefa_por_id_const(&app->lista, id);
    atualizar_detalhes(app, tarefa);
}

static void trocar_filtro(GtkComboBox *combo, gpointer user_data) {
    AppGtk *app = user_data;
    (void)combo;

    atualizar_lista_visual(app);
    atualizar_status(app, "Filtro atualizado.");
}

static int salvar_lista_ou_avisar(AppGtk *app) {
    if (salvar_tarefas(&app->lista, DATA_FILE)) {
        return 1;
    }

    mostrar_dialogo(
        GTK_WINDOW(app->janela),
        GTK_MESSAGE_ERROR,
        "Falha ao salvar",
        "Nao foi possivel salvar as tarefas em disco."
    );
    return 0;
}

static void clicar_adicionar(GtkButton *botao, gpointer user_data) {
    AppGtk *app = user_data;
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(app->texto_descricao));
    GtkTextIter inicio;
    GtkTextIter fim;
    gchar *descricao = NULL;
    const gchar *titulo = gtk_entry_get_text(GTK_ENTRY(app->entrada_titulo));
    const gchar *prazo = gtk_entry_get_text(GTK_ENTRY(app->entrada_prazo));
    gint categoria = gtk_combo_box_get_active(GTK_COMBO_BOX(app->combo_categoria)) + 1;

    (void)botao;

    gtk_text_buffer_get_bounds(buffer, &inicio, &fim);
    descricao = gtk_text_buffer_get_text(buffer, &inicio, &fim, FALSE);

    if (!adicionar_tarefa(&app->lista, (Categoria)categoria, titulo, descricao, prazo)) {
        mostrar_dialogo(
            GTK_WINDOW(app->janela),
            GTK_MESSAGE_WARNING,
            "Tarefa invalida",
            "Preencha pelo menos o titulo da tarefa e escolha uma categoria valida."
        );
        g_free(descricao);
        return;
    }

    if (!salvar_lista_ou_avisar(app)) {
        g_free(descricao);
        return;
    }

    atualizar_lista_visual(app);
    limpar_formulario(app);
    atualizar_status(app, "Tarefa adicionada com sucesso.");
    g_free(descricao);
}

static void clicar_concluir(GtkButton *botao, gpointer user_data) {
    AppGtk *app = user_data;
    int id = 0;
    Tarefa *tarefa = NULL;

    (void)botao;

    if (!obter_id_selecionado(app, &id)) {
        mostrar_dialogo(
            GTK_WINDOW(app->janela),
            GTK_MESSAGE_INFO,
            "Nenhuma selecao",
            "Selecione uma tarefa na lista antes de marcar como concluida."
        );
        return;
    }

    tarefa = buscar_tarefa_por_id(&app->lista, id);
    if (tarefa == NULL) {
        atualizar_lista_visual(app);
        return;
    }

    if (tarefa->concluida) {
        atualizar_status(app, "A tarefa selecionada ja estava concluida.");
        return;
    }

    concluir_tarefa(&app->lista, id);
    if (!salvar_lista_ou_avisar(app)) {
        return;
    }

    atualizar_lista_visual(app);
    atualizar_status(app, "Tarefa concluida.");
}

static void clicar_remover(GtkButton *botao, gpointer user_data) {
    AppGtk *app = user_data;
    GtkWidget *dialogo = NULL;
    gint resposta = GTK_RESPONSE_CANCEL;
    int id = 0;

    (void)botao;

    if (!obter_id_selecionado(app, &id)) {
        mostrar_dialogo(
            GTK_WINDOW(app->janela),
            GTK_MESSAGE_INFO,
            "Nenhuma selecao",
            "Selecione uma tarefa na lista antes de remover."
        );
        return;
    }

    dialogo = gtk_message_dialog_new(
        GTK_WINDOW(app->janela),
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        GTK_MESSAGE_QUESTION,
        GTK_BUTTONS_YES_NO,
        "Deseja remover a tarefa selecionada?"
    );
    gtk_window_set_title(GTK_WINDOW(dialogo), "Confirmar remocao");
    resposta = gtk_dialog_run(GTK_DIALOG(dialogo));
    gtk_widget_destroy(dialogo);

    if (resposta != GTK_RESPONSE_YES) {
        atualizar_status(app, "Remocao cancelada.");
        return;
    }

    if (!remover_tarefa(&app->lista, id)) {
        atualizar_lista_visual(app);
        return;
    }

    if (!salvar_lista_ou_avisar(app)) {
        return;
    }

    atualizar_lista_visual(app);
    atualizar_status(app, "Tarefa removida.");
}

static void criar_coluna(GtkTreeView *arvore, const char *titulo, int indice_coluna) {
    GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
    GtkTreeViewColumn *coluna = gtk_tree_view_column_new_with_attributes(
        titulo,
        renderer,
        "text",
        indice_coluna,
        NULL
    );

    gtk_tree_view_append_column(arvore, coluna);
}

static GtkWidget *montar_formulario(AppGtk *app) {
    GtkWidget *frame = gtk_frame_new("Nova tarefa");
    GtkWidget *grid = gtk_grid_new();
    GtkWidget *rotulo_titulo = gtk_label_new("Titulo");
    GtkWidget *rotulo_categoria = gtk_label_new("Categoria");
    GtkWidget *rotulo_prazo = gtk_label_new("Prazo");
    GtkWidget *rotulo_descricao = gtk_label_new("Descricao");
    GtkWidget *scroll_descricao = gtk_scrolled_window_new(NULL, NULL);
    GtkWidget *botao_adicionar = gtk_button_new_with_label("Adicionar tarefa");

    gtk_container_set_border_width(GTK_CONTAINER(frame), 10);
    gtk_container_add(GTK_CONTAINER(frame), grid);
    gtk_grid_set_row_spacing(GTK_GRID(grid), 10);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 10);

    gtk_label_set_xalign(GTK_LABEL(rotulo_titulo), 0.0f);
    gtk_label_set_xalign(GTK_LABEL(rotulo_categoria), 0.0f);
    gtk_label_set_xalign(GTK_LABEL(rotulo_prazo), 0.0f);
    gtk_label_set_xalign(GTK_LABEL(rotulo_descricao), 0.0f);

    app->entrada_titulo = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(app->entrada_titulo), "Ex: Estudar calculo");

    app->combo_categoria = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(app->combo_categoria), "Diaria");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(app->combo_categoria), "Projeto");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(app->combo_categoria), "Estudo");
    gtk_combo_box_set_active(GTK_COMBO_BOX(app->combo_categoria), 0);

    app->entrada_prazo = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(app->entrada_prazo), "Ex: 2026-04-15");

    app->texto_descricao = gtk_text_view_new();
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(app->texto_descricao), GTK_WRAP_WORD_CHAR);

    gtk_widget_set_size_request(scroll_descricao, -1, 140);
    gtk_container_add(GTK_CONTAINER(scroll_descricao), app->texto_descricao);
    gtk_scrolled_window_set_policy(
        GTK_SCROLLED_WINDOW(scroll_descricao),
        GTK_POLICY_AUTOMATIC,
        GTK_POLICY_AUTOMATIC
    );

    gtk_style_context_add_class(
        gtk_widget_get_style_context(botao_adicionar),
        GTK_STYLE_CLASS_SUGGESTED_ACTION
    );
    g_signal_connect(botao_adicionar, "clicked", G_CALLBACK(clicar_adicionar), app);

    gtk_grid_attach(GTK_GRID(grid), rotulo_titulo, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), app->entrada_titulo, 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), rotulo_categoria, 0, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), app->combo_categoria, 0, 3, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), rotulo_prazo, 0, 4, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), app->entrada_prazo, 0, 5, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), rotulo_descricao, 0, 6, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), scroll_descricao, 0, 7, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), botao_adicionar, 0, 8, 1, 1);

    return frame;
}

static GtkWidget *montar_lista(AppGtk *app) {
    GtkWidget *frame = gtk_frame_new("Tarefas");
    GtkWidget *caixa = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    GtkWidget *linha_filtro = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    GtkWidget *rotulo_filtro = gtk_label_new("Mostrar");
    GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
    GtkWidget *linha_botoes = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    GtkWidget *botao_concluir = gtk_button_new_with_label("Concluir selecionada");
    GtkWidget *botao_remover = gtk_button_new_with_label("Remover selecionada");
    GtkWidget *frame_detalhes = gtk_frame_new("Detalhes");
    GtkWidget *caixa_detalhes = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    GtkTreeSelection *selecao = NULL;

    gtk_container_set_border_width(GTK_CONTAINER(frame), 10);
    gtk_container_add(GTK_CONTAINER(frame), caixa);

    gtk_label_set_xalign(GTK_LABEL(rotulo_filtro), 0.0f);
    app->combo_filtro = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(app->combo_filtro), "Todas");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(app->combo_filtro), "Pendentes");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(app->combo_filtro), "Diaria");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(app->combo_filtro), "Projeto");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(app->combo_filtro), "Estudo");
    gtk_combo_box_set_active(GTK_COMBO_BOX(app->combo_filtro), 0);
    g_signal_connect(app->combo_filtro, "changed", G_CALLBACK(trocar_filtro), app);

    gtk_box_pack_start(GTK_BOX(linha_filtro), rotulo_filtro, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(linha_filtro), app->combo_filtro, FALSE, FALSE, 0);

    app->modelo = gtk_list_store_new(
        TOTAL_COLUNAS,
        G_TYPE_INT,
        G_TYPE_STRING,
        G_TYPE_STRING,
        G_TYPE_STRING,
        G_TYPE_STRING
    );
    app->arvore_tarefas = gtk_tree_view_new_with_model(GTK_TREE_MODEL(app->modelo));
    criar_coluna(GTK_TREE_VIEW(app->arvore_tarefas), "ID", COLUNA_ID);
    criar_coluna(GTK_TREE_VIEW(app->arvore_tarefas), "Titulo", COLUNA_TITULO);
    criar_coluna(GTK_TREE_VIEW(app->arvore_tarefas), "Categoria", COLUNA_CATEGORIA);
    criar_coluna(GTK_TREE_VIEW(app->arvore_tarefas), "Prazo", COLUNA_PRAZO);
    criar_coluna(GTK_TREE_VIEW(app->arvore_tarefas), "Status", COLUNA_STATUS);

    selecao = gtk_tree_view_get_selection(GTK_TREE_VIEW(app->arvore_tarefas));
    g_signal_connect(selecao, "changed", G_CALLBACK(selecionar_tarefa_na_lista), app);

    gtk_scrolled_window_set_policy(
        GTK_SCROLLED_WINDOW(scroll),
        GTK_POLICY_AUTOMATIC,
        GTK_POLICY_AUTOMATIC
    );
    gtk_widget_set_vexpand(scroll, TRUE);
    gtk_container_add(GTK_CONTAINER(scroll), app->arvore_tarefas);

    g_signal_connect(botao_concluir, "clicked", G_CALLBACK(clicar_concluir), app);
    g_signal_connect(botao_remover, "clicked", G_CALLBACK(clicar_remover), app);

    gtk_box_pack_start(GTK_BOX(linha_botoes), botao_concluir, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(linha_botoes), botao_remover, FALSE, FALSE, 0);

    app->rotulo_detalhes = gtk_label_new("Selecione uma tarefa para ver a descricao completa.");
    gtk_label_set_xalign(GTK_LABEL(app->rotulo_detalhes), 0.0f);
    gtk_label_set_yalign(GTK_LABEL(app->rotulo_detalhes), 0.0f);
    gtk_label_set_line_wrap(GTK_LABEL(app->rotulo_detalhes), TRUE);
    gtk_box_pack_start(GTK_BOX(caixa_detalhes), app->rotulo_detalhes, TRUE, TRUE, 0);
    gtk_container_add(GTK_CONTAINER(frame_detalhes), caixa_detalhes);

    gtk_box_pack_start(GTK_BOX(caixa), linha_filtro, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(caixa), scroll, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(caixa), linha_botoes, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(caixa), frame_detalhes, FALSE, FALSE, 0);

    return frame;
}

static void destruir_janela(GtkWidget *widget, gpointer user_data) {
    AppGtk *app = user_data;
    (void)widget;

    salvar_tarefas(&app->lista, DATA_FILE);
    if (app->modelo != NULL) {
        g_object_unref(app->modelo);
    }
    liberar_lista(&app->lista);
    g_free(app);
}

static void ativar(GtkApplication *application, gpointer user_data) {
    AppGtk *app = g_new0(AppGtk, 1);
    GtkWidget *janela = NULL;
    GtkWidget *conteudo = gtk_box_new(GTK_ORIENTATION_VERTICAL, 16);
    GtkWidget *cabecalho = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    GtkWidget *titulo = gtk_label_new("Painel de tarefas");
    GtkWidget *subtitulo = gtk_label_new("Organize estudos, projetos e rotina em uma unica tela.");
    GtkWidget *corpo = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 16);
    int carregou = 0;

    (void)user_data;

    inicializar_lista(&app->lista);
    carregou = carregar_tarefas(&app->lista, DATA_FILE);

    aplicar_estilo();

    janela = gtk_application_window_new(application);
    app->janela = janela;

    gtk_window_set_title(GTK_WINDOW(janela), "Gerenciador de Tarefas");
    gtk_window_set_default_size(GTK_WINDOW(janela), 1024, 640);
    gtk_container_set_border_width(GTK_CONTAINER(janela), 16);

    gtk_widget_set_name(titulo, "titulo-app");
    gtk_widget_set_name(subtitulo, "resumo-app");

    app->rotulo_resumo = gtk_label_new("");
    app->rotulo_status = gtk_label_new("Pronto para cadastrar novas tarefas.");
    gtk_widget_set_name(app->rotulo_resumo, "resumo-app");
    gtk_widget_set_name(app->rotulo_status, "status-app");

    gtk_label_set_xalign(GTK_LABEL(titulo), 0.0f);
    gtk_label_set_xalign(GTK_LABEL(subtitulo), 0.0f);
    gtk_label_set_xalign(GTK_LABEL(app->rotulo_resumo), 0.0f);
    gtk_label_set_xalign(GTK_LABEL(app->rotulo_status), 0.0f);

    gtk_box_pack_start(GTK_BOX(cabecalho), titulo, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(cabecalho), subtitulo, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(cabecalho), app->rotulo_resumo, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(corpo), montar_formulario(app), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(corpo), montar_lista(app), TRUE, TRUE, 0);

    gtk_box_pack_start(GTK_BOX(conteudo), cabecalho, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(conteudo), corpo, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(conteudo), app->rotulo_status, FALSE, FALSE, 0);

    gtk_container_add(GTK_CONTAINER(janela), conteudo);
    g_signal_connect(janela, "destroy", G_CALLBACK(destruir_janela), app);

    atualizar_lista_visual(app);
    gtk_widget_show_all(janela);

    if (!carregou) {
        mostrar_dialogo(
            GTK_WINDOW(janela),
            GTK_MESSAGE_WARNING,
            "Aviso ao carregar",
            "Nao foi possivel ler o arquivo de tarefas existente. A interface abriu com a lista atual."
        );
        atualizar_status(app, "Falha ao carregar o arquivo salvo.");
    }
}

int main(int argc, char **argv) {
    GtkApplication *application = gtk_application_new(
        "com.mvosdev.gerenciadortarefas",
        APP_FLAGS
    );
    int status = 0;

    g_signal_connect(application, "activate", G_CALLBACK(ativar), NULL);
    status = g_application_run(G_APPLICATION(application), argc, argv);
    g_object_unref(application);
    return status;
}
