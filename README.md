# Gerenciador de Tarefas com GTK

Aplicativo desktop em C para organizar atividades diarias, projetos e estudos, agora com interface grafica feita em GTK.

## O que o programa faz

- cadastrar tarefas com titulo, categoria, prazo e descricao
- listar todas as tarefas em uma tabela
- filtrar por status ou categoria
- marcar tarefas como concluidas
- remover tarefas selecionadas
- mostrar os detalhes completos da tarefa escolhida
- salvar os dados localmente em `tarefas.dat`

## Estrutura do projeto

- `gerenciador_tarefas.c`: interface grafica em GTK
- `tarefas.c`: regras de negocio e persistencia
- `tarefas.h`: estruturas e funcoes compartilhadas
- `README.md`: documentacao do projeto

## Como compilar

Antes de compilar, o GTK 3 precisa estar instalado e visivel para o `pkg-config`.

Voce pode testar isso com:

```powershell
pkg-config --modversion gtk+-3.0
```

Se esse comando responder com uma versao, no PowerShell o build pode ser feito assim:

```powershell
$gtkFlags = (pkg-config --cflags --libs gtk+-3.0) -split ' '
gcc gerenciador_tarefas.c tarefas.c -Wall -Wextra -pedantic -std=c11 -o gerenciador_tarefas.exe @gtkFlags
```

## Como executar

```powershell
.\gerenciador_tarefas.exe
```

## Persistencia

As tarefas sao salvas automaticamente em `tarefas.dat`. Esse arquivo fica fora do versionamento por causa do `.gitignore`, entao seus dados locais nao vao para o GitHub.

## Observacao sobre este ambiente

Neste ambiente do Codex, o GTK nao estava instalado, entao eu consegui:

- implementar a interface GTK em codigo
- validar a camada de dados compilando `tarefas.c`

O build completo da interface vai funcionar assim que o GTK estiver configurado na sua maquina.

## Melhorias futuras

- editar uma tarefa existente
- ordenar por prazo
- adicionar prioridade
- criar filtros por texto
- separar tarefas por periodo ou disciplina
