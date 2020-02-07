#ifndef CAF_CORPUS_INITIALIZE_H
#define CAF_CORPUS_INITIALIZE_H

#include <memory>

namespace caf {

class CAFCorpus;

/**
 * @brief Initialize corpus and seeds from settings provided by environment variables.
 *
 * This function consumes the following environment variables:
 * * `CAF_STORE`: Path to the JSON file containing CAF store definitions.
 * * `CAF_CORPUS`: Path to the directory containing test case seeds.
 *
 */
std::unique_ptr<CAFCorpus> InitCorpus();

} // namespace caf

#endif
