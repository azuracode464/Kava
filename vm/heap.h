/*
 * MIT License
 * Copyright (c) 2026 KAVA Team
 */

#ifndef KAVA_HEAP_H
#define KAVA_HEAP_H

#include <vector>
#include <string>
#include <map>
#include <stdint.h>

namespace Kava {

enum class ObjType {
    String,
    Instance,
    Array
};

struct Object {
    ObjType type;
    bool marked;
    
    Object(ObjType t) : type(t), marked(false) {}
    virtual ~Object() = default;
};

struct StringObj : public Object {
    std::string value;
    StringObj(std::string v) : Object(ObjType::String), value(v) {}
};

struct InstanceObj : public Object {
    std::map<int, struct Value> fields;
    int classId;
    InstanceObj(int id) : Object(ObjType::Instance), classId(id) {}
};

class Heap {
public:
    std::vector<Object*> objects;
    
    template<typename T, typename... Args>
    T* allocate(Args&&... args) {
        T* obj = new T(std::forward<Args>(args)...);
        objects.push_back(obj);
        return obj;
    }
    
    void collect(const std::vector<struct Value>& roots);
    
    ~Heap() {
        for (auto obj : objects) delete obj;
    }
};

} // namespace Kava

#endif // KAVA_HEAP_H
