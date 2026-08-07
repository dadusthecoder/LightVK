#pragma once

#if defined(LIGHTVK_EDITOR_TESTS)

#include <cstdint>
#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace Lgt {

class World;

namespace Editor::Tests {

struct TestResult {
    bool        passed = false;
    std::string message;

    static TestResult Pass(std::string message = {}) {
        return {true, std::move(message)};
    }

    static TestResult Fail(std::string message) {
        return {false, std::move(message)};
    }
};

using TestFunction   = std::function<TestResult()>;
using CleanupFunction = std::function<void()>;

class EditorTestRunner {
public:
    void Init(World* world);
    void DrawUi();
    void Shutdown();

private:
    struct TestEntry {
        std::string     name;
        std::string     description;
        TestFunction    run;
        CleanupFunction cleanup;
        TestResult      lastResult;
        uint32_t        runCount = 0;
    };

    void RegisterBuiltInTests();
    void RegisterTest(std::string name,
                      std::string description,
                      TestFunction run,
                      CleanupFunction cleanup = {});
    void RunTest(TestEntry& test);

    World*              _world = nullptr;
    std::vector<TestEntry> _tests;
};

} // namespace Editor::Tests
} // namespace Lgt

#endif // LIGHTVK_EDITOR_TESTS
