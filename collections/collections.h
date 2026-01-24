/*
 * MIT License
 * Copyright (c) 2026 KAVA Team
 * 
 * KAVA 2.0 - Collections Framework
 * Coleções genéricas inspiradas no Java 6 Collections API
 */

#ifndef KAVA_COLLECTIONS_H
#define KAVA_COLLECTIONS_H

#include <cstdint>
#include <cstring>
#include <functional>
#include "../gc/gc.h"

namespace Kava {

// Forward declarations
template<typename T> class ArrayList;
template<typename T> class LinkedList;
template<typename K, typename V> class HashMap;
template<typename T> class HashSet;
template<typename T> class Iterator;

// ============================================================
// INTERFACE ITERABLE (equivalente Java)
// ============================================================
template<typename T>
class Iterable {
public:
    virtual ~Iterable() = default;
    virtual Iterator<T> iterator() = 0;
};

// ============================================================
// INTERFACE COLLECTION (equivalente Java)
// ============================================================
template<typename T>
class Collection : public Iterable<T> {
public:
    virtual ~Collection() = default;
    
    // Operações básicas
    virtual int size() const = 0;
    virtual bool isEmpty() const { return size() == 0; }
    virtual bool contains(const T& element) const = 0;
    virtual bool add(const T& element) = 0;
    virtual bool remove(const T& element) = 0;
    virtual void clear() = 0;
    
    // Operações de bulk
    virtual bool addAll(const Collection<T>& c) {
        bool modified = false;
        auto it = const_cast<Collection<T>&>(c).iterator();
        while (it.hasNext()) {
            if (add(it.next())) modified = true;
        }
        return modified;
    }
    
    virtual bool removeAll(const Collection<T>& c) {
        bool modified = false;
        auto it = const_cast<Collection<T>&>(c).iterator();
        while (it.hasNext()) {
            if (remove(it.next())) modified = true;
        }
        return modified;
    }
    
    // Conversão para array
    virtual void toArray(T* array) const = 0;
};

// ============================================================
// INTERFACE LIST (equivalente Java)
// ============================================================
template<typename T>
class List : public Collection<T> {
public:
    // Acesso por índice
    virtual T& get(int index) = 0;
    virtual const T& get(int index) const = 0;
    virtual T set(int index, const T& element) = 0;
    virtual void add(int index, const T& element) = 0;
    virtual T removeAt(int index) = 0;
    
    // Busca
    virtual int indexOf(const T& element) const = 0;
    virtual int lastIndexOf(const T& element) const = 0;
    
    // Sublista
    virtual List<T>* subList(int fromIndex, int toIndex) = 0;
    
    // Operador de acesso
    T& operator[](int index) { return get(index); }
    const T& operator[](int index) const { return get(index); }
};

// ============================================================
// INTERFACE SET (equivalente Java)
// ============================================================
template<typename T>
class Set : public Collection<T> {
    // Set não permite duplicatas
    // Herda toda a interface de Collection
};

// ============================================================
// INTERFACE MAP (equivalente Java)
// ============================================================
template<typename K, typename V>
class Map {
public:
    virtual ~Map() = default;
    
    // Operações básicas
    virtual int size() const = 0;
    virtual bool isEmpty() const { return size() == 0; }
    virtual bool containsKey(const K& key) const = 0;
    virtual bool containsValue(const V& value) const = 0;
    virtual V* get(const K& key) = 0;
    virtual V put(const K& key, const V& value) = 0;
    virtual V remove(const K& key) = 0;
    virtual void clear() = 0;
    
    // Views
    virtual Set<K>* keySet() = 0;
    virtual Collection<V>* values() = 0;
    
    // Operador de acesso
    V& operator[](const K& key) {
        V* val = get(key);
        if (!val) {
            put(key, V());
            val = get(key);
        }
        return *val;
    }
};

// ============================================================
// ITERATOR (equivalente Java)
// ============================================================
template<typename T>
class Iterator {
public:
    virtual ~Iterator() = default;
    virtual bool hasNext() const = 0;
    virtual T& next() = 0;
    virtual void remove() = 0;
};

// ============================================================
// ARRAY ITERATOR
// ============================================================
template<typename T>
class ArrayIterator : public Iterator<T> {
private:
    T* array;
    int size;
    int current;
    
public:
    ArrayIterator(T* arr, int sz) : array(arr), size(sz), current(0) {}
    
    bool hasNext() const override { return current < size; }
    
    T& next() override {
        return array[current++];
    }
    
