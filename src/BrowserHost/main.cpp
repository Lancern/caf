#include <cerrno>
#include <cstdlib>
#include <cstdio>
#include <cstring>

#include <unordered_set>

#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define PRINT_ERR_AND_EXIT(msg) \
  do { std::fprintf(stderr, "error: " msg); std::exit(1); } while (false)

#define PRINT_ERR_AND_EXIT_FMT(msg, ...) \
  do { std::fprintf(stderr, "error: " msg, __VA_ARGS__); std::exit(1); } while (false)

#define PRINT_ERRNO_AND_EXIT(msg) \
  PRINT_ERR_AND_EXIT_FMT(msg ": %s (%d)", std::strerror(errno), errno)


#define IS_CLONE_PTRACE(status) \
  (((status) >> 8) == (SIGTRAP | (PTRACE_EVENT_CLONE << 8)))

#define IS_FORK_PTRACE(status) \
  (((status) >> 8) == (SIGTRAP | (PTRACE_EVENT_FORK << 8)))

#define IS_VFORK_PTRACE(status) \
  (((status) >> 8) == (SIGTRAP | (PTRACE_EVENT_VFORK << 8)))


namespace {

template <typename TAddr, typename TData>
long call_ptrace(enum __ptrace_request request, pid_t pid, TAddr addr, TData data) {
  auto ret = ptrace(request, pid, (void *)(intptr_t)addr, (void *)(intptr_t)data);
  if (ret == -1) {
    PRINT_ERRNO_AND_EXIT("ptrace failed");
  }
  return ret;
}

pid_t call_wait(int* wstatus) {
  auto ret = wait(wstatus);
  if (ret == -1) {
    PRINT_ERRNO_AND_EXIT("wait failed");
  }
  return ret;
}

template <typename Container, typename T>
bool contains(const Container& container, const T& value) {
  return container.find(value) != container.end();
}

} // namespace <anonymous>

int main(int argc, char* argv[]) {
  if (argc <= 1) {
    PRINT_ERR_AND_EXIT("invalid arguments");
  }

  auto childPid = fork();
  if (childPid == 0) {
    // In child process.
    // Issue traceme and execute the child program.
    call_ptrace(PTRACE_TRACEME, 0, nullptr, nullptr);
    execvp(argv[1], argv + 1);
    PRINT_ERR_AND_EXIT("exec failed");
  }

  std::unordered_set<pid_t> pendingChildren { childPid };

  while (true) {
    printf("childPid = %d\n", childPid);

    int wstatus;
    auto pid = call_wait(&wstatus);

    if (contains(pendingChildren, pid)) {
      pendingChildren.erase(pid);
      call_ptrace(PTRACE_SETOPTIONS, pid, 0,
          PTRACE_O_EXITKILL | PTRACE_O_TRACECLONE | PTRACE_O_TRACEFORK | PTRACE_O_TRACEVFORK);
      call_ptrace(PTRACE_CONT, pid, 0, 0);
      printf("Started: %d\n", pid);
    } else {
      if (IS_CLONE_PTRACE(wstatus) || IS_FORK_PTRACE(wstatus) || IS_VFORK_PTRACE(wstatus)) {
        pid_t newPid;
        call_ptrace(PTRACE_GETEVENTMSG, pid, 0, &newPid);
        pendingChildren.insert(newPid);
        call_ptrace(PTRACE_CONT, pid, 0, 0);
        printf("New process: %d\n", newPid);
      } else if (WIFEXITED(wstatus) || WIFSIGNALED(wstatus)) {
        if (WIFEXITED(wstatus)) {
          printf("Exited: %d\n", pid);
        } else {
          // WIFSIGNALED(wstatus)
          printf("Signaled: %d\n", pid);
        }
        if (pid == childPid) {
          // The master process has exited.
          break;
        }
      } else {
        call_ptrace(PTRACE_CONT, pid, 0, 0);
      }
    }
  }

  return 0;
}
