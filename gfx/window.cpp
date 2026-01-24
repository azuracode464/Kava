/*
 * MIT License
 * Copyright (c) 2026 KAVA Team
 */

#include <iostream>
#include <string>

namespace Kava {
namespace Gfx {

class Window {
public:
    Window(int width, int height, std::string title) {
        std::cout << "[GFX] Janela criada: " << title << " (" << width << "x" << height << ")" << std::endl;
        open = true;
    }

    bool isOpen() { return open; }
    void clear() { std::cout << "[GFX] Tela limpa" << std::endl; }
    void drawRect(int x, int y, int w, int h) {
        std::cout << "[GFX] Retângulo em (" << x << "," << y << ") tamanho " << w << "x" << h << std::endl;
    }
    void present() { std::cout << "[GFX] Frame apresentado" << std::endl; }
    void close() { open = false; }

private:
    bool open;
};

} // namespace Gfx
} // namespace Kava
