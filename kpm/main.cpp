/*
 * MIT License - KAVA 2.5
 * KPM CLI - KAVA Package Manager Command Line Interface
 */

#include "kpm.h"
#include <iostream>
#include <string>

void printUsage() {
    std::cout << "\n";
    std::cout << "  KPM - KAVA Package Manager v2.5\n\n";
    std::cout << "  Usage: kpm <command> [options]\n\n";
    std::cout << "  Commands:\n";
    std::cout << "    init [name]     Initialize a new KAVA project\n";
    std::cout << "    add <pkg>       Add a dependency (e.g., kpm add http@^1.0)\n";
    std::cout << "    add -D <pkg>    Add a dev dependency\n";
    std::cout << "    install         Install all dependencies\n";
    std::cout << "    build           Build the project\n";
    std::cout << "    test            Run tests\n";
    std::cout << "    publish         Publish package\n";
    std::cout << "    run <script>    Run a script (build, test, start, dev)\n";
    std::cout << "    version         Show version\n";
    std::cout << "\n";
}

int main(int argc, char** argv) {
    if (argc < 2) {
        printUsage();
        return 0;
    }
    
    std::string cmd = argv[1];
    Kava::KPM kpm;
    
    if (cmd == "init") {
        std::string name = (argc > 2) ? argv[2] : "";
        return kpm.cmdInit(name);
    }
    else if (cmd == "add") {
        if (argc < 3) {
            std::cerr << "  Usage: kpm add <package[@version]>\n";
            return 1;
        }
        bool isDev = false;
        std::string pkg;
        if (std::string(argv[2]) == "-D" || std::string(argv[2]) == "--dev") {
            isDev = true;
            if (argc < 4) {
                std::cerr << "  Usage: kpm add -D <package[@version]>\n";
                return 1;
            }
            pkg = argv[3];
        } else {
            pkg = argv[2];
        }
        return kpm.cmdAdd(pkg, isDev);
    }
    else if (cmd == "install" || cmd == "i") {
        return kpm.cmdInstall();
    }
    else if (cmd == "build" || cmd == "b") {
        return kpm.cmdBuild();
    }
    else if (cmd == "test" || cmd == "t") {
        return kpm.cmdTest();
    }
    else if (cmd == "publish") {
        return kpm.cmdPublish();
    }
    else if (cmd == "run") {
        if (argc < 3) {
            std::cerr << "  Usage: kpm run <script>\n";
            return 1;
        }
        return kpm.cmdRun(argv[2]);
    }
    else if (cmd == "version" || cmd == "-v" || cmd == "--version") {
        std::cout << "  KPM v2.5.0 (KAVA Package Manager)\n";
        return 0;
    }
    else if (cmd == "help" || cmd == "-h" || cmd == "--help") {
        printUsage();
        return 0;
    }
    else {
        std::cerr << "  Unknown command: " << cmd << "\n";
        printUsage();
        return 1;
    }
}
