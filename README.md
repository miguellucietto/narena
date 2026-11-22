# narena

`narena` é uma biblioteca C pequena de alocação por arena, distribuída em um
único header no estilo STB. Ela agrupa alocações relacionadas e libera a memória
em conjunto. Também oferece `realloc`, `calloc`, cópia e formatação de strings,
reset da arena e um checkpoint reutilizável.

## Escolha do modo de uso

### Header-only (estilo STB)

Defina `NARENA_IMPLEMENTATION` em exatamente um arquivo `.c` antes de incluir o
header. Nos demais arquivos, inclua-o sem essa definição:

```c
/* main.c */
#define NARENA_IMPLEMENTATION
#include "narena.h"

int main(void)
{
    NArena arena = {0};
    int *values = (int *)narena_calloc(&arena, 32, sizeof(*values));
    if (!values) {
        narena_destroy(&arena);
        return 1;
    }
    values[0] = 42;
    narena_destroy(&arena);
    return 0;
}
```

Compile normalmente:

```sh
cc -std=c11 -Wall -Wextra -pedantic main.c -o app
```

### Compilação separada

Como conveniência, `narena.c` define `NARENA_IMPLEMENTATION` e inclui o header.
Nesse modo, nenhum outro arquivo deve definir a macro:

```sh
cc -std=c11 -Wall -Wextra -pedantic main.c narena.c -o app
```

## Garbage collector opcional

`NARENA_GARBAGE_COLLECTOR` cria uma arena global chamada `__narena__`, troca o
`main` do programa por um wrapper que chama o `main` escrito pelo usuário e
destrói a arena quando ele retorna. Também redireciona `malloc`, `calloc`,
`realloc` e `free` para essa arena:

```c
/* gc_app.c: modo de unidade única; estas definições vêm antes do header. */
#define NARENA_IMPLEMENTATION
#define NARENA_GARBAGE_COLLECTOR
#include <stdio.h>
#include "narena.h"

int main(void)
{
    char *message = malloc(64);
    if (!message)
        return 1;
    snprintf(message, 64, "memory is reclaimed when main returns");
    puts(message);
    return 0;
}
```

Compile apenas esse arquivo:

```sh
cc -std=c11 -Wall -Wextra -pedantic gc_app.c -o gc_app
```

No modo coletor, `main()` e `main(int argc, char **argv)` são aceitos; a função
do usuário é renomeada internamente, e o wrapper preserva os argumentos da
linha de comando. Inclua os headers padrão e de terceiros **antes** de
`narena.h`, porque os macros de alocação ficam ativos depois dele. Esse modo é
para um único arquivo de aplicação e não deve ser combinado com `narena.c` nem
com outra definição de `NARENA_IMPLEMENTATION`.

As chamadas `malloc(n)`, `calloc(count, size)` e `realloc(ptr, n)` passam a
usar `__narena__`. `free(ptr)` vira uma operação sem efeito: a memória só é
recuperada quando o `main` retorna. Portanto, não use esses macros para memória
que precise ser liberada individualmente, nem misture ponteiros dessa arena com
`free` da biblioteca C. `realloc(NULL, n)` e tamanhos zero retornam `NULL`, de
acordo com a API `narena_realloc`/`narena_alloc`.

## API

Inclua `narena.h`. Uma arena inicializada com `{0}` pode ser reutilizada após
`narena_reset`; `narena_destroy` libera as regiões e zera a estrutura.

| Função | Comportamento |
| --- | --- |
| `void *narena_alloc(NArena *, size_t)` | Reserva bytes alinhados para `max_align_t`; tamanho zero retorna `NULL`. |
| `void *narena_calloc(NArena *, size_t count, size_t size)` | Reserva `count * size` bytes zerados; retorna `NULL` para zero ou overflow. |
| `void *narena_realloc(NArena *, void *, size_t)` | Redimensiona a última alocação da região quando possível; caso contrário aloca e copia. Falha preserva a alocação original. |
| `char *narena_strdup(NArena *, const char *)` | Copia a string, incluindo o terminador. |
| `char *narena_sprintf(NArena *, const char *fmt, ...)` | Formata uma string dentro da arena. |
| `void narena_save(NArena *)` | Substitui o checkpoint único pelo estado atual. |
| `void narena_restore(NArena *)` | Volta ao checkpoint; as regiões posteriores ficam disponíveis para reutilização. O checkpoint continua válido. |
| `void narena_reset(NArena *)` | Invalida as alocações e o checkpoint, mantendo regiões para reuso. |
| `void narena_destroy(NArena *)` | Libera todas as regiões e zera a arena. |

Toda função aceita `NULL` para o argumento `NArena *` com comportamento seguro
(`NULL` ou nenhuma ação). Os ponteiros obtidos da arena deixam de ser válidos
após `restore`, `reset` ou `destroy`, conforme o estado restaurado. Não use
`free` da libc em ponteiros da arena.

`narena_realloc` aceita ponteiros retornados por esta arena. Se a alocação não
for a última da região, ela aloca outro bloco e copia os bytes preservados; o
bloco antigo continua consumindo espaço até reset, restore aplicável ou
destruição. Tamanho zero retorna `NULL` e não libera o bloco antigo.

`narena_save` mantém somente um checkpoint por arena. Chamadas sucessivas
substituem-no; checkpoints não são empilhados. `restore` não libera regiões do
sistema operacional, apenas permite reutilizá-las.

O tamanho padrão de região é 4096 bytes. Para alterá-lo, defina
`NARENA_DEFAULT_REGION_SIZE` antes do header. Uma alocação maior que esse valor
recebe uma região própria com tamanho suficiente.

## Desenvolvimento

```sh
make tests
```

Os binários dos exemplos são escritos em `build/tests/`. A licença é MIT; veja
[LICENSE](LICENSE).
