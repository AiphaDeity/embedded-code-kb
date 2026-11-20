#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

enum { REPLY_TIMEOUT_MS = 20, MAX_ATTEMPTS = 3 };

typedef enum { READY, WAIT_REPLY, SUCCEEDED, FAILED } State;
typedef enum { NOTHING, SEND_REQUEST, REPORT_OK, REPORT_TIMEOUT } Action;
typedef struct
{
  State state;
  uint32_t started_at;
  unsigned attempts;
} Command;

// 每轮传入当前毫秒计数和本命令匹配的应答。SEND_REQUEST 交给调用者立刻发出。
// 两次调用间隔须小于计数完整回绕周期，且应答要由上层按本命令归属筛选。
static Action command_step(Command *command, uint32_t now, bool reply)
{
  // 1. 已经报告结果的命令保持终态，由调用者另建命令开始下一轮。
  if (command->state == SUCCEEDED || command->state == FAILED)
  {
    return NOTHING;
  }

  // 2. 等待期间先判断是否到期。窗口内收到应答就交给调用者报告成功。
  if (command->state == WAIT_REPLY &&
      (uint32_t)(now - command->started_at) < REPLY_TIMEOUT_MS)
  {
    if (reply)
    {
      command->state = SUCCEEDED;
      return REPORT_OK;
    }
    return NOTHING;
  }

  // 3. 首次调用或本次到期时才发起发送。次数用尽就报告超时，不再重发。
  if (command->attempts == MAX_ATTEMPTS)
  {
    command->state = FAILED;
    return REPORT_TIMEOUT;
  }
  ++command->attempts;
  command->started_at = now;
  command->state = WAIT_REPLY;
  return SEND_REQUEST;
}

int main(void)
{
  // 1. 计数在第一次发送后回绕，下一次轮询仍按无符号差计算经过时间。
  Command command = {.state = READY};
  assert(command_step(&command, UINT32_MAX - 9U, false) == SEND_REQUEST);
  assert(command_step(&command, 9U, false) == NOTHING);
  assert(command_step(&command, 10U, true) == SEND_REQUEST);
  assert(command_step(&command, 11U, true) == REPORT_OK);
  assert(command.attempts == 2U);
  assert(command_step(&command, 12U, true) == NOTHING);

  // 2. 另一条命令始终收不到应答。到达上限后只上报一次，后续轮询返回无动作。
  Command silent = {.state = READY};
  assert(command_step(&silent, 0U, false) == SEND_REQUEST);
  assert(command_step(&silent, 20U, false) == SEND_REQUEST);
  assert(command_step(&silent, 40U, false) == SEND_REQUEST);
  assert(command_step(&silent, 60U, false) == REPORT_TIMEOUT);
  assert(command_step(&silent, 61U, true) == NOTHING);
  puts("timed retry: wrap, reply, 3 attempts, single timeout");
  return 0;
}
