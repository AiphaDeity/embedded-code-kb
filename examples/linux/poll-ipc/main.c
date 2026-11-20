#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

// 子进程用数据报送读数，用管道报告状态。
static void report(int data_fd, int status_fd)
{
  const char reading = 42;
  const char done = 'D';
  const int success = send(data_fd, &reading, 1U, 0) == 1 &&
                      write(status_fd, &done, 1U) == 1;
  close(data_fd);
  close(status_fd);
  _exit(success ? EXIT_SUCCESS : EXIT_FAILURE);
}

static int collect(int data_fd, int status_fd)
{
  struct pollfd inputs[] = {{.fd = data_fd, .events = POLLIN},
                            {.fd = status_fd, .events = POLLIN}};
  unsigned remaining = 2;
  while (remaining != 0U)
  {
    // 1. 一个等待同时覆盖管道和 Unix 域套接字，等待失败就报告。
    const int result = poll(inputs, 2, 1000);
    if (result < 0) { perror("poll"); return EXIT_FAILURE; }
    if (result == 0) { fputs("poll timeout\n", stderr); return EXIT_FAILURE; }

    for (unsigned i = 0; i < 2U; ++i)
    {
      // 2. 即使同时到达 hangup，仍先消费已排队的数据。
      if (inputs[i].revents & POLLIN)
      {
        unsigned char byte;
        if (read(inputs[i].fd, &byte, 1U) != 1)
        {
          fputs("incomplete IPC message\n", stderr);
          return EXIT_FAILURE;
        }
        if (i == 0U) { printf("reading=%u\n", (unsigned)byte); }
        else { printf("status=%c\n", byte); }
        inputs[i].fd = -1;
        --remaining;
      }
      else if (inputs[i].revents & (POLLERR | POLLHUP | POLLNVAL))
      {
        fputs("IPC endpoint closed early\n", stderr);
        return EXIT_FAILURE;
      }
    }
  }
  return EXIT_SUCCESS;
}

int main(void)
{
  // 1. 先创建两个通道，随后 fork。两进程关闭不再使用的端点。
  int data[2], state[2];
  if (socketpair(AF_UNIX, SOCK_DGRAM, 0, data) < 0)
  {
    perror("socketpair");
    return EXIT_FAILURE;
  }
  if (pipe(state) < 0)
  {
    perror("pipe"); close(data[0]); close(data[1]);
    return EXIT_FAILURE;
  }
  const pid_t child = fork();
  if (child < 0)
  {
    perror("fork");
    close(data[0]); close(data[1]); close(state[0]); close(state[1]);
    return EXIT_FAILURE;
  }
  if (child == 0)
  {
    close(data[0]); close(state[0]);
    report(data[1], state[1]);
  }

  // 2. 父进程同时监听两类输入，最后回收子进程并检查退出状态。
  close(data[1]); close(state[1]);
  const int result = collect(data[0], state[0]);
  close(data[0]); close(state[0]);
  int status;
  pid_t waited;
  do { waited = waitpid(child, &status, 0); } while (waited < 0 && errno == EINTR);
  if (waited < 0) { perror("waitpid"); return EXIT_FAILURE; }
  return WIFEXITED(status) && WEXITSTATUS(status) == EXIT_SUCCESS ? result : EXIT_FAILURE;
}