    void remove() override {
        // Não suportado para array simples
    }
};

// ============================================================
// ARRAYLIST - Lista baseada em array dinâmico
// ============================================================
template<typename T>
class ArrayList : public List<T> {
private:
    T* elements;
    int count;
    int capacity;
    
    void ensureCapacity(int minCapacity) {
        if (minCapacity > capacity) {
            int newCapacity = capacity * 2;
            if (newCapacity < minCapacity) newCapacity = minCapacity;
            
            T* newElements = new T[newCapacity];
            for (int i = 0; i < count; i++) {
                newElements[i] = std::move(elements[i]);
            }
            delete[] elements;
            elements = newElements;
            capacity = newCapacity;
        }
    }
    
    void rangeCheck(int index) const {
        if (index < 0 || index >= count) {
            // throw IndexOutOfBoundsException
        }
    }
    
public:
    explicit ArrayList(int initialCapacity = 16)
        : count(0), capacity(initialCapacity) {
        elements = new T[capacity];
    }
    
    ~ArrayList() {
        delete[] elements;
    }
    
    // Copy constructor
    ArrayList(const ArrayList& other)
        : count(other.count), capacity(other.capacity) {
        elements = new T[capacity];
        for (int i = 0; i < count; i++) {
            elements[i] = other.elements[i];
        }
    }
    
    // Move constructor
    ArrayList(ArrayList&& other) noexcept
        : elements(other.elements), count(other.count), capacity(other.capacity) {
        other.elements = nullptr;
        other.count = 0;
        other.capacity = 0;
    }
    
    // Collection interface
    int size() const override { return count; }
    
    bool contains(const T& element) const override {
        return indexOf(element) >= 0;
    }
    
    bool add(const T& element) override {
        ensureCapacity(count + 1);
        elements[count++] = element;
        return true;
    }
    
    bool remove(const T& element) override {
        int idx = indexOf(element);
        if (idx >= 0) {
            removeAt(idx);
            return true;
        }
        return false;
    }
    
    void clear() override {
        count = 0;
    }
    
    void toArray(T* array) const override {
        for (int i = 0; i < count; i++) {
            array[i] = elements[i];
        }
    }
    
    Iterator<T> iterator() override {
        return ArrayIterator<T>(elements, count);
    }
    
    // List interface
    T& get(int index) override {
        rangeCheck(index);
        return elements[index];
    }
    
    const T& get(int index) const override {
        rangeCheck(index);
        return elements[index];
    }
    
    T set(int index, const T& element) override {
        rangeCheck(index);
        T oldValue = elements[index];
        elements[index] = element;
        return oldValue;
    }
    
    void add(int index, const T& element) override {
        if (index < 0 || index > count) return;
        
        ensureCapacity(count + 1);
        
        // Desloca elementos
        for (int i = count; i > index; i--) {
            elements[i] = std::move(elements[i - 1]);
        }
        
        elements[index] = element;
        count++;
    }
    
    T removeAt(int index) override {
        rangeCheck(index);
        
        T oldValue = elements[index];
        
        // Desloca elementos
        for (int i = index; i < count - 1; i++) {
            elements[i] = std::move(elements[i + 1]);
        }
        
        count--;
        return oldValue;
    }
    
    int indexOf(const T& element) const override {
        for (int i = 0; i < count; i++) {
            if (elements[i] == element) return i;
        }
        return -1;
    }
    
    int lastIndexOf(const T& element) const override {
        for (int i = count - 1; i >= 0; i--) {
            if (elements[i] == element) return i;
        }
        return -1;
    }
    
    List<T>* subList(int fromIndex, int toIndex) override {
        auto* sub = new ArrayList<T>(toIndex - fromIndex);
        for (int i = fromIndex; i < toIndex; i++) {
            sub->add(elements[i]);
        }
        return sub;
    }
    
    // Métodos adicionais
    void trimToSize() {
        if (count < capacity) {
            T* newElements = new T[count];
            for (int i = 0; i < count; i++) {
                newElements[i] = std::move(elements[i]);
            }
            delete[] elements;
            elements = newElements;
            capacity = count;
        }
    }
    
    int getCapacity() const { return capacity; }
};

// ============================================================
// LINKEDLIST NODE
// ============================================================
template<typename T>
struct LinkedNode {
    T value;
    LinkedNode* prev;
    LinkedNode* next;
    
