/*
 * MIT License - KAVA 2.5
 * KPM - KAVA Package Manager
 * Gerenciador de pacotes: init, add, build, test, publish
 * Versionamento semântico e resolução de dependências
 */

#ifndef KAVA_KPM_H
#define KAVA_KPM_H

#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <sstream>
#include <iostream>
#include <filesystem>
#include <algorithm>
#include <cstdlib>
#include <chrono>

namespace fs = std::filesystem;

namespace Kava {

// ============================================================
// SEMVER - Versionamento Semântico
// ============================================================
struct SemVer {
    int major = 1;
    int minor = 0;
    int patch = 0;
    std::string prerelease;
    
    SemVer() = default;
    SemVer(int maj, int min, int pat) : major(maj), minor(min), patch(pat) {}
    
    static SemVer parse(const std::string& s) {
        SemVer v;
        if (sscanf(s.c_str(), "%d.%d.%d", &v.major, &v.minor, &v.patch) < 3) {
            if (sscanf(s.c_str(), "%d.%d", &v.major, &v.minor) < 2) {
                sscanf(s.c_str(), "%d", &v.major);
            }
        }
        auto dash = s.find('-');
        if (dash != std::string::npos) v.prerelease = s.substr(dash + 1);
        return v;
    }
    
    std::string toString() const {
        std::string s = std::to_string(major) + "." + std::to_string(minor) + "." + std::to_string(patch);
        if (!prerelease.empty()) s += "-" + prerelease;
        return s;
    }
    
    bool operator>(const SemVer& o) const {
        if (major != o.major) return major > o.major;
        if (minor != o.minor) return minor > o.minor;
        return patch > o.patch;
    }
    
    bool operator>=(const SemVer& o) const { return !(*this < o); }
    bool operator<(const SemVer& o) const { return o > *this; }
    bool operator==(const SemVer& o) const { return major == o.major && minor == o.minor && patch == o.patch; }
    
    bool satisfies(const std::string& range) const {
        if (range.empty() || range == "*") return true;
        if (range[0] == '^') {
            SemVer min = SemVer::parse(range.substr(1));
            return major == min.major && *this >= min;
        }
        if (range[0] == '~') {
            SemVer min = SemVer::parse(range.substr(1));
            return major == min.major && minor == min.minor && patch >= min.patch;
        }
        if (range.find(">=") == 0) {
            return *this >= SemVer::parse(range.substr(2));
        }
        return *this == SemVer::parse(range);
    }
};

// ============================================================
// DEPENDENCY
// ============================================================
struct Dependency {
    std::string name;
    std::string version;  // semver range
    bool isDev = false;
    
    std::string toString() const {
        return name + "@" + version;
    }
};

// ============================================================
// PACKAGE MANIFEST (kava.json)
// ============================================================
struct PackageManifest {
    std::string name;
    std::string version;
    std::string description;
    std::string author;
    std::string license;
    std::string main;  // entry point
    
    std::vector<Dependency> dependencies;
    std::vector<Dependency> devDependencies;
    
    struct Scripts {
        std::string build;
        std::string test;
        std::string start;
        std::string dev;
    } scripts;
    
    std::vector<std::string> keywords;
    std::string repository;
    
