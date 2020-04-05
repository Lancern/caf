#include "Command.h"
#include "Diagnostics.h"
#include "RegisterCommand.h"

#include "Infrastructure/Stream.h"

#include "Basic/CAFStore.h"
#include "Fuzzer/TestCase.h"
#include "Fuzzer/ObjectPool.h"
#include "Fuzzer/TestCaseDeserializer.h"
#include "Fuzzer/TestCaseSynthesiser.h"
#include "Fuzzer/SynthesisBuilder.h"
#include "Fuzzer/JavaScriptSynthesisBuilder.h"
#include "Fuzzer/NodejsSynthesisBuilder.h"

#include "json/json.hpp"

#include <unordered_map>

namespace caf {

class SynthesisCommand : public Command {
public:
  explicit SynthesisCommand() = default;

  void SetupArgs(CLI::App &app) override {
    app.add_option("-s", _opts.CAFStorePath, "Path to the cafstore.json file");
    app.add_option("-t,--target", _opts.Target, "Name of the target. Available targets: js, nodejs")
        ->default_val("js");
    app.add_option("tc", _opts.TestCasePath, "Path to the test case file");

    InitializeSynthesisers();
  }

  int Execute(CLI::App &app) override {
    // Check target name.
    if (Synthesisers.find(_opts.Target) != Synthesisers.end()) {
      PRINT_ERR_AND_EXIT_FMT("invalid target: %s", _opts.Target.c_str());
    }

    std::ifstream storeFile { _opts.CAFStorePath };
    if (storeFile.fail()) {
      PRINT_LAST_OS_ERR_AND_EXIT("failed to load CAF store file");
    }

    nlohmann::json json;
    storeFile >> json;
    auto store = caf::make_unique<CAFStore>(json);
    auto pool = caf::make_unique<ObjectPool>();
    storeFile.close();

    std::ifstream tcFile { _opts.TestCasePath };
    if (tcFile.fail()) {
      PRINT_LAST_OS_ERR_AND_EXIT("failed to load test case file");
    }
    StlInputStream fileStream { tcFile };
    TestCaseDeserializer de { *pool, fileStream };
    auto tc = de.Deserialize();

    auto& builder = *Synthesisers[_opts.Target];
    TestCaseSynthesiser syn { *store, builder };
    syn.Synthesis(tc);

    auto code = syn.GetCode();
    std::cout << code << std::endl;

    return 0;
  }

private:
  static void InitializeSynthesisers() {

  }

  static std::unordered_map<std::string, std::unique_ptr<SynthesisBuilder>> Synthesisers;

  struct Opts {
    std::string CAFStorePath;
    std::string Target;
    std::string TestCasePath;
  }; // struct Opts

  Opts _opts;
};

std::unordered_map<std::string, std::unique_ptr<SynthesisBuilder>> SynthesisCommand::Synthesisers;

static RegisterCommand<SynthesisCommand> X {
    "synthesis", "Synthesis a test case to script form" };

} // namespace caf
