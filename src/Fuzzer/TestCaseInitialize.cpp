#include "TestCaseInitialize.h"
#include "Fuzzer/TestCaseDeserializer.h"

#include <ftw.h>

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <fstream>

#define NORETURN __attribute__((noreturn))

#define CAF_SEED_DIR_ENV "CAF_SEED_DIR"

namespace caf {

namespace {

NORETURN
void FatalError(const char* msg) {
  std::cerr << "CAF fatal error: " << msg << std::endl;
  std::exit(1);
}

NORETURN
void FatalError(const char* msg, int errCode) {
  std::cerr << "CAF fatal error: " << msg << ": "
            << std::strerror(errCode) << "(" << errCode << ")"
            << std::endl;
  std::exit(errCode);
}

TestCaseDeserializer* tcReader;

class StdInputStreamAdapter {
public:
  explicit StdInputStreamAdapter(std::istream* inner)
    : _inner(inner)
  { }

  void read(void* buffer, size_t size) {
    _inner->read(reinterpret_cast<typename std::istream::char_type *>(buffer), size);
  }

private:
  std::istream* _inner;
}; // class StdInputStreamAdapter

void LoadSeedFromFile(const char* path) {
  std::ifstream stream { path };
  if (stream.fail()) {
    FatalError("Failed to open seed file", errno);
  }

  StdInputStreamAdapter adapter { &stream };
  tcReader->Read(adapter);
}

extern "C" {

int HandleSeedDirEntry(const char* path, const struct stat* sb, int typeflag) {
  if (typeflag == FTW_F) {
    LoadSeedFromFile(path);
  }
  return 0;
}

} // extern "C"

} // namespace <anonymous>

void LoadSeeds(CAFCorpus& corpus) {
  const char* seedDir = std::getenv(CAF_SEED_DIR_ENV);
  if (!seedDir) {
    FatalError("The " CAF_SEED_DIR_ENV " environment variable is not found.");
  }

  TestCaseDeserializer deserializer { &corpus };
  tcReader = &deserializer;

  auto ret = ftw(seedDir, &HandleSeedDirEntry, 4);
  if (ret != 0) {
    if (errno) {
      FatalError("Failed to walk seed directory", errno);
    } else {
      FatalError("Failed to walk seed directory");
    }
  }
}

} // namespace caf
