#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "layout.h"
#include "policy.h"
#include "../boot-app/boot/image.h"

static void check_policy(void)
{
  uint32_t journal[6U * STATE_WORDS];
  for (unsigned i = 0; i < sizeof(journal) / sizeof(journal[0]); ++i) { journal[i] = UINT32_MAX; }
  assert(scan_boot_state(journal, 6U).state == CONFIRMED_A);
  const BootPlan requested = plan_boot(REQUEST_B, true, true);
  assert(requested.trial && requested.save_before_jump == TRIED_B);
  assert(chosen_slot(requested, true) == SLOT_B && chosen_slot(requested, false) == SLOT_A);
  assert(plan_boot(REQUEST_B, true, false).target == SLOT_A);
  assert(plan_boot(REQUEST_B, false, true).target == SLOT_NONE);

  // 1. 电源切在每个状态字写入之后。确认未提交时仍回 A，提交后选 B。
  journal[0] = REQUEST_B; journal[1] = boot_state_crc(REQUEST_B); journal[2] = STATE_COMMIT;
  journal[3] = TRIED_B; journal[4] = boot_state_crc(TRIED_B);
  assert(scan_boot_state(journal, 6U).state == REQUEST_B);
  assert(scan_boot_state(journal, 6U).next_record == 2U);
  journal[5] = STATE_COMMIT;
  assert(scan_boot_state(journal, 6U).state == TRIED_B);
  assert(plan_boot(TRIED_B, true, true).target == SLOT_A);
  journal[6] = CONFIRMED_B; journal[7] = boot_state_crc(CONFIRMED_B);
  assert(scan_boot_state(journal, 6U).state == TRIED_B);
  journal[8] = STATE_COMMIT;
  assert(scan_boot_state(journal, 6U).state == CONFIRMED_B);
  assert(plan_boot(CONFIRMED_B, true, true).target == SLOT_B);
  journal[7] ^= 1U;
  assert(scan_boot_state(journal, 6U).state == TRIED_B);

  // 2. 反向请求也必须先记录已试跑。B 已确认时，A 试跑后的复位回 B。
  const BootPlan reverse = plan_boot(REQUEST_A, true, true);
  assert(reverse.trial && reverse.save_before_jump == TRIED_A);
  assert(chosen_slot(reverse, false) == SLOT_B && chosen_slot(reverse, true) == SLOT_A);
  assert(plan_boot(TRIED_A, true, true).target == SLOT_B);
}

static void check_artifact(const char *header_path, const char *image_path,
                           uint32_t vector, uint32_t other, size_t capacity)
{
  uint8_t header[IMAGE_HEADER_BYTES];
  uint8_t *image = malloc(capacity);
  assert(image != NULL);
  FILE *input = fopen(header_path, "rb");
  assert(input != NULL && fread(header, 1U, sizeof(header), input) == sizeof(header));
  assert(fgetc(input) == EOF && !ferror(input) && fclose(input) == 0);
  input = fopen(image_path, "rb");
  assert(input != NULL);
  const size_t size = fread(image, 1U, capacity, input);
  assert(!ferror(input) && fgetc(input) == EOF && fclose(input) == 0);
  AppEntry entry;
  assert(validate_at(header, image, vector, size, &entry) == IMAGE_OK);
  assert(validate_at(header, image, other, size, &entry) == IMAGE_BAD_VECTOR);
  image[size - 1U] ^= 1U;
  assert(validate_at(header, image, vector, size, &entry) == IMAGE_BAD_CRC);
  free(image);
}

int main(int argc, char **argv)
{
  assert(argc == 5);
  check_policy();
  check_artifact(argv[1], argv[2], A_VECTOR, B_VECTOR, A_CAPACITY);
  check_artifact(argv[3], argv[4], B_VECTOR, A_VECTOR, B_CAPACITY);
  puts("Boot A/B: commit ordering, rollback, confirmation, bad CRC, real slot addresses");
  return 0;
}
