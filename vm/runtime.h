/*
 * MIT License - KAVA 2.5 Runtime
 * Runtime estilo Node.js: Event Loop, HTTP Server, Async IO, JSON, WebSocket
 */

#ifndef KAVA_RUNTIME_H
#define KAVA_RUNTIME_H

#include "async.h"
#include <string>
#include <vector>
#include <map>
#include <functional>
#include <sstream>
#include <cstring>
#include <thread>
#include <mutex>
#include <atomic>
#include <fstream>
#include <chrono>
#include <cstdio>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>

namespace Kava {

// ============================================================
// JSON PARSER/SERIALIZER (Fast, simple)
// ============================================================
class JSON {
public:
    enum class Type { Null, Bool, Int, Double, String, Array, Object };
    
    Type type = Type::Null;
    bool boolVal = false;
    int64_t intVal = 0;
    double doubleVal = 0;
    std::string strVal;
    std::vector<JSON> arrayVal;
    std::map<std::string, JSON> objectVal;
    
    JSON() : type(Type::Null) {}
    JSON(bool v) : type(Type::Bool), boolVal(v) {}
    JSON(int v) : type(Type::Int), intVal(v) {}
    JSON(int64_t v) : type(Type::Int), intVal(v) {}
    JSON(double v) : type(Type::Double), doubleVal(v) {}
    JSON(const std::string& v) : type(Type::String), strVal(v) {}
    JSON(const char* v) : type(Type::String), strVal(v) {}
    
    static JSON object() { JSON j; j.type = Type::Object; return j; }
    static JSON array() { JSON j; j.type = Type::Array; return j; }
    static JSON null() { return JSON(); }
    
    JSON& operator[](const std::string& key) {
        if (type != Type::Object) type = Type::Object;
        return objectVal[key];
    }
    
    void push(const JSON& val) {
        if (type != Type::Array) type = Type::Array;
        arrayVal.push_back(val);
    }
    
    std::string stringify() const {
        std::ostringstream ss;
        writeJSON(ss);
        return ss.str();
    }
    
    static JSON parse(const std::string& input) {
        size_t pos = 0;
        return parseValue(input, pos);
    }

private:
    void writeJSON(std::ostringstream& ss) const {
        switch (type) {
            case Type::Null: ss << "null"; break;
            case Type::Bool: ss << (boolVal ? "true" : "false"); break;
            case Type::Int: ss << intVal; break;
            case Type::Double: ss << doubleVal; break;
            case Type::String: 
                ss << '"';
                for (char c : strVal) {
                    if (c == '"') ss << "\\\"";
                    else if (c == '\\') ss << "\\\\";
                    else if (c == '\n') ss << "\\n";
                    else if (c == '\t') ss << "\\t";
                    else ss << c;
                }
                ss << '"';
                break;
            case Type::Array:
                ss << '[';
                for (size_t i = 0; i < arrayVal.size(); i++) {
                    if (i > 0) ss << ',';
                    arrayVal[i].writeJSON(ss);
                }
                ss << ']';
                break;
            case Type::Object:
                ss << '{';
                { bool first = true;
                  for (auto& [k, v] : objectVal) {
                      if (!first) ss << ',';
                      first = false;
                      ss << '"' << k << "\":";
                      v.writeJSON(ss);
                  }
                }
                ss << '}';
                break;
        }
    }
    
    static void skipWhitespace(const std::string& s, size_t& pos) {
        while (pos < s.size() && (s[pos] == ' ' || s[pos] == '\n' || s[pos] == '\r' || s[pos] == '\t'))
            pos++;
    }
    
    static JSON parseValue(const std::string& s, size_t& pos) {
        skipWhitespace(s, pos);
        if (pos >= s.size()) return JSON();
        
        char c = s[pos];
        if (c == '"') return parseString(s, pos);
        if (c == '{') return parseObject(s, pos);
        if (c == '[') return parseArray(s, pos);
        if (c == 't') { pos += 4; return JSON(true); }
        if (c == 'f') { pos += 5; return JSON(false); }
        if (c == 'n') { pos += 4; return JSON(); }
        return parseNumber(s, pos);
    }
    