    // Serializar para JSON
    std::string toJSON() const {
        std::ostringstream ss;
        ss << "{\n";
        ss << "  \"name\": \"" << name << "\",\n";
        ss << "  \"version\": \"" << version << "\",\n";
        ss << "  \"description\": \"" << description << "\",\n";
        ss << "  \"author\": \"" << author << "\",\n";
        ss << "  \"license\": \"" << license << "\",\n";
        ss << "  \"main\": \"" << main << "\",\n";
        
        ss << "  \"scripts\": {\n";
        ss << "    \"build\": \"" << scripts.build << "\",\n";
        ss << "    \"test\": \"" << scripts.test << "\",\n";
        ss << "    \"start\": \"" << scripts.start << "\",\n";
        ss << "    \"dev\": \"" << scripts.dev << "\"\n";
        ss << "  },\n";
        
        ss << "  \"dependencies\": {\n";
        for (size_t i = 0; i < dependencies.size(); i++) {
            ss << "    \"" << dependencies[i].name << "\": \"" << dependencies[i].version << "\"";
            if (i + 1 < dependencies.size()) ss << ",";
            ss << "\n";
        }
        ss << "  },\n";
        
        ss << "  \"devDependencies\": {\n";
        for (size_t i = 0; i < devDependencies.size(); i++) {
            ss << "    \"" << devDependencies[i].name << "\": \"" << devDependencies[i].version << "\"";
            if (i + 1 < devDependencies.size()) ss << ",";
            ss << "\n";
        }
        ss << "  },\n";
        
        ss << "  \"keywords\": [";
        for (size_t i = 0; i < keywords.size(); i++) {
            if (i > 0) ss << ", ";
            ss << "\"" << keywords[i] << "\"";
        }
        ss << "]\n";
        
        ss << "}\n";
        return ss.str();
    }
    
    // Parse simples de kava.json
    static PackageManifest fromJSON(const std::string& json) {
        PackageManifest m;
        // Simplified parsing - extract key fields
        auto extractField = [&json](const std::string& key) -> std::string {
            std::string search = "\"" + key + "\": \"";
            auto pos = json.find(search);
            if (pos == std::string::npos) return "";
            pos += search.size();
            auto end = json.find("\"", pos);
            if (end == std::string::npos) return "";
            return json.substr(pos, end - pos);
        };
        
        m.name = extractField("name");
        m.version = extractField("version");
        m.description = extractField("description");
        m.author = extractField("author");
        m.license = extractField("license");
        m.main = extractField("main");
        m.scripts.build = extractField("build");
        m.scripts.test = extractField("test");
        m.scripts.start = extractField("start");
        
        return m;
    }
};

// ============================================================
// KPM - KAVA PACKAGE MANAGER
// ============================================================
class KPM {
public:
    std::string projectDir;
    PackageManifest manifest;
    
    KPM() : projectDir(fs::current_path().string()) {}
    KPM(const std::string& dir) : projectDir(dir) {}
    
    // ========================================
    // kpm init - Inicializa novo projeto
    // ========================================
    int cmdInit(const std::string& name = "") {
        std::string projName = name.empty() ? fs::path(projectDir).filename().string() : name;
        
        manifest.name = projName;
        manifest.version = "1.0.0";
        manifest.description = "A KAVA 2.5 project";
        manifest.author = "";
        manifest.license = "MIT";
        manifest.main = "src/main.kava";
        manifest.scripts.build = "kavac src/main.kava";
        manifest.scripts.test = "kavavm tests/test.kvb";
        manifest.scripts.start = "kavavm src/main.kvb";
        manifest.scripts.dev = "kavac src/main.kava && kavavm src/main.kvb";
        
        // Create project structure
        fs::create_directories(projectDir + "/src");
        fs::create_directories(projectDir + "/tests");
        fs::create_directories(projectDir + "/lib");
        fs::create_directories(projectDir + "/kava_modules");
        
        // Write kava.json
        writeManifest();
        
        // Create main.kava
        std::ofstream mainFile(projectDir + "/src/main.kava");
        mainFile << "/*\n * " << projName << " - KAVA 2.5 Project\n */\n\n"
                 << "print \"Hello from " << projName << "!\"\n";
        mainFile.close();
        
        // Create test
        std::ofstream testFile(projectDir + "/tests/test.kava");
        testFile << "/*\n * Tests for " << projName << "\n */\n\n"
                 << "let x = 1 + 1\n"
                 << "if (x == 2) {\n"
                 << "    print \"PASS: basic arithmetic\"\n"
                 << "} else {\n"
                 << "    print \"FAIL: basic arithmetic\"\n"
                 << "}\n";
        testFile.close();
        
        // Create .gitignore
        std::ofstream gitignore(projectDir + "/.gitignore");
        gitignore << "*.kvb\nkava_modules/\n.kpm/\n";
        gitignore.close();
        
        std::cout << "\n  Initialized KAVA project: " << projName << "\n";
        std::cout << "  Location: " << projectDir << "\n\n";
        std::cout << "  Created:\n";
        std::cout << "    kava.json\n";
        std::cout << "    src/main.kava\n";
        std::cout << "    tests/test.kava\n";
        std::cout << "    .gitignore\n\n";
        std::cout << "  Commands:\n";
        std::cout << "    kpm build   - Compile project\n";
        std::cout << "    kpm test    - Run tests\n";
        std::cout << "    kpm start   - Run project\n\n";
        
        return 0;
    }
    
