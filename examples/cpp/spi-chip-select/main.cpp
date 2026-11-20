#include <cassert>
#include <cstdint>
#include <cstdio>

class SpiPort
{
public:
  virtual void select(bool active) noexcept = 0;
  virtual bool exchange(std::uint8_t output, std::uint8_t &input) noexcept = 0;
  virtual ~SpiPort() = default;
};

// 阻塞式传输期间独占片选。对象存在时片选有效，作用域结束时释放片选。
class ChipSelect
{
public:
  explicit ChipSelect(SpiPort &port) noexcept : port_(port)
  {
    port_.select(true);
  }

  ~ChipSelect()
  {
    port_.select(false);
  }

  ChipSelect(const ChipSelect &) = delete;
  ChipSelect &operator=(const ChipSelect &) = delete;

private:
  SpiPort &port_;
};

// 成功后把一个响应字节写给调用者。任意一次收发失败时 output 保持原值。
static bool read_status(SpiPort &port, std::uint8_t &output) noexcept
{
  // 1. 先拉低外部设备片选，让命令和回读发生在同一段事务中。
  const ChipSelect transaction(port);
  std::uint8_t response = 0;

  // 2. 命令失败时立刻退回。成功则送一个占位字节，用这八个时钟读回状态。
  if (!port.exchange(0x05U, response) || !port.exchange(0xFFU, response))
  {
    return false;
  }

  // 3. 单次传输全部成功才提交结果。两种返回路径都会析构 transaction。
  output = response;
  return true;
}

class MockSpi final : public SpiPort
{
public:
  void select(bool active) noexcept override
  {
    assert(selected != active);
    selected = active;
    ++select_changes;
  }

  bool exchange(std::uint8_t output, std::uint8_t &input) noexcept override
  {
    assert(selected);
    ++exchanges;
    assert(output == (exchanges == 1U ? 0x05U : 0xFFU));
    if (exchanges == fail_on)
    {
      return false;
    }
    input = exchanges == 1U ? 0U : 0xA4U;
    return true;
  }

  bool selected = false;
  unsigned select_changes = 0;
  unsigned exchanges = 0;
  unsigned fail_on = 0;
};

int main()
{
  // 1. 先验证正常两次收发与片选释放。
  MockSpi ok;
  std::uint8_t status = 0;
  assert(read_status(ok, status) && status == 0xA4U);
  assert(ok.exchanges == 2U && ok.select_changes == 2U && !ok.selected);

  // 2. 分别模拟命令和回读失败。每种情况下都释放片选，并让旧结果继续可用。
  for (unsigned fail_on = 1U; fail_on <= 2U; ++fail_on)
  {
    MockSpi fail;
    fail.fail_on = fail_on;
    status = 0xCCU;
    assert(!read_status(fail, status) && status == 0xCCU);
    assert(fail.exchanges == fail_on && fail.select_changes == 2U && !fail.selected);
  }
  std::puts("SPI transaction: success and both failures release CS");
}
