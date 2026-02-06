
---

# KAVA 2.5 - Plataforma de Programação Profissional

[![Licença: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Versão: 2.5](https://img.shields.io/badge/Version-2.5-blue.svg)](#)
[![Build: Passing](https://img.shields.io/badge/Build-Passing-brightgreen.svg)](#-compilação-e-uso)

```
██╗  ██╗ █████╗ ██╗   ██╗ █████╗     ██████╗    ██████╗
██║ ██╔╝██╔══██╗██║   ██║██╔══██╗    ╚════██╗  ██╔═████╗
█████╔╝ ███████║██║   ██║███████║     █████╔╝  ██║██╔██║
██╔═██╗ ██╔══██║╚██╗ ██╔╝██╔══██║    ██╔═══╝   ████╔╝██║
██║  ██╗██║  ██║ ╚████╔╝ ██║  ██║    ███████╗  ╚██████╔╝
╚═╝  ╚═╝╚═╝  ╚═╝  ╚═══╝  ╚═╝  ╚═╝    ╚══════╝   ╚═════╝
```

**KAVA 2.5** é uma linguagem de programação moderna, de alta performance e com sintaxe familiar, inspirada em Java. Ela foi projetada para ser leve, rápida e educacional, ao mesmo tempo que introduz funcionalidades poderosas para o desenvolvimento profissional, como programação assíncrona, streams e um ecossistema de pacotes.

## 🎯 Objetivos do Projeto

1.  **Performance Superior:** Superar tecnicamente o Java 6 e competir com o Java 8 em benchmarks de performance, focando em execução rápida, baixo uso de memória e inicialização instantânea da VM.
2.  **Sintaxe Moderna:** Introduzir paradigmas de programação funcional e assíncrona (`lambdas`, `streams`, `async/await`) para simplificar o desenvolvimento de aplicações complexas.
3.  **Código Educacional:** Manter uma base de código clara e bem documentada, servindo como uma ferramenta de aprendizado para a construção de compiladores, máquinas virtuais e sistemas de concorrência.
4.  **Ecossistema Completo:** Fornecer um conjunto de ferramentas integradas, incluindo um compilador (`kavac`), uma máquina virtual (`kavavm`), um gerenciador de pacotes (`kpm`) e um sistema de build (`Makefile`).

## ✨ Principais Funcionalidades

| Categoria | Funcionalidades Chave | Status |
| :--- | :--- | :---: |
| **Novidades (KAVA 2.5)** | **Lambdas, Streams, Async/Await, Pipe Operator (`|>`)** | ✅ |
| **Runtime / VM** | Bytecode próprio, VM baseada em pilha, JIT (experimental) | ✅ |
| **Gerenciamento de Memória** | Garbage Collector (Mark-Sweep + Generational) | ✅ |
| **Sistema de Tipos** | Tipagem estática, Generics, Classes, Interfaces, Enums | ✅ |
| **Orientação a Objetos** | Herança, Polimorfismo, Encapsulamento, Classes Abstratas | ✅ |
| **Concorrência** | `Thread`, `Runnable`, `synchronized`, `ReentrantLock`, `ThreadPoolExecutor` | ✅ |
| **Framework de Coleções** | `ArrayList`, `LinkedList`, `HashMap`, `HashSet`, `Queue`, `Stack` | ✅ |
| **Ferramentas** | Compilador, VM, Gerenciador de Pacotes (KPM), Benchmarks | ✅ |

## 🚀 Começando

Para compilar e executar o KAVA 2.5 em seu sistema, siga os passos abaixo.

### Requisitos

-   `g++` com suporte a C++17
-   `make`
-   `pkg-config`
-   `SDL2`, `SDL2_image`, `SDL2_ttf` (opcional, para os exemplos gráficos)

### Compilação e Uso

O `Makefile` fornecido automatiza todo o processo de build.

1.  **Clone o repositório:**
    ```bash
    git clone <URL_DO_SEU_REPOSITORIO>
    cd kava2
    ```

2.  **Compile o projeto:**
    ```bash
    # Compila tudo (compilador, VM, benchmarks, kpm)
    make all
    ```

3.  **Execute um exemplo:**
    ```bash
    # Compile o arquivo de teste do KAVA 2.5
    ./kavac examples/test_2_5.kava

    # Execute o bytecode gerado
    ./kavavm examples/test_2_5.kvb
    ```

### Comandos do `Makefile`

-   `make all`: Compila todas as ferramentas.
-   `make kavac`: Compila apenas o compilador.
-   `make kavavm`: Compila apenas a Máquina Virtual.
-   `make test`: Compila e executa os testes principais.
-   `make bench`: Compila e executa os benchmarks de performance.
-   `make clean`: Limpa os artefatos de build.
-   `sudo make install`: Instala os binários `kavac`, `kavavm` e `kpm` em `/usr/local/bin`.

## 📦 Gerenciador de Pacotes (KPM)

KAVA 2.5 inclui o **KPM**, um gerenciador de pacotes para criar e gerenciar projetos.

-   **Inicializar um novo projeto:**
    ```bash
    ./kpm init meu-app
    ```
-   **Construir o projeto (conforme definido em `kava.pkg`):**
    ```bash
    ./kpm build
    ```
-   **Executar testes:**
    ```bash
    ./kpm test
    ```

## 📁 Estrutura do Projeto

```
kava2/
├── compiler/       # Compilador (Lexer, Parser, Codegen, AST)
├── vm/             # Máquina Virtual (VM, Bytecode, JIT, Async)
├── gc/             # Garbage Collector
├── collections/    # Framework de Coleções
├── threads/        # Sistema de Concorrência
├── benchmark/      # Sistema de Benchmarks
├── stdlib/         # Biblioteca Padrão (http, json, fs, etc.)
├── examples/       # Códigos de exemplo
├── kpm/            # Gerenciador de Pacotes KPM
├── Makefile        # Sistema de build
├── LICENSE         # Licença MIT
└── README.md       # Este arquivo
```

## 📚 Documentação

Para aprender a programar em KAVA, consulte nossos guias detalhados:

-   **[Guia de Programação KAVA 2.5 (PT-BR)](./docs/PROGRAMMING_GUIDE.pt-BR.md)**: Um guia completo da sintaxe e dos recursos da linguagem.
-   **[KAVA 2.5 Programming Guide (EN)](./docs/PROGRAMMING_GUIDE.en-US.md)**: The complete guide to the language syntax and features.

## 🤝 Como Contribuir

Contribuições são muito bem-vindas! Se você deseja melhorar o KAVA, siga estes passos:

1.  **Faça um Fork** do repositório.
2.  **Crie uma Branch** para sua feature (`git checkout -b feature/nova-feature`).
3.  **Faça o Commit** de suas mudanças (`git commit -m 'Adiciona nova feature'`).
4.  **Faça o Push** para a branch (`git push origin feature/nova-feature`).
5.  **Abra um Pull Request**.

Por favor, certifique-se de que seus commits seguem as convenções do projeto e que todos os testes estão passando.

## 📜 Licença

Este projeto é distribuído sob a **Licença MIT**. Veja o arquivo [LICENSE](./LICENSE) para mais detalhes.

Copyright (c) 2026 KAVA Team.
