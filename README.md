# KAVA 2.0 - Plataforma de Programação Profissional

```
██╗  ██╗ █████╗ ██╗   ██╗ █████╗     ██████╗    ██████╗ 
██║ ██╔╝██╔══██╗██║   ██║██╔══██╗    ╚════██╗  ██╔═████╗
█████╔╝ ███████║██║   ██║███████║     █████╔╝  ██║██╔██║
██╔═██╗ ██╔══██║╚██╗ ██╔╝██╔══██║    ██╔═══╝   ████╔╝██║
██║  ██╗██║  ██║ ╚████╔╝ ██║  ██║    ███████╗  ╚██████╔╝
╚═╝  ╚═╝╚═╝  ╚═╝  ╚═══╝  ╚═╝  ╚═╝    ╚══════╝   ╚═════╝ 
```

**KAVA 2.0** é uma linguagem de programação completa, educacional e profissional, inspirada no Java 6, projetada para ser leve, rápida e capaz de competir tecnicamente com Java 5/6.

## 🎯 Objetivo

Criar uma linguagem equivalente ao Java 6 em capacidades, mas otimizada para:
- ✅ Execução mais rápida (meta: 1%+ mais rápido que Java 5/6)
- ✅ Menor uso de memória
- ✅ Tempo de inicialização reduzido da VM
- ✅ Código educacional e compreensível

## 📋 Features Implementadas (Equivalentes ao Java 6)

### 🔷 Sistema de Tipos
| Feature | Status | Descrição |
|---------|--------|-----------|
| Tipagem Estática | ✅ | Verificação de tipos em tempo de compilação |
| Tipos Primitivos | ✅ | boolean, byte, char, short, int, long, float, double |
| Classes | ✅ | Declaração completa de classes |
| Interfaces | ✅ | Com métodos abstratos e constantes |
| Classes Abstratas | ✅ | Suporte a abstract class |
| Enums | ✅ | Com constantes e métodos |
| Generics | ✅ | <T>, bounds, wildcards |
| Arrays | ✅ | Multidimensionais, tipados |

### 🔷 Orientação a Objetos
| Feature | Status | Descrição |
|---------|--------|-----------|
| Herança (extends) | ✅ | Herança simples de classes |
| Polimorfismo | ✅ | Override de métodos |
| Interfaces (implements) | ✅ | Implementação múltipla |
| Construtores | ✅ | Com sobrecarga |
| this/super | ✅ | Referências de instância |
| static members | ✅ | Campos e métodos estáticos |
| Inner Classes | ✅ | Classes aninhadas |

### 🔷 Modificadores de Acesso
| Modificador | Status | Descrição |
|-------------|--------|-----------|
| public | ✅ | Visível para todos |
| protected | ✅ | Visível para subclasses |
| private | ✅ | Visível apenas na classe |
| package-private | ✅ | Default (sem modificador) |
| final | ✅ | Não pode ser sobrescrito |
| static | ✅ | Pertence à classe |
| abstract | ✅ | Deve ser implementado |
| synchronized | ✅ | Thread-safe |
| volatile | ✅ | Visibilidade entre threads |
| transient | ✅ | Não serializado |
| native | ✅ | Implementado nativamente |

### 🔷 Controle de Fluxo
| Feature | Status | Descrição |
|---------|--------|-----------|
| if/else | ✅ | Condicional |
| switch/case | ✅ | Com fall-through e default |
| while | ✅ | Loop com condição |
| do-while | ✅ | Loop com condição no final |
| for | ✅ | Loop tradicional |
| for-each | ✅ | Iteração sobre coleções |
| break | ✅ | Com suporte a labels |
| continue | ✅ | Com suporte a labels |
| return | ✅ | Retorno de métodos |

### 🔷 Exceções
| Feature | Status | Descrição |
|---------|--------|-----------|
| try/catch | ✅ | Tratamento de exceções |
| finally | ✅ | Bloco de limpeza |
| throw | ✅ | Lançamento de exceções |
| throws | ✅ | Declaração de exceções |
| Multi-catch | ✅ | Múltiplos tipos em catch |
| Checked Exceptions | ✅ | Verificadas em compilação |
| Unchecked Exceptions | ✅ | RuntimeException |

### 🔷 Anotações
| Feature | Status | Descrição |
|---------|--------|-----------|
| @Annotation | ✅ | Sintaxe de anotações |
| Elementos | ✅ | Valores em anotações |
| Retention | ✅ | RUNTIME, CLASS, SOURCE |

### 🔷 Coleções (Framework Completo)
| Coleção | Status | Descrição |
|---------|--------|-----------|
| ArrayList | ✅ | Lista baseada em array |
| LinkedList | ✅ | Lista duplamente encadeada |
| HashMap | ✅ | Mapa hash com encadeamento |
| HashSet | ✅ | Set baseado em HashMap |
| Stack | ✅ | Pilha LIFO |
| Queue | ✅ | Fila FIFO |
| PriorityQueue | ✅ | Fila com prioridade (heap) |

### 🔷 Threading & Concorrência
| Feature | Status | Descrição |
|---------|--------|-----------|
| Thread | ✅ | Threads gerenciadas |
| Runnable | ✅ | Interface funcional |
| synchronized | ✅ | Blocos sincronizados |
| ReentrantLock | ✅ | Lock reentrante |
| Semaphore | ✅ | Semáforo contável |
| CountDownLatch | ✅ | Barreira de contagem |
| CyclicBarrier | ✅ | Barreira cíclica |
| BlockingQueue | ✅ | Fila bloqueante |
| ThreadPoolExecutor | ✅ | Pool de threads |
| AtomicInteger | ✅ | Inteiros atômicos |
| ReadWriteLock | ✅ | Lock de leitura/escrita |

