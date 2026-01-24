/*
 * MIT License
 * Copyright (c) 2026 KAVA Team
 * 
 * KAVA 2.0 - Garbage Collector Mark-Sweep
 * GC completo inspirado no Java 6 HotSpot
 */

#ifndef KAVA_GC_H
#define KAVA_GC_H

#include <vector>
#include <cstdint>
#include <cstring>
#include <mutex>
#include <atomic>
#include <chrono>
#include <functional>

namespace Kava {

// Forward declarations
class GCObject;
class Heap;
class GarbageCollector;

// ============================================================
// FLAGS DE OBJETO GC
// ============================================================
enum GCFlags : uint8_t {
    GC_FLAG_NONE      = 0x00,
    GC_FLAG_MARKED    = 0x01,  // Objeto alcançável
    GC_FLAG_FINALIZER = 0x02,  // Tem finalizador
    GC_FLAG_FINALIZED = 0x04,  // Finalizador já executado
    GC_FLAG_PINNED    = 0x08,  // Não pode ser movido
    GC_FLAG_OLD_GEN   = 0x10,  // Na geração antiga
    GC_FLAG_ARRAY     = 0x20,  // É um array
    GC_FLAG_STATIC    = 0x40   // Campo estático (root)
};

// ============================================================
// TIPOS DE OBJETO
// ============================================================
enum class GCObjectType : uint8_t {
    INSTANCE,    // Instância de classe
    ARRAY_INT,   // Array de int
    ARRAY_LONG,  // Array de long
    ARRAY_FLOAT, // Array de float
    ARRAY_DOUBLE,// Array de double
    ARRAY_BYTE,  // Array de byte/boolean
    ARRAY_CHAR,  // Array de char
    ARRAY_SHORT, // Array de short
    ARRAY_OBJECT,// Array de referências
    STRING,      // String (otimizado)
    CLASS_INFO   // Metadados de classe
};

// ============================================================
// HEADER DE OBJETO GC
// ============================================================
struct GCHeader {
    uint32_t classId;       // ID da classe
    uint32_t size;          // Tamanho total em bytes
    GCObjectType type;      // Tipo do objeto
    GCFlags flags;          // Flags do GC
    uint16_t age;           // Idade (para generational GC)
    
    // Métodos inline
    bool isMarked() const { return flags & GC_FLAG_MARKED; }
    void mark() { flags = static_cast<GCFlags>(flags | GC_FLAG_MARKED); }
    void unmark() { flags = static_cast<GCFlags>(flags & ~GC_FLAG_MARKED); }
    bool hasFinalizer() const { return flags & GC_FLAG_FINALIZER; }
    bool isArray() const { return flags & GC_FLAG_ARRAY; }
    bool isPinned() const { return flags & GC_FLAG_PINNED; }
    bool isOldGen() const { return flags & GC_FLAG_OLD_GEN; }
};

// ============================================================
// OBJETO GC BASE
// ============================================================
class GCObject {
public:
    GCHeader header;
    
    // Dados começam após o header
    uint8_t data[0];  // Flexible array member
    
    // Obtém ponteiro para dados
    template<typename T>
    T* dataAs() { return reinterpret_cast<T*>(data); }
    
    template<typename T>
    const T* dataAs() const { return reinterpret_cast<const T*>(data); }
    
    // Obtém campo por offset
    template<typename T>
    T& fieldAt(uint32_t offset) {
        return *reinterpret_cast<T*>(data + offset);
    }
    
    // Obtém elemento de array
    template<typename T>
    T& arrayElement(uint32_t index) {
        // Primeiro slot é o length
        return reinterpret_cast<T*>(data + sizeof(int32_t))[index];
    }
    
    int32_t arrayLength() const {
        return *reinterpret_cast<const int32_t*>(data);
    }
};

// ============================================================
// ESTATÍSTICAS DO GC
// ============================================================
struct GCStats {
    uint64_t totalCollections = 0;
    uint64_t minorCollections = 0;
    uint64_t majorCollections = 0;
    uint64_t totalBytesCollected = 0;
    uint64_t totalObjectsCollected = 0;
    uint64_t totalTimeMs = 0;
    uint64_t maxPauseMs = 0;
    uint64_t currentHeapSize = 0;
    uint64_t peakHeapSize = 0;
    
    void reset() {
        totalCollections = 0;
        minorCollections = 0;
        majorCollections = 0;
        totalBytesCollected = 0;
        totalObjectsCollected = 0;
        totalTimeMs = 0;
        maxPauseMs = 0;
    }
    
