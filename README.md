# Gerenciador de Tarefas em C

Um gerenciador de tarefas feito em C para organizar atividades do dia a dia, projetos e estudos da faculdade.

## Funcionalidades

- Adicionar tarefas
- Listar todas as tarefas
- Listar apenas tarefas pendentes
- Filtrar tarefas por categoria
- Marcar tarefas como concluidas
- Remover tarefas
- Salvar os dados localmente em arquivo

## Categorias disponiveis

- Diaria
- Projeto
- Estudo

## Como compilar

No Windows, usando `gcc`:

```bash
gcc gerenciador_tarefas.c -Wall -Wextra -pedantic -std=c11 -o gerenciador_tarefas.exe
```

## Como executar

```bash
.\gerenciador_tarefas.exe
```

## Persistencia de dados

As tarefas sao salvas no arquivo `tarefas.dat`, criado automaticamente ao usar o programa. Esse arquivo fica fora do versionamento por causa do `.gitignore`.

## Estrutura do projeto

- `gerenciador_tarefas.c`: codigo-fonte principal
- `README.md`: documentacao do projeto
- `.gitignore`: arquivos locais ignorados pelo Git

## Objetivo do projeto

Este projeto foi criado para ajudar na organizacao de:

- atividades diarias
- projetos pessoais
- estudos para provas e trabalhos da faculdade

## Melhorias futuras

- adicionar prioridade nas tarefas
- ordenar por prazo
- editar tarefas existentes
- buscar tarefas por titulo
- melhorar a interface no terminal