    static JSON parseString(const std::string& s, size_t& pos) {
        pos++; // skip opening "
        std::string result;
        while (pos < s.size() && s[pos] != '"') {
            if (s[pos] == '\\' && pos + 1 < s.size()) {
                pos++;
                switch (s[pos]) {
                    case '"': result += '"'; break;
                    case '\\': result += '\\'; break;
                    case 'n': result += '\n'; break;
                    case 't': result += '\t'; break;
                    default: result += s[pos]; break;
                }
            } else {
                result += s[pos];
            }
            pos++;
        }
        if (pos < s.size()) pos++; // skip closing "
        return JSON(result);
    }
    
    static JSON parseNumber(const std::string& s, size_t& pos) {
        size_t start = pos;
        bool isDouble = false;
        if (s[pos] == '-') pos++;
        while (pos < s.size() && s[pos] >= '0' && s[pos] <= '9') pos++;
        if (pos < s.size() && s[pos] == '.') { isDouble = true; pos++; }
        while (pos < s.size() && s[pos] >= '0' && s[pos] <= '9') pos++;
        
        std::string num = s.substr(start, pos - start);
        if (isDouble) return JSON(std::stod(num));
        return JSON(std::stoll(num));
    }
    
    static JSON parseArray(const std::string& s, size_t& pos) {
        JSON arr = JSON::array();
        pos++; // skip [
        skipWhitespace(s, pos);
        if (pos < s.size() && s[pos] == ']') { pos++; return arr; }
        
        while (pos < s.size()) {
            arr.push(parseValue(s, pos));
            skipWhitespace(s, pos);
            if (pos < s.size() && s[pos] == ',') pos++;
            else break;
        }
        if (pos < s.size() && s[pos] == ']') pos++;
        return arr;
    }
    
    static JSON parseObject(const std::string& s, size_t& pos) {
        JSON obj = JSON::object();
        pos++; // skip {
        skipWhitespace(s, pos);
        if (pos < s.size() && s[pos] == '}') { pos++; return obj; }
        
        while (pos < s.size()) {
            skipWhitespace(s, pos);
            JSON key = parseString(s, pos);
            skipWhitespace(s, pos);
            if (pos < s.size() && s[pos] == ':') pos++;
            obj[key.strVal] = parseValue(s, pos);
            skipWhitespace(s, pos);
            if (pos < s.size() && s[pos] == ',') pos++;
            else break;
        }
        if (pos < s.size() && s[pos] == '}') pos++;
        return obj;
    }
};

// ============================================================
// HTTP REQUEST/RESPONSE
// ============================================================
struct HttpRequest {
    std::string method;
    std::string path;
    std::string version;
    std::map<std::string, std::string> headers;
    std::string body;
    std::map<std::string, std::string> queryParams;
    
    static HttpRequest parse(const std::string& raw) {
        HttpRequest req;
        std::istringstream stream(raw);
        std::string line;
        
        // Request line
        if (std::getline(stream, line)) {
            std::istringstream reqLine(line);
            reqLine >> req.method >> req.path >> req.version;
            
            // Parse query params
            auto qpos = req.path.find('?');
            if (qpos != std::string::npos) {
                std::string query = req.path.substr(qpos + 1);
                req.path = req.path.substr(0, qpos);
                // Simple query parse
                std::istringstream qs(query);
                std::string pair;
                while (std::getline(qs, pair, '&')) {
                    auto eq = pair.find('=');
                    if (eq != std::string::npos) {
                        req.queryParams[pair.substr(0, eq)] = pair.substr(eq + 1);
                    }
                }
            }
        }
        
        // Headers
        while (std::getline(stream, line) && line != "\r" && !line.empty()) {
            if (line.back() == '\r') line.pop_back();
            auto colon = line.find(':');
            if (colon != std::string::npos) {
                std::string key = line.substr(0, colon);
                std::string val = line.substr(colon + 1);
                while (!val.empty() && val[0] == ' ') val.erase(0, 1);
                req.headers[key] = val;
            }
        }
        
        // Body
        std::string bodyData;
        while (std::getline(stream, line)) {
            bodyData += line + "\n";
        }
        if (!bodyData.empty()) req.body = bodyData;
        
        return req;
    }
};

struct HttpResponse {
    int statusCode = 200;
    std::string statusText = "OK";
    std::map<std::string, std::string> headers;
    std::string body;
    