    double avgPauseMs() const {
        return totalCollections > 0 ? 
            static_cast<double>(totalTimeMs) / totalCollections : 0;
    }
};

// ============================================================
// CONFIGURAÇÃO DO GC
// ============================================================
struct GCConfig {
    size_t initialHeapSize = 16 * 1024 * 1024;   // 16 MB
    size_t maxHeapSize = 256 * 1024 * 1024;      // 256 MB
    size_t youngGenRatio = 3;                     // 1/3 para young gen
    size_t survivorRatio = 8;                     // 1/8 de young para survivor
    uint16_t tenureThreshold = 15;                // Idade para promoção
    float gcTriggerRatio = 0.75f;                 // Trigger GC em 75% uso
    bool enableGenerational = true;
    bool enableCompaction = false;               // Não implementado ainda
    bool verboseGC = false;
};

// ============================================================
// BLOCO DE MEMÓRIA
// ============================================================
struct MemoryBlock {
    uint8_t* start;
    uint8_t* end;
    uint8_t* current;  // Próxima alocação
    
    size_t capacity() const { return end - start; }
    size_t used() const { return current - start; }
    size_t available() const { return end - current; }
    
    void reset() { current = start; }
    
    bool canAllocate(size_t size) const {
        return current + size <= end;
    }
    
    void* allocate(size_t size) {
        if (!canAllocate(size)) return nullptr;
        void* ptr = current;
        current += size;
        return ptr;
    }
};

// ============================================================
// HEAP - Gerenciador de Memória
// ============================================================
class Heap {
public:
    GCConfig config;
    
    // Regiões de memória
    MemoryBlock eden;           // Novos objetos
    MemoryBlock survivor1;      // Survivor from
    MemoryBlock survivor2;      // Survivor to
    MemoryBlock oldGen;         // Objetos antigos
    
    // Lista de todos os objetos (para GC simples)
    std::vector<GCObject*> allObjects;
    
    // Estatísticas
    GCStats stats;
    
    Heap() = default;
    ~Heap();
    
    // Inicialização
    void initialize(const GCConfig& cfg);
    
    // Alocação
    GCObject* allocate(uint32_t classId, GCObjectType type, size_t dataSize);
    GCObject* allocateArray(GCObjectType elemType, int32_t length);
    GCObject* allocateString(const char* str, size_t length);
    
    // Helpers
    size_t totalUsed() const;
    size_t totalCapacity() const;
    float usageRatio() const;
    
    bool needsGC() const {
        return usageRatio() >= config.gcTriggerRatio;
    }
    
private:
    void* allocateInEden(size_t size);
    void* allocateInOldGen(size_t size);
    void expandHeap();
};

// ============================================================
// GARBAGE COLLECTOR - Mark-Sweep com Generational
// ============================================================
class GarbageCollector {
public:
    using RootVisitor = std::function<void(GCObject*&)>;
    
    GarbageCollector(Heap& heap);
    
    // Coleta de lixo
    void collect();
    void collectYoung();    // Minor GC
    void collectFull();     // Major GC
    
    // Registra callbacks para encontrar roots
    void setRootScanner(RootVisitor scanner) { rootScanner = scanner; }
    
    // Adiciona root manual (para variáveis globais, etc)
    void addRoot(GCObject** root);
    void removeRoot(GCObject** root);
    
    // Estatísticas
    const GCStats& getStats() const { return heap.stats; }
    
    // Forçar GC
    void forceGC() { collect(); }
    
    // Write barrier para generational GC
    void writeBarrier(GCObject* parent, GCObject* child);
    
private:
    Heap& heap;
    RootVisitor rootScanner;
    std::vector<GCObject**> roots;
    std::vector<GCObject*> rememberedSet;  // Old -> Young refs
    
    // Fases do GC
    void markPhase();
    void sweepPhase();
    void mark(GCObject* obj);
    void scanObject(GCObject* obj);
    
    // Para generational
    void minorMark();
    void minorSweep();
    void promoteToOldGen(GCObject* obj);
    