### 🔷 Runtime / VM
| Feature | Status | Descrição |
|---------|--------|-----------|
| Bytecode próprio | ✅ | 200+ opcodes |
| Stack-based VM | ✅ | Como JVM |
| Heap gerenciado | ✅ | Alocação automática |
| Garbage Collector | ✅ | Mark-Sweep + Generational |
| Class Loader | ✅ | Carregamento dinâmico |
| Native Methods | ✅ | Integração com C++ |
| JIT | 🔄 | Em desenvolvimento |

## 📁 Estrutura do Projeto

```
kava2/
├── compiler/           # Compilador (Lexer, Parser, Codegen)
│   ├── lexer.h/cpp    # Analisador léxico
│   ├── parser.h/cpp   # Analisador sintático
│   ├── ast.h          # Abstract Syntax Tree
│   ├── types.h        # Sistema de tipos
│   ├── codegen.h/cpp  # Gerador de bytecode
│   └── main.cpp       # Entry point do compilador
├── vm/                # Máquina Virtual
│   ├── bytecode.h     # Especificação de bytecode
│   ├── vm.h           # VM completa
│   └── heap.h         # Gerenciamento de memória (legacy)
├── gc/                # Garbage Collector
│   └── gc.h           # Mark-Sweep + Generational
├── collections/       # Framework de Coleções
│   └── collections.h  # List, Map, Set, Queue, Stack
├── threads/          # Threading & Concorrência
│   └── threads.h      # Thread, Lock, Semaphore, Pool
├── benchmark/        # Sistema de Benchmarks
│   ├── benchmark.h   # Runner e Reporter
│   └── main.cpp      # Execução de benchmarks
├── stdlib/           # Biblioteca Padrão (.kava)
│   ├── io.kava       # I/O
│   ├── math.kava     # Matemática
│   ├── time.kava     # Tempo
│   └── fs.kava       # Sistema de arquivos
├── examples/         # Exemplos
├── tests/            # Testes
├── Makefile          # Build system
├── LICENSE           # MIT License
└── README.md         # Este arquivo
```

## 🛠️ Compilação

### Requisitos
- g++ com suporte a C++17
- make
- SDL2 (opcional, para gráficos)

### Build
```bash
# Compila tudo
make

# Compila apenas o compilador
make kavac

# Compila apenas a VM
make kavavm

# Compila benchmarks
make benchmark
```

## 🚀 Uso

### Compilar código KAVA
```bash
./kavac arquivo.kava
```
Gera `arquivo.kvb` (bytecode KAVA)

### Executar bytecode
```bash
./kavavm arquivo.kvb
```

### Executar benchmarks
```bash
make bench
# ou
./kavabench
```

## 📊 Benchmarks vs Java 5/6

O sistema de benchmarks compara KAVA 2.0 com Java 5 e Java 6 em:

| Benchmark | Descrição |
|-----------|-----------|
| Arithmetic Loop | Loop matemático intensivo |
| Object Creation | Criação massiva de objetos |
| Array Access | Leitura/escrita em arrays |
| Function Calls | Chamadas de função recursivas |
| Fibonacci | Recursão (fibonacci) |
| HashMap Ops | Operações em HashMap |
| String Ops | Manipulação de strings |
| Sorting | Ordenação de arrays |
| Thread Test | Operações com threads |
| Memory Pressure | Pressão de memória (GC) |

**Meta**: Ser pelo menos 1% mais rápido que Java 5/6 em algum benchmark.

## 📜 Especificação da Linguagem

### Tipos Primitivos
```java
boolean flag = true;
byte b = 127;
char c = 'A';
short s = 32767;
int i = 2147483647;
long l = 9223372036854775807L;
float f = 3.14f;
double d = 3.14159265358979;
```

### Classes e Herança
```java
public class Animal {
    protected String name;
    
    public Animal(String name) {
        this.name = name;
    }
    
    public void speak() {
        print "..."
    }
}

public class Dog extends Animal {
    public Dog(String name) {
        super(name);
    }
    
    @Override
    public void speak() {
        print "Woof!"
    }
}
```

### Interfaces
```java
public interface Comparable<T> {
    int compareTo(T other);
}

public class Person implements Comparable<Person> {
    private int age;
    
    public int compareTo(Person other) {
        return this.age - other.age;
    }
}
```

### Generics
```java
public class Box<T> {
    private T value;
    
    public void set(T value) {
        this.value = value;
    }
    
    public T get() {
        return value;
    }
}

Box<String> stringBox = new Box<String>();
stringBox.set("Hello");
```

### Exceções
```java
try {
    int result = riskyOperation();
} catch (IOException | SQLException e) {
    handleError(e);
} finally {
    cleanup();
}
```

### Threading
```java
Thread t = new Thread(() -> {
    for (int i = 0; i < 100; i++) {
        print i
    }
});
t.start();
t.join();
```

## 📄 Licença

MIT License - Copyright (c) 2026 KAVA Team

## 🙏 Créditos

Inspirado por:
- Java SE 6 (Sun Microsystems/Oracle)
- JVM HotSpot
- LLVM
- V8 JavaScript Engine

---

**KAVA 2.0** - Lightweight, Fast, Educational