    HttpResponse() {
        headers["Content-Type"] = "text/plain";
        headers["Server"] = "KAVA/2.5";
        headers["Connection"] = "close";
    }
    
    HttpResponse& status(int code, const std::string& text = "") {
        statusCode = code;
        statusText = text.empty() ? getStatusText(code) : text;
        return *this;
    }
    
    HttpResponse& json(const JSON& data) {
        headers["Content-Type"] = "application/json";
        body = data.stringify();
        return *this;
    }
    
    HttpResponse& html(const std::string& content) {
        headers["Content-Type"] = "text/html";
        body = content;
        return *this;
    }
    
    HttpResponse& text(const std::string& content) {
        headers["Content-Type"] = "text/plain";
        body = content;
        return *this;
    }
    
    std::string serialize() const {
        std::ostringstream ss;
        ss << "HTTP/1.1 " << statusCode << " " << statusText << "\r\n";
        for (auto& [k, v] : headers) {
            ss << k << ": " << v << "\r\n";
        }
        ss << "Content-Length: " << body.size() << "\r\n";
        ss << "\r\n";
        ss << body;
        return ss.str();
    }
    
    static std::string getStatusText(int code) {
        switch (code) {
            case 200: return "OK";
            case 201: return "Created";
            case 204: return "No Content";
            case 301: return "Moved Permanently";
            case 302: return "Found";
            case 400: return "Bad Request";
            case 401: return "Unauthorized";
            case 403: return "Forbidden";
            case 404: return "Not Found";
            case 500: return "Internal Server Error";
            default: return "Unknown";
        }
    }
};

// ============================================================
// HTTP ROUTE HANDLER
// ============================================================
using RouteHandler = std::function<HttpResponse(const HttpRequest&)>;

struct Route {
    std::string method;
    std::string path;
    RouteHandler handler;
};

// ============================================================
// HTTP SERVER (Nativo)
// ============================================================
class HttpServer {
public:
    int port;
    std::vector<Route> routes;
    std::atomic<bool> running{false};
    int serverFd = -1;
    
    HttpServer(int p = 8080) : port(p) {}
    
    ~HttpServer() { stop(); }
    
    void get(const std::string& path, RouteHandler handler) {
        routes.push_back({"GET", path, handler});
    }
    
    void post(const std::string& path, RouteHandler handler) {
        routes.push_back({"POST", path, handler});
    }
    
    void put(const std::string& path, RouteHandler handler) {
        routes.push_back({"PUT", path, handler});
    }
    
    void del(const std::string& path, RouteHandler handler) {
        routes.push_back({"DELETE", path, handler});
    }
    