    // Timing
    std::chrono::high_resolution_clock::time_point gcStartTime;
    void startTiming();
    uint64_t endTiming();
};

// ============================================================
// IMPLEMENTAÇÕES INLINE
// ============================================================

inline Heap::~Heap() {
    // Libera memória das regiões
    if (eden.start) delete[] eden.start;
    if (survivor1.start) delete[] survivor1.start;
    if (survivor2.start) delete[] survivor2.start;
    if (oldGen.start) delete[] oldGen.start;
    
    // Limpa lista de objetos (não deleta, já foram liberados acima)
    allObjects.clear();
}

inline void Heap::initialize(const GCConfig& cfg) {
    config = cfg;
    
    if (config.enableGenerational) {
        // Divide heap em gerações
        size_t youngSize = config.initialHeapSize / config.youngGenRatio;
        size_t edenSize = youngSize - (youngSize / config.survivorRatio * 2);
        size_t survivorSize = youngSize / config.survivorRatio;
        size_t oldSize = config.initialHeapSize - youngSize;
        
        eden.start = new uint8_t[edenSize];
        eden.end = eden.start + edenSize;
        eden.current = eden.start;
        
        survivor1.start = new uint8_t[survivorSize];
        survivor1.end = survivor1.start + survivorSize;
        survivor1.current = survivor1.start;
        
        survivor2.start = new uint8_t[survivorSize];
        survivor2.end = survivor2.start + survivorSize;
        survivor2.current = survivor2.start;
        
        oldGen.start = new uint8_t[oldSize];
        oldGen.end = oldGen.start + oldSize;
        oldGen.current = oldGen.start;
    } else {
        // Heap simples
        oldGen.start = new uint8_t[config.initialHeapSize];
        oldGen.end = oldGen.start + config.initialHeapSize;
        oldGen.current = oldGen.start;
    }
    
    stats.currentHeapSize = config.initialHeapSize;
    stats.peakHeapSize = config.initialHeapSize;
}

inline GCObject* Heap::allocate(uint32_t classId, GCObjectType type, size_t dataSize) {
    size_t totalSize = sizeof(GCHeader) + dataSize;
    totalSize = (totalSize + 7) & ~7;  // Alinha em 8 bytes
    
    void* ptr = config.enableGenerational ? 
        allocateInEden(totalSize) : allocateInOldGen(totalSize);
    
    if (!ptr) return nullptr;
    
    GCObject* obj = static_cast<GCObject*>(ptr);
    obj->header.classId = classId;
    obj->header.size = totalSize;
    obj->header.type = type;
    obj->header.flags = GC_FLAG_NONE;
    obj->header.age = 0;
    
    // Zero-initialize data
    std::memset(obj->data, 0, dataSize);
    
    allObjects.push_back(obj);
    
    return obj;
}

inline GCObject* Heap::allocateArray(GCObjectType elemType, int32_t length) {
    size_t elemSize = 4;  // Default int
    switch (elemType) {
        case GCObjectType::ARRAY_BYTE: elemSize = 1; break;
        case GCObjectType::ARRAY_SHORT:
        case GCObjectType::ARRAY_CHAR: elemSize = 2; break;
        case GCObjectType::ARRAY_LONG:
        case GCObjectType::ARRAY_DOUBLE: elemSize = 8; break;
        default: elemSize = 4; break;
    }
    
    size_t dataSize = sizeof(int32_t) + length * elemSize;  // length + elements
    
    GCObject* obj = allocate(0, elemType, dataSize);
    if (obj) {
        obj->header.flags = static_cast<GCFlags>(obj->header.flags | GC_FLAG_ARRAY);
        *reinterpret_cast<int32_t*>(obj->data) = length;
    }
    
    return obj;
}

inline GCObject* Heap::allocateString(const char* str, size_t length) {
    // String = length (int32) + char data + null terminator
    size_t dataSize = sizeof(int32_t) + length + 1;
    
    GCObject* obj = allocate(0, GCObjectType::STRING, dataSize);
    if (obj) {
        *reinterpret_cast<int32_t*>(obj->data) = static_cast<int32_t>(length);
        std::memcpy(obj->data + sizeof(int32_t), str, length);
        obj->data[sizeof(int32_t) + length] = '\0';
    }
    
    return obj;
}

inline void* Heap::allocateInEden(size_t size) {
    if (eden.canAllocate(size)) {
        return eden.allocate(size);
    }
    // Eden cheio - precisa de GC
    return nullptr;
}

inline void* Heap::allocateInOldGen(size_t size) {
    if (oldGen.canAllocate(size)) {
        return oldGen.allocate(size);
    }
    // Old gen cheio
    return nullptr;
}

inline size_t Heap::totalUsed() const {
    size_t used = 0;
    if (eden.start) used += eden.used();
    if (survivor1.start) used += survivor1.used();
    if (oldGen.start) used += oldGen.used();
    return used;
}

inline size_t Heap::totalCapacity() const {
    size_t cap = 0;
    if (eden.start) cap += eden.capacity();
    if (survivor1.start) cap += survivor1.capacity();
    if (survivor2.start) cap += survivor2.capacity();
    if (oldGen.start) cap += oldGen.capacity();
    return cap;
}

inline float Heap::usageRatio() const {
    size_t cap = totalCapacity();
    return cap > 0 ? static_cast<float>(totalUsed()) / cap : 1.0f;
}

// ============================================================
// GARBAGE COLLECTOR IMPLEMENTATION
// ============================================================

inline GarbageCollector::GarbageCollector(Heap& h) : heap(h) {}

inline void GarbageCollector::startTiming() {
    gcStartTime = std::chrono::high_resolution_clock::now();
}

inline uint64_t GarbageCollector::endTiming() {
    auto end = std::chrono::high_resolution_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        end - gcStartTime).count();
}

