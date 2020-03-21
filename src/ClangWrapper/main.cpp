#include <unistd.h>

#include <cerrno>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

#ifdef CAF_CLANG_CXX
#define GCLANG_NAME "gclang++"
#else
#define GCLANG_NAME "gclang"
#endif

#define AFL_CLANG_NAME "afl-clang-fast"
#define AFL_CLANG_CXX_NAME "afl-clang-fast++"

namespace {

char* DuplicateString(const char* s) {
  auto len = std::strlen(s);
  auto buffer = new char[len + 1];
  std::strcpy(buffer, s);
  return buffer;
}

/**
 * @brief Locate an executable file with the given name in the given directory.
 *
 * @param name the name of the executable.
 * @param dir the directory to search.
 * @return std::string path to the executable found, or empty string if the executable cannot be
 * found in the given directory.
 */
std::string LocateExecutableWithin(const char* name, const char* dir) {
  std::string path(dir);
  if (!path.empty() && path[path.size() - 1] != '/') {
    path.push_back('/');
  }
  path.append(name);

  if (access(path.c_str(), X_OK) == 0) {
    return path;
  } else {
    return std::string();
  }
}

/**
 * @brief Locate an executable file with the given name within the directory pointed to by the
 * environment variable of the given name. If the executable cannot be located there, then the
 * executable will be further searched along the PATH environment variable.
 *
 * @param name the name of the executable.
 * @param envName the name of the environment variable that points to the directory to be searched
 * first.
 * @param searchPath should this function search the directories pointed to by the PATH environment
 * variable for the desired executable if the executable cannot be found in the first directory?
 * @return std::string path to the located executable file, or empty string if the desired
 * executable cannot be located.
 */
std::string LocateExecutable(const char* name, const char* envName, bool searchPath = true) {
  auto dir = std::getenv(envName);
  if (dir) {
    auto path = LocateExecutableWithin(name, dir);
    if (!path.empty()) {
      return path;
    }
  }

  if (!searchPath) {
    return std::string();
  }

  auto pathsEnv = std::getenv("PATH");
  if (!pathsEnv) {
    return std::string();
  }

  auto paths = DuplicateString(pathsEnv);
  std::string result;

  dir = std::strtok(paths, ":");
  while (dir) {
    auto path = LocateExecutableWithin(name, dir);
    if (!path.empty()) {
      result = std::move(path);
      break;
    }
    dir = std::strtok(nullptr, ":");
  }

  delete[] paths;
  return result;
}

} // namespace <anonymous>

void PrintUsage(int argc, char* argv[]) {
  auto usage =
    "USAGE: %s params...\n"
    "\n"
    "The following environment variables can be set to customize the toolchain:\n"
    "  GCLANG_PATH - Path to the directory containing gclang or gclang++\n"
    "  AFL_PATH - Path to the directory containing AFL++ binaries\n"
    "  LLVM_CONFIG - Path to the llvm-config utility corresponding to the desired\n"
    "                LLVM version\n"
    "\n"
    "If some of the environment variables above are not set, then the corresponding \n"
    "tool will be located from the PATH variable.\n";
  fprintf(stderr, usage, argv[0]);
}

int main(int argc, char* argv[]) {
  if (argc == 2 && (std::strcmp(argv[1], "-h") == 0 || std::strcmp(argv[1], "--help") == 0)) {
    PrintUsage(argc, argv);
    return 0;
  }

  // Locate gclang or gclang++.
  auto gclangPath = LocateExecutable(GCLANG_NAME, "GCLANG_PATH");
  if (gclangPath.empty()) {
    std::cerr << "Cannot locate " GCLANG_NAME << std::endl;
    std::cerr << "Please ensure GCLANG_PATH is properly set." << std::endl;
    return 1;
  }

  // Prepare arguments to gclang or gclang++.
  std::vector<char *> args;
  args.push_back(DuplicateString(gclangPath.c_str()));
  for (auto i = 1; i < argc; ++i) {
    args.push_back(argv[i]);
  }
  args.push_back(nullptr);

  // Prepare environment variables to gclang or gclang++.
  std::vector<char *> envs;
  for (auto e = environ; *e; ++e) {
    envs.push_back(*e);
  }

  // export LLVM_COMPILER_PATH=AFL_PATH
  // export LLVM_CC_NAME=afl-clang-fast
  // export LLVM_CXX_NAME=afl-clang-fast++
  auto aflPath = std::getenv("AFL_PATH");
  if (aflPath) {
    std::string compilerPathEnv("LLVM_COMPILER_PATH=");
    compilerPathEnv.append(aflPath);
    envs.push_back(DuplicateString(compilerPathEnv.c_str()));
  }

  envs.push_back(DuplicateString("LLVM_CC_NAME=" AFL_CLANG_NAME));
  envs.push_back(DuplicateString("LLVM_CXX_NAME=" AFL_CLANG_CXX_NAME));
  envs.push_back(nullptr);

  execve(gclangPath.c_str(), args.data(), envs.data());
  auto errCode = errno;
  auto errMessage = std::strerror(errCode);
  std::cerr << "exec failed: " << errMessage << " (" << errCode << ")" << std::endl;

  return 1;
}