    LinkedNode(const T& val) : value(val), prev(nullptr), next(nullptr) {}
};

// ============================================================
// LINKEDLIST - Lista duplamente encadeada
// ============================================================
template<typename T>
class LinkedList : public List<T> {
private:
    LinkedNode<T>* head;
    LinkedNode<T>* tail;
    int count;
    
    LinkedNode<T>* nodeAt(int index) const {
        if (index < count / 2) {
            LinkedNode<T>* node = head;
            for (int i = 0; i < index; i++) {
                node = node->next;
            }
            return node;
        } else {
            LinkedNode<T>* node = tail;
            for (int i = count - 1; i > index; i--) {
                node = node->prev;
            }
            return node;
        }
    }
    
public:
    LinkedList() : head(nullptr), tail(nullptr), count(0) {}
    
    ~LinkedList() {
        clear();
    }
    
    // Collection interface
    int size() const override { return count; }
    
    bool contains(const T& element) const override {
        LinkedNode<T>* node = head;
        while (node) {
            if (node->value == element) return true;
            node = node->next;
        }
        return false;
    }
    
    bool add(const T& element) override {
        addLast(element);
        return true;
    }
    
    bool remove(const T& element) override {
        LinkedNode<T>* node = head;
        while (node) {
            if (node->value == element) {
                unlink(node);
                return true;
            }
            node = node->next;
        }
        return false;
    }
    
    void clear() override {
        LinkedNode<T>* node = head;
        while (node) {
            LinkedNode<T>* next = node->next;
            delete node;
            node = next;
        }
        head = tail = nullptr;
        count = 0;
    }
    
    void toArray(T* array) const override {
        LinkedNode<T>* node = head;
        int i = 0;
        while (node) {
            array[i++] = node->value;
            node = node->next;
        }
    }
    
    Iterator<T> iterator() override {
        // TODO: Implementar LinkedListIterator
        return ArrayIterator<T>(nullptr, 0);
    }
    
    // List interface
    T& get(int index) override {
        return nodeAt(index)->value;
    }
    
    const T& get(int index) const override {
        return nodeAt(index)->value;
    }
    
    T set(int index, const T& element) override {
        LinkedNode<T>* node = nodeAt(index);
        T oldValue = node->value;
        node->value = element;
        return oldValue;
    }
    
    void add(int index, const T& element) override {
        if (index == count) {
            addLast(element);
        } else {
            LinkedNode<T>* succ = nodeAt(index);
            LinkedNode<T>* pred = succ->prev;
            LinkedNode<T>* newNode = new LinkedNode<T>(element);
            
            newNode->next = succ;
            newNode->prev = pred;
            succ->prev = newNode;
            
            if (pred == nullptr) {
                head = newNode;
            } else {
                pred->next = newNode;
            }
            
            count++;
        }
    }
    
    T removeAt(int index) override {
        LinkedNode<T>* node = nodeAt(index);
        T oldValue = node->value;
        unlink(node);
        return oldValue;
    }
    
    int indexOf(const T& element) const override {
        int index = 0;
        LinkedNode<T>* node = head;
        while (node) {
            if (node->value == element) return index;
            node = node->next;
            index++;
        }
        return -1;
    }
    
    int lastIndexOf(const T& element) const override {
        int index = count - 1;
        LinkedNode<T>* node = tail;
        while (node) {
            if (node->value == element) return index;
            node = node->prev;
            index--;
        }
        return -1;
    }
    
    List<T>* subList(int fromIndex, int toIndex) override {
        auto* sub = new LinkedList<T>();
        LinkedNode<T>* node = nodeAt(fromIndex);
        for (int i = fromIndex; i < toIndex && node; i++) {
            sub->add(node->value);
            node = node->next;
        }
        return sub;
    }
    
    // Deque operations
    void addFirst(const T& element) {
        LinkedNode<T>* newNode = new LinkedNode<T>(element);
        if (head == nullptr) {
            head = tail = newNode;
        } else {
            newNode->next = head;
            head->prev = newNode;
            head = newNode;
        }
        count++;
    }
    
    void addLast(const T& element) {
        LinkedNode<T>* newNode = new LinkedNode<T>(element);
        if (tail == nullptr) {
            head = tail = newNode;
        } else {
            newNode->prev = tail;
            tail->next = newNode;
            tail = newNode;
        }
        count++;
    }
    
    T removeFirst() {
        if (head == nullptr) return T();
        T value = head->value;
        unlink(head);
        return value;
    }
    
    T removeLast() {
        if (tail == nullptr) return T();
        T value = tail->value;
        unlink(tail);
        return value;
    }
    