    // ========================================
    // kpm add - Adiciona dependência
    // ========================================
    int cmdAdd(const std::string& packageSpec, bool isDev = false) {
        loadManifest();
        
        Dependency dep;
        auto atPos = packageSpec.find('@');
        if (atPos != std::string::npos && atPos > 0) {
            dep.name = packageSpec.substr(0, atPos);
            dep.version = packageSpec.substr(atPos + 1);
        } else {
            dep.name = packageSpec;
            dep.version = "^1.0.0";
        }
        dep.isDev = isDev;
        
        // Check if already exists
        auto& deps = isDev ? manifest.devDependencies : manifest.dependencies;
        for (auto& d : deps) {
            if (d.name == dep.name) {
                d.version = dep.version;
                writeManifest();
                std::cout << "  Updated " << dep.name << " to " << dep.version << "\n";
                return 0;
            }
        }
        
        deps.push_back(dep);
        writeManifest();
        
        // Create package dir
        fs::create_directories(projectDir + "/kava_modules/" + dep.name);
        
        std::cout << "  Added " << dep.toString() << (isDev ? " (dev)" : "") << "\n";
        return 0;
    }
    
    // ========================================
    // kpm build - Compila projeto
    // ========================================
    int cmdBuild() {
        loadManifest();
        
        std::cout << "\n  Building " << manifest.name << " v" << manifest.version << "...\n\n";
        
        // Find all .kava files in src/
        std::vector<std::string> sourceFiles;
        if (fs::exists(projectDir + "/src")) {
            for (auto& entry : fs::recursive_directory_iterator(projectDir + "/src")) {
                if (entry.path().extension() == ".kava") {
                    sourceFiles.push_back(entry.path().string());
                }
            }
        }
        
        if (sourceFiles.empty()) {
            std::cerr << "  No .kava files found in src/\n";
            return 1;
        }
        
        int compiled = 0;
        int errors = 0;
        
        for (auto& file : sourceFiles) {
            std::string cmd = "kavac " + file + " 2>&1";
            int ret = system(cmd.c_str());
            if (ret == 0) {
                compiled++;
            } else {
                errors++;
                std::cerr << "  ERROR compiling: " << file << "\n";
            }
        }
        
        std::cout << "\n  Build complete: " << compiled << " files compiled";
        if (errors > 0) std::cout << ", " << errors << " errors";
        std::cout << "\n\n";
        
        return errors > 0 ? 1 : 0;
    }
    
    // ========================================
    // kpm test - Executa testes
    // ========================================
    int cmdTest() {
        loadManifest();
        
        std::cout << "\n  Running tests for " << manifest.name << "...\n\n";
        
        // Compile tests
        std::vector<std::string> testFiles;
        if (fs::exists(projectDir + "/tests")) {
            for (auto& entry : fs::recursive_directory_iterator(projectDir + "/tests")) {
                if (entry.path().extension() == ".kava") {
                    testFiles.push_back(entry.path().string());
                }
            }
        }
        
        int passed = 0;
        int failed = 0;
        
        for (auto& file : testFiles) {
            // Compile
            std::string compileCmd = "kavac " + file + " > /dev/null 2>&1";
            if (system(compileCmd.c_str()) != 0) {
                std::cout << "  FAIL (compile): " << file << "\n";
                failed++;
                continue;
            }
            
            // Run
            std::string kvb = file.substr(0, file.find_last_of('.')) + ".kvb";
            std::string runCmd = "kavavm " + kvb + " 2>&1";
            int ret = system(runCmd.c_str());
            
            if (ret == 0) {
                std::cout << "  PASS: " << fs::path(file).filename().string() << "\n";
                passed++;
            } else {
                std::cout << "  FAIL: " << fs::path(file).filename().string() << "\n";
                failed++;
            }
        }
        
        std::cout << "\n  Results: " << passed << " passed, " << failed << " failed\n\n";
        return failed > 0 ? 1 : 0;
    }
    
