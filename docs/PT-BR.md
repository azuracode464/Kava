x
---

## Guia de Programação KAVA 2.5

Bem-vindo ao guia oficial da linguagem de programação KAVA 2.5. Este documento é um recurso completo para aprender a sintaxe, os conceitos e os recursos avançados da linguagem.

### Índice

1.  [Visão Geral](#1-visão-geral)
2.  [Sintaxe Básica](#2-sintaxe-básica)
    -   [Variáveis e Tipos](#variáveis-e-tipos)
    -   [Operadores](#operadores)
    -   [Controle de Fluxo](#controle-de-fluxo)
3.  [Orientação a Objetos](#3-orientação-a-objetos)
    -   [Classes e Objetos](#classes-e-objetos)
    -   [Herança](#herança)
    -   [Interfaces e Classes Abstratas](#interfaces-e-classes-abstratas)
4.  [Recursos Avançados (KAVA 2.5)](#4-recursos-avançados-kava-25)
    -   [Expressões Lambda](#expressões-lambda)
    -   [API de Streams](#api-de-streams)
    -   [Programação Assíncrona com `async/await`](#programação-assíncrona-com-asyncawait)
    -   [Pipe Operator (`|>)`](#pipe-operator-)
5.  [Biblioteca Padrão](#5-biblioteca-padrão)

---

### 1. Visão Geral

KAVA é uma linguagem de programação de tipagem estática, orientada a objetos e com fortes influências de Java. A versão 2.5 introduz conceitos modernos de programação funcional e assíncrona, tornando-a poderosa tanto para aprendizado quanto para desenvolvimento profissional.

Um programa "Olá, Mundo" em KAVA é simples e direto:

```java
// O ponto de entrada padrão pode ser uma função main()
func main() {
    print "Olá, Mundo KAVA 2.5!"
}
```

### 2. Sintaxe Básica

#### Variáveis e Tipos

KAVA possui um sistema de tipos similar ao Java, com tipos primitivos e tipos de referência (objetos).

-   **Tipos Primitivos:** `boolean`, `byte`, `char`, `short`, `int`, `long`, `float`, `double`.
-   **Declaração de Variáveis:** Use a palavra-chave `let` para inferência de tipo (quando inicializada) ou declare o tipo explicitamente.

```java
// Declaração explícita
int a = 10;
String message = "KAVA";
boolean flag = true;

// Usando 'let' para inferência de tipo
let b = 20; // Ingerido como int
let name = "Kava"; // Inferido como String
let pi = 3.14; // Inferido como double
```

Arrays são declarados com `[]`:

```java
int[] numbers = {1, 2, 3, 4, 5};
String[] names = new String[5];
names[0] = "Kava";
```

#### Operadores

KAVA suporta os operadores aritméticos, relacionais, lógicos e bitwise padrão.

```java
let x = 10;
let y = 4;

// Aritméticos
print x + y; // 14
print x * y; // 40
print x % y; // 2

// Relacionais
print x > y;  // true
print x == 10; // true

// Lógicos
let isReady = true;
let hasPermission = false;
print isReady && hasPermission; // false
print isReady || hasPermission; // true
```

#### Controle de Fluxo

**`if-else`**

```java
let score = 85;
if (score >= 90) {
    print "Excelente!";
} else if (score >= 70) {
    print "Bom.";
} else {
    print "Precisa melhorar.";
}
```

**`while` e `do-while`**

```java
let i = 0;
while (i < 5) {
    print i;
    i = i + 1;
}
```

**`for` (tradicional e for-each)**

```java
// Loop for tradicional
for (int j = 0; j < 5; j++) {
    print "j = " + j;
}

// Loop for-each para coleções e arrays
int[] numbers = {10, 20, 30};
for (int num : numbers) {
    print "Número: " + num;
}
```

**`switch`**

```java
int day = 3;
switch (day) {
    case 1: print "Segunda"; break;
    case 2: print "Terça"; break;
    case 3: print "Quarta"; break;
    default: print "Outro dia";
}
```

### 3. Orientação a Objetos

#### Classes e Objetos

A declaração de classes é o pilar da programação em KAVA.

```java
public class Greeter {
    // Campo (propriedade)
    private String greeting;

    // Construtor
    public Greeter(String greeting) {
        this.greeting = greeting;
    }

    // Método
    public void sayHello(String name) {
        print this.greeting + ", " + name + "!";
    }
}

// Criando e usando um objeto (instância)
let g = new Greeter("Bem-vindo");
g.sayHello("usuário"); // Saída: Bem-vindo, usuário!
```

#### Herança

Use `extends` para herdar de uma classe e `super` para acessar membros da classe pai.

```java
public class Animal {
    protected String name;

    public Animal(String name) { this.name = name; }
    public void speak() { print "..."; }
}

public class Dog extends Animal {
    public Dog(String name) {
        super(name); // Chama o construtor da classe pai
    }

    @Override // Anotação para indicar sobrescrita de método
    public void speak() {
        print "Woof!";
    }
}

Dog myDog = new Dog("Rex");
myDog.speak(); // Saída: Woof!
```

#### Interfaces e Classes Abstratas

-   **Interfaces** definem um contrato que as classes devem seguir.
-   **Classes Abstratas** podem conter tanto métodos abstratos (sem implementação) quanto métodos concretos.

```java
// Interface
public interface Comparable<T> {
    int compareTo(T other);
}

// Classe abstrata
public abstract class Shape {
    // Método concreto
    public String getColor() {
        return "blue";
    }
    // Método abstrato
    public abstract double getArea();
}

public class Circle extends Shape implements Comparable<Circle> {
    private double radius;

    @Override
    public double getArea() {
        return 3.14159 * radius * radius;
    }

    @Override
    public int compareTo(Circle other) {
        return this.radius - other.radius;
    }
}
```

### 4. Recursos Avançados (KAVA 2.5)

#### Expressões Lambda

Lambdas fornecem uma sintaxe concisa para criar instâncias de interfaces funcionais (interfaces com um único método abstrato).

```java
// Sintaxe: (parâmetros) -> { corpo } ou (parâmetros) -> expressão

// Exemplo com a interface Runnable
Runnable myTask = () -> print "Executando em uma thread!";
new Thread(myTask).start();

// Exemplo com parâmetros
Function<int, int> square = (x) -> x * x;
print square.apply(5); // 25
```

#### API de Streams

A API de Streams permite processar coleções de dados de forma declarativa e funcional.

```java
int[] numbers = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

// Encadeando operações de stream
let sumOfSquaresOfEvens = numbers.stream()
    .filter(n -> n % 2 == 0)   // Filtra apenas os pares
    .map(n -> n * n)           // Eleva ao quadrado
    .sum();                    // Soma os resultados

print sumOfSquaresOfEvens; // 220 (4 + 16 + 36 + 64 + 100)
```

#### Programação Assíncrona com `async/await`

Simplifique o código assíncrono, evitando o "callback hell".

-   `async`: Marca um método como assíncrono. Ele retorna uma `Promise`.
-   `await`: Pausa a execução de um método `async` até que uma `Promise` seja resolvida.

```java
// Simula uma busca de dados na rede
async String fetchData() {
    // Em uma aplicação real, aqui haveria uma chamada de rede
    Thread.sleep(1000); // Simula latência
    return "Dados recebidos!";
}

async func main() {
    print "Buscando dados...";
    String result = await fetchData(); // Pausa aqui, não bloqueia a thread principal
    print result; // "Dados recebidos!"
    print "Processo finalizado.";
}

main();
```

#### Pipe Operator (`|>`)

O operador `|>` passa o resultado da expressão à esquerda como o primeiro argumento da função à direita, melhorando a legibilidade de chamadas aninhadas.

```java
func addOne(n) { return n + 1; }
func doubleIt(n) { return n * 2; }

// Sem o pipe operator
let result1 = doubleIt(addOne(5)); // 12

// Com o pipe operator
let result2 = 5 |> addOne |> doubleIt; // 12

print result2;
```

### 5. Biblioteca Padrão

KAVA 2.5 vem com uma biblioteca padrão rica, organizada em pacotes. Para usá-los, basta importar com `import <pacote>`.

-   `io`: Funções de entrada e saída, como `print`.
-   `math`: Funções matemáticas (`abs`, `max`, `sqrt`).
-   `fs`: Manipulação de sistema de arquivos.
-   `http`: Cliente e servidor HTTP.
-   `json`: Parser e serializador de JSON.
-   `gfx`: Funções para criação de interfaces gráficas (via SDL2).

Este guia cobre os principais aspectos da programação em KAVA 2.5. Para mais detalhes, consulte os arquivos de exemplo no diretório `/examples` do projeto.
