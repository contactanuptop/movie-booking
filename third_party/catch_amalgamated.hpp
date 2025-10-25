#pragma once
#include <iostream>
#include <vector>
#include <functional>
#include <string>
#include <stdexcept>

// -----------------------------------------------------------------------------
// Minimal Catch-like testing framework (header-only, no deps)
// -----------------------------------------------------------------------------
namespace mini_catch {

struct TestCase {
    std::string name;
    std::function<void()> func;
};

inline std::vector<TestCase>& registry() {
    static std::vector<TestCase> tests;
    return tests;
}

struct Registrar {
    Registrar(const std::string& name, std::function<void()> func) {
        registry().push_back({name, std::move(func)});
    }
};

inline int run_all_tests(const std::string& filter = "") {
    int failed = 0;
    int executed = 0;
    for (auto& t : registry()) {
        if (!filter.empty() && t.name.find(filter) == std::string::npos)
            continue; // skip non-matching tests
        executed++;
        try {
            t.func();
            std::cout << "\033[32m" << t.name << " PASSED\033[0m\n";
        } catch (const std::exception& e) {
            ++failed;
            std::cout << "\033[31m" << t.name << " FAILED: " << e.what() << "\033[0m\n";
        } catch (...) {
            ++failed;
            std::cout << "\033[31m" << t.name << " FAILED (unknown error)\033[0m\n";
        }
    }

    //std::cout << "\nTotal: " << registry().size()
    std::cout << "\nExecuted: " << executed
              << " | Failed: " << failed << std::endl;
    return failed;
}

inline void require(bool condition, const char* expr, const char* file, int line) {
    if (!condition) {
        std::string msg = std::string("Requirement failed: ") + expr +
                          " at " + file + ":" + std::to_string(line);
        throw std::runtime_error(msg);
    }
}

template <typename Ex, typename F>
void require_throws_as(F&& f, const char* expr, const char* ex, const char* file, int line) {
    try {
        f();
    } catch (const Ex&) {
        return; // success
    } catch (...) {
        throw std::runtime_error(std::string("Expected exception of type ") + ex +
                                 " but got something else at " + file + ":" + std::to_string(line));
    }
    throw std::runtime_error(std::string("Expected exception of type ") + ex +
                             " but no exception thrown for " + expr);
}

} // namespace mini_catch

// -----------------------------------------------------------------------------
// Macros for unique test names using __LINE__ expansion
// -----------------------------------------------------------------------------
#define CONCAT_IMPL(x, y) x##y
#define CONCAT(x, y) CONCAT_IMPL(x, y)

#define TEST_CASE(name)                                                       \
    static void CONCAT(test_func_, __LINE__)();                               \
    static ::mini_catch::Registrar CONCAT(test_reg_, __LINE__)(name, CONCAT(test_func_, __LINE__)); \
    static void CONCAT(test_func_, __LINE__)()

#define REQUIRE(expr) ::mini_catch::require((expr), #expr, __FILE__, __LINE__)
#define REQUIRE_THROWS_AS(expr, ex) \
    ::mini_catch::require_throws_as<ex>([&](){ (void)(expr); }, #expr, #ex, __FILE__, __LINE__)

// -----------------------------------------------------------------------------
// Main entry point (only if not provided elsewhere)
// -----------------------------------------------------------------------------
#ifdef MINI_CATCH_MAIN
#include "args/args.hpp"

// Global variable accessible from tests
inline int g_thread_count = 0;

int main(int argc, char** argv) {
    args::ArgumentParser parser("MiniCatch - Lightweight Test Runner");
    args::HelpFlag help(parser, "help", "Display this help menu", {'h', "help"});
    args::ValueFlag<std::string> filterArg(parser, "filter", "Run only tests matching this substring", {'f', "filter"});
    args::ValueFlag<int> threadsArg(parser, "threads", "Number of threads for concurrency tests", {'t', "threads"}, 0);

    try {
        parser.ParseCLI(argc, argv);
    } catch (const args::Help&) {
        std::cout << parser;
        return 0;
    } catch (const args::ParseError &e) {
        std::cerr << e.what() << std::endl << parser;
        return 1;
    }

    std::string filter = filterArg ? args::get(filterArg) : "";
    g_thread_count = threadsArg ? args::get(threadsArg) : 0;

    return ::mini_catch::run_all_tests(filter);
}
#endif