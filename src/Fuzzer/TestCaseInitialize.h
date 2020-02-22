#ifndef CAF_TEST_CASE_INITIALIZE_H
#define CAF_TEST_CASE_INITIALIZE_H

namespace caf {

class CAFCorpus;

/**
 * @brief Load seed test cases to the corpus.
 *
 * Seed test cases are stored under the directory whose path is given by the environment variable
 * `CAF_SEED_DIR`. All files in this directory will be loaded as a test case independently.
 *
 * This function will kill the program if the `CAF_SEED_DIR` environment variable is not found or
 * its content does not name a valid directory in the file system.
 *
 * @param corpus the corpus.
 */
void LoadSeeds(CAFCorpus& corpus);

} // namespace caf

#endif