    T& getFirst() { return head->value; }
    T& getLast() { return tail->value; }
    
private:
    void unlink(LinkedNode<T>* node) {
        LinkedNode<T>* prev = node->prev;
        LinkedNode<T>* next = node->next;
        
        if (prev == nullptr) {
            head = next;
        } else {
            prev->next = next;
            node->prev = nullptr;
        }
        
        if (next == nullptr) {
            tail = prev;
        } else {
            next->prev = prev;
            node->next = nullptr;
        }
        
        delete node;
        count--;
    }
};

// ============================================================
// HASH MAP ENTRY
// ============================================================
template<typename K, typename V>
struct HashEntry {
    K key;
    V value;
    HashEntry* next;
    uint32_t hash;
    
    HashEntry(const K& k, const V& v, uint32_t h)
        : key(k), value(v), next(nullptr), hash(h) {}
};

// ============================================================
// HASHMAP - Tabela hash com encadeamento
// ============================================================
template<typename K, typename V>
class HashMap : public Map<K, V> {
private:
    HashEntry<K, V>** table;
    int count;
    int capacity;
    float loadFactor;
    int threshold;
    
    uint32_t hash(const K& key) const {
        // Hash simples para tipos primitivos
        return std::hash<K>{}(key);
    }
    
    int indexFor(uint32_t h) const {
        return h & (capacity - 1);
    }
    
    void resize(int newCapacity) {
        HashEntry<K, V>** newTable = new HashEntry<K, V>*[newCapacity]();
        
        // Rehash all entries
        for (int i = 0; i < capacity; i++) {
            HashEntry<K, V>* entry = table[i];
            while (entry) {
                HashEntry<K, V>* next = entry->next;
                int newIndex = entry->hash & (newCapacity - 1);
                entry->next = newTable[newIndex];
                newTable[newIndex] = entry;
                entry = next;
            }
        }
        
        delete[] table;
        table = newTable;
        capacity = newCapacity;
        threshold = static_cast<int>(capacity * loadFactor);
    }
    
public:
    explicit HashMap(int initialCapacity = 16, float lf = 0.75f)
        : count(0), capacity(initialCapacity), loadFactor(lf) {
        // Garante que capacidade é potência de 2
        int cap = 1;
        while (cap < initialCapacity) cap <<= 1;
        capacity = cap;
        
        table = new HashEntry<K, V>*[capacity]();
        threshold = static_cast<int>(capacity * loadFactor);
    }
    
    ~HashMap() {
        clear();
        delete[] table;
    }
    
    // Map interface
    int size() const override { return count; }
    
    bool containsKey(const K& key) const override {
        return get(key) != nullptr;
    }
    
    bool containsValue(const V& value) const override {
        for (int i = 0; i < capacity; i++) {
            HashEntry<K, V>* entry = table[i];
            while (entry) {
                if (entry->value == value) return true;
                entry = entry->next;
            }
        }
        return false;
    }
    
    V* get(const K& key) override {
        uint32_t h = hash(key);
        int index = indexFor(h);
        
        HashEntry<K, V>* entry = table[index];
        while (entry) {
            if (entry->hash == h && entry->key == key) {
                return &entry->value;
            }
            entry = entry->next;
        }
        return nullptr;
    }
    
    V put(const K& key, const V& value) override {
        uint32_t h = hash(key);
        int index = indexFor(h);
        
        // Procura entrada existente
        HashEntry<K, V>* entry = table[index];
        while (entry) {
            if (entry->hash == h && entry->key == key) {
                V oldValue = entry->value;
                entry->value = value;
                return oldValue;
            }
            entry = entry->next;
        }
        
        // Adiciona nova entrada
        if (count >= threshold) {
            resize(capacity * 2);
            index = indexFor(h);
        }
        
        HashEntry<K, V>* newEntry = new HashEntry<K, V>(key, value, h);
        newEntry->next = table[index];
        table[index] = newEntry;
        count++;
        
        return V();
    }
    
    V remove(const K& key) override {
        uint32_t h = hash(key);
        int index = indexFor(h);
        
        HashEntry<K, V>* prev = nullptr;
        HashEntry<K, V>* entry = table[index];
        
        while (entry) {
            if (entry->hash == h && entry->key == key) {
                V oldValue = entry->value;
                
                if (prev == nullptr) {
                    table[index] = entry->next;
                } else {
                    prev->next = entry->next;
                }
                
                delete entry;
                count--;
                return oldValue;
            }
            prev = entry;
            entry = entry->next;
        }
        
        return V();
    }
    