    // ========================================
    // kpm publish - Publica pacote
    // ========================================
    int cmdPublish() {
        loadManifest();
        
        std::cout << "\n  Publishing " << manifest.name << " v" << manifest.version << "...\n";
        
        // Validate
        if (manifest.name.empty()) {
            std::cerr << "  ERROR: package name is required\n";
            return 1;
        }
        if (manifest.version.empty()) {
            std::cerr << "  ERROR: version is required\n";
            return 1;
        }
        
        // Create tarball
        std::string tarName = manifest.name + "-" + manifest.version + ".tar.gz";
        std::string cmd = "tar -czf " + tarName + " --exclude=kava_modules --exclude=.git --exclude='*.kvb' .";
        int ret = system(cmd.c_str());
        
        if (ret == 0) {
            std::cout << "  Package created: " << tarName << "\n";
            std::cout << "  Ready for publishing to KAVA Registry\n\n";
        } else {
            std::cerr << "  ERROR: Failed to create package\n";
            return 1;
        }
        
        return 0;
    }
    
    // ========================================
    // kpm install - Instala dependências
    // ========================================
    int cmdInstall() {
        loadManifest();
        
        std::cout << "\n  Installing dependencies for " << manifest.name << "...\n\n";
        
        for (auto& dep : manifest.dependencies) {
            installPackage(dep);
        }
        for (auto& dep : manifest.devDependencies) {
            installPackage(dep);
        }
        
        std::cout << "\n  Dependencies installed.\n\n";
        return 0;
    }
    
    // ========================================
    // kpm run <script> - Executa script
    // ========================================
    int cmdRun(const std::string& script) {
        loadManifest();
        
        std::string cmd;
        if (script == "build") cmd = manifest.scripts.build;
        else if (script == "test") cmd = manifest.scripts.test;
        else if (script == "start") cmd = manifest.scripts.start;
        else if (script == "dev") cmd = manifest.scripts.dev;
        else {
            std::cerr << "  Unknown script: " << script << "\n";
            return 1;
        }
        
        if (cmd.empty()) {
            std::cerr << "  Script '" << script << "' not defined in kava.json\n";
            return 1;
        }
        
        std::cout << "  > " << cmd << "\n\n";
        return system(cmd.c_str());
    }

private:
    void loadManifest() {
        std::string path = projectDir + "/kava.json";
        if (fs::exists(path)) {
            std::ifstream file(path);
            std::stringstream ss;
            ss << file.rdbuf();
            manifest = PackageManifest::fromJSON(ss.str());
        }
    }
    
    void writeManifest() {
        std::string path = projectDir + "/kava.json";
        std::ofstream file(path);
        file << manifest.toJSON();
    }
    
    void installPackage(const Dependency& dep) {
        std::string pkgDir = projectDir + "/kava_modules/" + dep.name;
        fs::create_directories(pkgDir);
        
        // For stdlib packages, create stub files
        std::vector<std::string> stdlibPkgs = {"http", "json", "fs", "net", "math", "time", "io"};
        bool isStdlib = std::find(stdlibPkgs.begin(), stdlibPkgs.end(), dep.name) != stdlibPkgs.end();
        
        if (isStdlib) {
            std::ofstream index(pkgDir + "/index.kava");
            index << "// KAVA stdlib: " << dep.name << "\n";
            index << "// Auto-installed by kpm\n";
            index.close();
        }
        
        std::cout << "  + " << dep.name << "@" << dep.version;
        if (isStdlib) std::cout << " (stdlib)";
        std::cout << "\n";
    }
};

} // namespace Kava

#endif // KAVA_KPM_H
