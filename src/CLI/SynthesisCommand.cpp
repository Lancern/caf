#include "Command.h"
#include "Diagnostics.h"
#include "RegisterCommand.h"

#include "Infrastructure/Stream.h"
#include "Infrastructure/Memory.h"

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
    app.add_option("-s", _opts.CAFStorePath, "Path to the cafstore.json file")
        ->required();
    app.add_option("-t,--target", _opts.Target, "Name of the target. Available targets: js, nodejs")
        ->default_val("js");
    app.add_option("tc", _opts.TestCasePath, "Path to the test case file")
        ->required();
  }

  int Execute(CLI::App &app) override {
    // Check target name.
    if (_opts.Target != "js" && _opts.Target != "nodejs") {
      PRINT_ERR_AND_EXIT_FMT("invalid target: %s", _opts.Target.c_str());
    }

    std::ifstream storeFile { _opts.CAFStorePath };
    if (storeFile.fail()) {
      PRINT_LAST_OS_ERR_AND_EXIT("failed to load CAF store file");
    }

    nlohmann::json json;
    storeFile >> json;
    auto store = caf::make_unique<CAFStore>();
    store->Load(json);
    auto pool = caf::make_unique<ObjectPool>();
    storeFile.close();

    std::ifstream tcFile { _opts.TestCasePath };
    if (tcFile.fail()) {
      PRINT_LAST_OS_ERR_AND_EXIT("failed to load test case file");
    }
    StlInputStream fileStream { tcFile };
    TestCaseDeserializer de { *pool, fileStream };
    auto tc = de.Deserialize();

    std::unique_ptr<SynthesisBuilder> synthesisBuilder;
    if (_opts.Target == "js") {
      synthesisBuilder = caf::make_unique<JavaScriptSynthesisBuilder>(*store);
    } else {
      // Target is nodejs.
      synthesisBuilder = caf::make_unique<NodejsSynthesisBuilder>(*store);
    }

    TestCaseSynthesiser syn { *store, *synthesisBuilder };
    syn.Synthesis(tc);

    auto code = syn.GetCode();
    std::cout << code << std::endl;

    return 0;
  }

private:
  struct Opts {
    std::string CAFStorePath;
    std::string Target;
    std::string TestCasePath;
  }; // struct Opts

  Opts _opts;
};

static RegisterCommand<SynthesisCommand> X {
    "synthesis", "Synthesis a test case to script form" };

} // namespace caf