    void clear() override {
        for (int i = 0; i < capacity; i++) {
            HashEntry<K, V>* entry = table[i];
            while (entry) {
                HashEntry<K, V>* next = entry->next;
                delete entry;
                entry = next;
            }
            table[i] = nullptr;
        }
        count = 0;
    }
    
    Set<K>* keySet() override {
        auto* keys = new HashSet<K>();
        for (int i = 0; i < capacity; i++) {
            HashEntry<K, V>* entry = table[i];
            while (entry) {
                keys->add(entry->key);
                entry = entry->next;
            }
        }
        return keys;
    }
    
    Collection<V>* values() override {
        auto* vals = new ArrayList<V>();
        for (int i = 0; i < capacity; i++) {
            HashEntry<K, V>* entry = table[i];
            while (entry) {
                vals->add(entry->value);
                entry = entry->next;
            }
        }
        return vals;
    }
    
    // Iteration helper
    void forEach(std::function<void(const K&, V&)> action) {
        for (int i = 0; i < capacity; i++) {
            HashEntry<K, V>* entry = table[i];
            while (entry) {
                action(entry->key, entry->value);
                entry = entry->next;
            }
        }
    }
};

// ============================================================
// HASHSET - Set baseado em HashMap
// ============================================================
template<typename T>
class HashSet : public Set<T> {
private:
    HashMap<T, bool> map;
    
public:
    explicit HashSet(int initialCapacity = 16)
        : map(initialCapacity) {}
    
    int size() const override { return map.size(); }
    
    bool contains(const T& element) const override {
        return const_cast<HashMap<T, bool>&>(map).containsKey(element);
    }
    
    bool add(const T& element) override {
        if (contains(element)) return false;
        map.put(element, true);
        return true;
    }
    
    bool remove(const T& element) override {
        if (!contains(element)) return false;
        map.remove(element);
        return true;
    }
    
    void clear() override {
        map.clear();
    }
    
    void toArray(T* array) const override {
        auto* keys = const_cast<HashMap<T, bool>&>(map).keySet();
        keys->toArray(array);
        delete keys;
    }
    
    Iterator<T> iterator() override {
        // TODO: Implementar SetIterator
        return ArrayIterator<T>(nullptr, 0);
    }
};

// ============================================================
// STACK - Pilha LIFO
// ============================================================
template<typename T>
class Stack : public ArrayList<T> {
public:
    void push(const T& item) {
        this->add(item);
    }
    
    T pop() {
        if (this->isEmpty()) return T();
        return this->removeAt(this->size() - 1);
    }
    
    T& peek() {
        return this->get(this->size() - 1);
    }
    
    int search(const T& item) const {
        int i = this->lastIndexOf(item);
        if (i >= 0) {
            return this->size() - i;
        }
        return -1;
    }
};

// ============================================================
// QUEUE - Fila FIFO (usando LinkedList)
// ============================================================
template<typename T>
class Queue {
private:
    LinkedList<T> list;
    
public:
    bool offer(const T& element) {
        list.addLast(element);
        return true;
    }
    
    T poll() {
        if (list.isEmpty()) return T();
        return list.removeFirst();
    }
    
    T& peek() {
        return list.getFirst();
    }
    
    int size() const { return list.size(); }
    bool isEmpty() const { return list.isEmpty(); }
    void clear() { list.clear(); }
};

// ============================================================
// PRIORITY QUEUE - Fila com prioridade (heap)
// ============================================================
template<typename T, typename Compare = std::less<T>>
class PriorityQueue {
private:
    ArrayList<T> heap;
    Compare comp;
    
    void siftUp(int index) {
        while (index > 0) {
            int parent = (index - 1) / 2;
            if (comp(heap[index], heap[parent])) {
                std::swap(heap[index], heap[parent]);
                index = parent;
            } else {
                break;
            }
        }
    }
    
    void siftDown(int index) {
        int size = heap.size();
        while (true) {
            int left = 2 * index + 1;
            int right = 2 * index + 2;
            int smallest = index;
            
            if (left < size && comp(heap[left], heap[smallest])) {
                smallest = left;
            }
            if (right < size && comp(heap[right], heap[smallest])) {
                smallest = right;
            }
            
            if (smallest != index) {
                std::swap(heap[index], heap[smallest]);
                index = smallest;
            } else {
                break;
            }
        }
    }
    
public:
    bool offer(const T& element) {
        heap.add(element);
        siftUp(heap.size() - 1);
        return true;
    }
    
