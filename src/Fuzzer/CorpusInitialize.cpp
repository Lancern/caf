#include "CorpusInitialize.h"
#include "Basic/CAFStore.h"
#include "Basic/JsonDeserializer.h"
#include "Fuzzer/Corpus.h"

#include "json/json.hpp"

#include <cassert>
#include <cstdlib>
#include <fstream>

namespace caf {

namespace {

/**
 * @brief Load CAFStore instances from settings given by environment variables.
 *
 * This function consumes the following environment variables:
 * * `CAF_STORE`: Path to the JSON file containinng CAFStore definitions.
 *
 * @return std::unique_ptr<CAFStore> an owning pointer to the CAFStore instance loaded.
 */
std::unique_ptr<CAFStore> InitStore() {
  auto storeFileName = getenv("CAF_STORE");
  assert(storeFileName && "CAF_STORE environment variable is not set.");

  std::ifstream file { storeFileName };
  nlohmann::json rawJson;
  file >> rawJson;
  file.close();

  caf::JsonDeserializer json;
  return json.DeserializeCAFStore(rawJson);
}

} // namespace <anonymous>

std::unique_ptr<CAFCorpus> InitCorpus() {
  auto store = InitStore();

  // TODO: Load test case seeds and then build corpus to return.

  return nullptr;
}

} // namespace caf