inline void GarbageCollector::collect() {
    if (heap.config.enableGenerational) {
        collectYoung();
        if (heap.oldGen.used() > heap.oldGen.capacity() * 0.75) {
            collectFull();
        }
    } else {
        collectFull();
    }
}

inline void GarbageCollector::collectYoung() {
    startTiming();
    
    // Minor collection
    minorMark();
    minorSweep();
    
    uint64_t elapsed = endTiming();
    heap.stats.minorCollections++;
    heap.stats.totalCollections++;
    heap.stats.totalTimeMs += elapsed;
    if (elapsed > heap.stats.maxPauseMs) {
        heap.stats.maxPauseMs = elapsed;
    }
}

inline void GarbageCollector::collectFull() {
    startTiming();
    
    markPhase();
    sweepPhase();
    
    uint64_t elapsed = endTiming();
    heap.stats.majorCollections++;
    heap.stats.totalCollections++;
    heap.stats.totalTimeMs += elapsed;
    if (elapsed > heap.stats.maxPauseMs) {
        heap.stats.maxPauseMs = elapsed;
    }
    
    if (heap.config.verboseGC) {
        // Log GC info
    }
}

inline void GarbageCollector::markPhase() {
    // Desmarca todos os objetos
    for (auto* obj : heap.allObjects) {
        if (obj) obj->header.unmark();
    }
    
    // Marca a partir dos roots
    for (auto* root : roots) {
        if (*root) mark(*root);
    }
    
    // Scanner customizado para roots da VM
    if (rootScanner) {
        for (auto* obj : heap.allObjects) {
            if (obj && obj->header.isMarked()) {
                scanObject(obj);
            }
        }
    }
}

inline void GarbageCollector::mark(GCObject* obj) {
    if (!obj || obj->header.isMarked()) return;
    
    obj->header.mark();
    scanObject(obj);
}

inline void GarbageCollector::scanObject(GCObject* obj) {
    // Escaneia campos de referência do objeto
    if (obj->header.type == GCObjectType::INSTANCE) {
        // TODO: Usar metadata da classe para encontrar campos de referência
    } else if (obj->header.type == GCObjectType::ARRAY_OBJECT) {
        int32_t length = obj->arrayLength();
        GCObject** elements = reinterpret_cast<GCObject**>(obj->data + sizeof(int32_t));
        for (int32_t i = 0; i < length; i++) {
            if (elements[i]) mark(elements[i]);
        }
    }
}

inline void GarbageCollector::sweepPhase() {
    size_t freedBytes = 0;
    size_t freedObjects = 0;
    
    auto it = heap.allObjects.begin();
    while (it != heap.allObjects.end()) {
        GCObject* obj = *it;
        if (obj && !obj->header.isMarked()) {
            freedBytes += obj->header.size;
            freedObjects++;
            it = heap.allObjects.erase(it);
            // Nota: Não deletamos o ponteiro pois a memória é gerenciada em blocos
        } else {
            ++it;
        }
    }
    
    heap.stats.totalBytesCollected += freedBytes;
    heap.stats.totalObjectsCollected += freedObjects;
}

inline void GarbageCollector::minorMark() {
    // Marca objetos na young generation a partir de roots
    for (auto* root : roots) {
        if (*root && !(*root)->header.isOldGen()) {
            mark(*root);
        }
    }
    
    // Marca a partir do remembered set (refs old -> young)
    for (auto* obj : rememberedSet) {
        if (obj && !obj->header.isOldGen()) {
            mark(obj);
        }
    }
}

inline void GarbageCollector::minorSweep() {
    // Promove objetos sobreviventes
    for (auto* obj : heap.allObjects) {
        if (obj && obj->header.isMarked() && !obj->header.isOldGen()) {
            obj->header.age++;
            if (obj->header.age >= heap.config.tenureThreshold) {
                promoteToOldGen(obj);
            }
        }
    }
    
    // Reseta eden
    heap.eden.reset();
    
    // Limpa remembered set
    rememberedSet.clear();
}

inline void GarbageCollector::promoteToOldGen(GCObject* obj) {
    obj->header.flags = static_cast<GCFlags>(obj->header.flags | GC_FLAG_OLD_GEN);
    // TODO: Copiar para old gen se usando copying collection
}

inline void GarbageCollector::addRoot(GCObject** root) {
    roots.push_back(root);
}

inline void GarbageCollector::removeRoot(GCObject** root) {
    auto it = std::find(roots.begin(), roots.end(), root);
    if (it != roots.end()) {
        roots.erase(it);
    }
}

inline void GarbageCollector::writeBarrier(GCObject* parent, GCObject* child) {
    // Se parent está na old gen e child na young gen, adiciona ao remembered set
    if (parent && child && 
        parent->header.isOldGen() && !child->header.isOldGen()) {
        rememberedSet.push_back(child);
    }
}

} // namespace Kava

#endif // KAVA_GC_H