    T poll() {
        if (heap.isEmpty()) return T();
        
        T result = heap[0];
        T last = heap.removeAt(heap.size() - 1);
        
        if (!heap.isEmpty()) {
            heap.set(0, last);
            siftDown(0);
        }
        
        return result;
    }
    
    T& peek() {
        return heap[0];
    }
    
    int size() const { return heap.size(); }
    bool isEmpty() const { return heap.isEmpty(); }
    void clear() { heap.clear(); }
};

// ============================================================
// UTILITY: COLLECTIONS (métodos estáticos)
// ============================================================
class Collections {
public:
    // Sort usando quicksort
    template<typename T>
    static void sort(List<T>& list) {
        if (list.size() <= 1) return;
        quickSort(list, 0, list.size() - 1);
    }
    
    template<typename T, typename Compare>
    static void sort(List<T>& list, Compare comp) {
        if (list.size() <= 1) return;
        quickSort(list, 0, list.size() - 1, comp);
    }
    
    // Binary search
    template<typename T>
    static int binarySearch(const List<T>& list, const T& key) {
        int low = 0;
        int high = list.size() - 1;
        
        while (low <= high) {
            int mid = (low + high) / 2;
            const T& midVal = list.get(mid);
            
            if (midVal < key) {
                low = mid + 1;
            } else if (key < midVal) {
                high = mid - 1;
            } else {
                return mid;
            }
        }
        
        return -(low + 1);
    }
    
    // Reverse
    template<typename T>
    static void reverse(List<T>& list) {
        int size = list.size();
        for (int i = 0; i < size / 2; i++) {
            T temp = list.get(i);
            list.set(i, list.get(size - 1 - i));
            list.set(size - 1 - i, temp);
        }
    }
    
    // Shuffle
    template<typename T>
    static void shuffle(List<T>& list) {
        int size = list.size();
        for (int i = size - 1; i > 0; i--) {
            int j = rand() % (i + 1);
            T temp = list.get(i);
            list.set(i, list.get(j));
            list.set(j, temp);
        }
    }
    
    // Min/Max
    template<typename T>
    static T min(const Collection<T>& coll) {
        T* arr = new T[coll.size()];
        coll.toArray(arr);
        T minVal = arr[0];
        for (int i = 1; i < coll.size(); i++) {
            if (arr[i] < minVal) minVal = arr[i];
        }
        delete[] arr;
        return minVal;
    }
    
    template<typename T>
    static T max(const Collection<T>& coll) {
        T* arr = new T[coll.size()];
        coll.toArray(arr);
        T maxVal = arr[0];
        for (int i = 1; i < coll.size(); i++) {
            if (maxVal < arr[i]) maxVal = arr[i];
        }
        delete[] arr;
        return maxVal;
    }
    
private:
    template<typename T>
    static void quickSort(List<T>& list, int low, int high) {
        if (low < high) {
            int pi = partition(list, low, high);
            quickSort(list, low, pi - 1);
            quickSort(list, pi + 1, high);
        }
    }
    
    template<typename T, typename Compare>
    static void quickSort(List<T>& list, int low, int high, Compare comp) {
        if (low < high) {
            int pi = partition(list, low, high, comp);
            quickSort(list, low, pi - 1, comp);
            quickSort(list, pi + 1, high, comp);
        }
    }
    
    template<typename T>
    static int partition(List<T>& list, int low, int high) {
        T pivot = list.get(high);
        int i = low - 1;
        
        for (int j = low; j < high; j++) {
            if (list.get(j) < pivot) {
                i++;
                T temp = list.get(i);
                list.set(i, list.get(j));
                list.set(j, temp);
            }
        }
        
        T temp = list.get(i + 1);
        list.set(i + 1, list.get(high));
        list.set(high, temp);
        
        return i + 1;
    }
    
    template<typename T, typename Compare>
    static int partition(List<T>& list, int low, int high, Compare comp) {
        T pivot = list.get(high);
        int i = low - 1;
        
        for (int j = low; j < high; j++) {
            if (comp(list.get(j), pivot)) {
                i++;
                T temp = list.get(i);
                list.set(i, list.get(j));
                list.set(j, temp);
            }
        }
        
        T temp = list.get(i + 1);
        list.set(i + 1, list.get(high));
        list.set(high, temp);
        
        return i + 1;
    }
};

} // namespace Kava

#endif // KAVA_COLLECTIONS_H
