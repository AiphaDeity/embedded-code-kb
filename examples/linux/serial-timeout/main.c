#define _DEFAULT_SOURCE

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <stdint.h>
#include <stdio.h>
#include <termios.h>
#include <unistd.h>

static int read_once(int fd)
{
  // 1. 等到有数据或 1 秒窗口结束。断开、超时和错误分别返回。
  struct pollfd port = {.fd = fd, .events = POLLIN};
  const int ready = poll(&port, 1, 1000);
  if (ready < 0) { perror("poll"); return 1; }
  if (ready == 0) { puts("timeout"); return 2; }
  if (!(port.revents & POLLIN))
  {
    fprintf(stderr, "serial is unavailable: revents=0x%x\n", (unsigned)port.revents);
    return 1;
  }

  // 2. 有可读事件再读取。把本次拿到的字节按长度打印。
  uint8_t data[64];
  const ssize_t length = read(fd, data, sizeof(data));
  if (length < 0) { perror("read"); return 1; }
  if (length == 0) { fputs("serial closed\n", stderr); return 1; }
  for (ssize_t i = 0; i < length; ++i)
  {
    printf("%02X%s", (unsigned)data[i], i + 1 == length ? "\n" : " ");
  }
  return 0;
}

int main(int argc, char **argv)
{
  if (argc != 2)
  {
    fprintf(stderr, "usage: %s /dev/ttyUSB0\n", argv[0]);
    return 1;
  }
  // 1. 非阻塞打开，避免驱动在 open 阶段等待载波。保留旧配置供退出时恢复。
  const int fd = open(argv[1], O_RDWR | O_NOCTTY | O_NONBLOCK | O_CLOEXEC);
  if (fd < 0) { perror("open"); return 1; }
  struct termios original;
  if (tcgetattr(fd, &original) < 0)
  {
    perror("tcgetattr");
    close(fd);
    return 1;
  }

  // 2. 配置原始 115200 8N1，并关闭软硬件流控。
  struct termios options = original;
  cfmakeraw(&options);
  options.c_cflag = (options.c_cflag & ~(CSIZE | PARENB | CSTOPB | CRTSCTS)) | CS8 | CLOCAL | CREAD;
  options.c_iflag &= ~(IXON | IXOFF | IXANY);
  options.c_cc[VMIN] = 1;
  options.c_cc[VTIME] = 0;
  if (cfsetispeed(&options, B115200) < 0 || cfsetospeed(&options, B115200) < 0 ||
      tcsetattr(fd, TCSANOW, &options) < 0)
  {
    perror("configure serial");
    close(fd);
    return 1;
  }

  // 3. 读取结束后恢复原有配置，再关闭文件描述符。
  int result = read_once(fd);
  if (tcsetattr(fd, TCSANOW, &original) < 0)
  {
    perror("restore serial");
    result = 1;
  }
  close(fd);
  return result;
}