    bool listen() {
        serverFd = socket(AF_INET, SOCK_STREAM, 0);
        if (serverFd < 0) return false;
        
        int opt = 1;
        setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        
        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(port);
        
        if (bind(serverFd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            close(serverFd);
            return false;
        }
        
        if (::listen(serverFd, 128) < 0) {
            close(serverFd);
            return false;
        }
        
        running = true;
        return true;
    }
    
    void serve() {
        while (running) {
            struct pollfd pfd;
            pfd.fd = serverFd;
            pfd.events = POLLIN;
            
            int ret = poll(&pfd, 1, 100);  // 100ms timeout
            if (ret <= 0) continue;
            
            struct sockaddr_in clientAddr;
            socklen_t clientLen = sizeof(clientAddr);
            int clientFd = accept(serverFd, (struct sockaddr*)&clientAddr, &clientLen);
            if (clientFd < 0) continue;
            
            handleConnection(clientFd);
        }
    }
    
    void serveAsync(EventLoop& loop) {
        loop.queueIO([this]() {
            serve();
        });
    }
    
    void stop() {
        running = false;
        if (serverFd >= 0) {
            close(serverFd);
            serverFd = -1;
        }
    }

private:
    void handleConnection(int clientFd) {
        char buffer[8192];
        ssize_t bytesRead = read(clientFd, buffer, sizeof(buffer) - 1);
        if (bytesRead <= 0) {
            close(clientFd);
            return;
        }
        buffer[bytesRead] = '\0';
        
        HttpRequest req = HttpRequest::parse(std::string(buffer, bytesRead));
        HttpResponse resp = route(req);
        
        std::string response = resp.serialize();
        write(clientFd, response.c_str(), response.size());
        close(clientFd);
    }
    
    HttpResponse route(const HttpRequest& req) {
        for (auto& r : routes) {
            if (r.method == req.method && matchPath(r.path, req.path)) {
                return r.handler(req);
            }
        }
        
        HttpResponse resp;
        resp.status(404).text("Not Found: " + req.path);
        return resp;
    }
    
    bool matchPath(const std::string& pattern, const std::string& path) {
        if (pattern == path) return true;
        if (pattern == "*") return true;
        // Simple wildcard: /api/* matches /api/anything
        if (pattern.back() == '*') {
            return path.substr(0, pattern.size() - 1) == pattern.substr(0, pattern.size() - 1);
        }
        return false;
    }
};

// ============================================================
// FILE SYSTEM (Async)
// ============================================================
class FileSystem {
public:
    static std::string readFileSync(const std::string& path) {
        std::ifstream file(path);
        if (!file) return "";
        std::stringstream ss;
        ss << file.rdbuf();
        return ss.str();
    }
    
    static bool writeFileSync(const std::string& path, const std::string& content) {
        std::ofstream file(path);
        if (!file) return false;
        file << content;
        return true;
    }
    
    static bool existsSync(const std::string& path) {
        std::ifstream file(path);
        return file.good();
    }
    
    static void readFile(const std::string& path, EventLoop& loop,
                         std::function<void(const std::string&)> callback) {
        loop.queueIO([path, &loop, callback]() {
            std::string content = readFileSync(path);
            loop.completeIO([callback, content]() {
                callback(content);
            });
        });
    }
    
    static void writeFile(const std::string& path, const std::string& content,
                          EventLoop& loop, std::function<void(bool)> callback) {
        loop.queueIO([path, content, &loop, callback]() {
            bool success = writeFileSync(path, content);
            loop.completeIO([callback, success]() {
                callback(success);
            });
        });
    }
};

// ============================================================
// TIME UTILITIES
// ============================================================
class Time {
public:
    static int64_t nowMs() {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
    }
    
    static int64_t nowNs() {
        return std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch()).count();
    }
    
    static void sleep(int64_t ms) {
        std::this_thread::sleep_for(std::chrono::milliseconds(ms));
    }
};

// ============================================================
// MATH EXTENDED
// ============================================================
class MathExt {
public:
    static constexpr double PI = 3.14159265358979323846;
    static constexpr double E = 2.71828182845904523536;
    
    static double random() {
        static std::mt19937 gen(std::chrono::steady_clock::now().time_since_epoch().count());
        static std::uniform_real_distribution<double> dist(0.0, 1.0);
        return dist(gen);
    }
    
    static int randomInt(int min, int max) {
        return min + static_cast<int>(random() * (max - min + 1));
    }
};

// ============================================================
// NET (TCP) 
// ============================================================
class TcpSocket {
public:
    int fd = -1;
    
    bool connect(const std::string& host, int port) {
        fd = socket(AF_INET, SOCK_STREAM, 0);
        if (fd < 0) return false;
        
        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        inet_pton(AF_INET, host.c_str(), &addr.sin_addr);
        
        return ::connect(fd, (struct sockaddr*)&addr, sizeof(addr)) == 0;
    }
    
    ssize_t send(const std::string& data) {
        return write(fd, data.c_str(), data.size());
    }
    
    std::string recv(int maxBytes = 4096) {
        char buf[4096];
        ssize_t n = read(fd, buf, std::min(maxBytes, 4096));
        if (n <= 0) return "";
        return std::string(buf, n);
    }
    
    void close() {
        if (fd >= 0) { ::close(fd); fd = -1; }
    }
    
    ~TcpSocket() { close(); }
};

} // namespace Kava

#endif // KAVA_RUNTIME_H
