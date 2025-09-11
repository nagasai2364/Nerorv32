#include <stdint.h>
#include "neorv32.h"

// 64-bit cycle read (RV32-safe)
static inline uint64_t rdcycle64(void) {
  uint32_t hi1, lo, hi2;
  asm volatile ("rdcycleh %0" : "=r"(hi1));
  asm volatile ("rdcycle  %0" : "=r"(lo));
  asm volatile ("rdcycleh %0" : "=r"(hi2));
  if (hi1 != hi2) { asm volatile ("rdcycle  %0" : "=r"(lo)); hi1 = hi2; }
  return ((uint64_t)hi1 << 32) | lo;
}

// 64-bit instret read (RV32-safe)
static inline uint64_t rdinstret64(void) {
  uint32_t hi1, lo, hi2;
  asm volatile ("rdinstreth %0" : "=r"(hi1));
  asm volatile ("rdinstret  %0" : "=r"(lo));
  asm volatile ("rdinstreth %0" : "=r"(hi2));
  if (hi1 != hi2) { asm volatile ("rdinstret  %0" : "=r"(lo)); hi1 = hi2; }
  return ((uint64_t)hi1 << 32) | lo;
}

// Show 64-bit value on 8 LEDs in 8 frames (MSB -> LSB)
static void show_u64_on_leds(uint64_t x, uint32_t clk_hz) {
  for (int frame = 7; frame >= 0; frame--) {
    uint8_t byte = (uint8_t)((x >> (frame*8)) & 0xffu);
    neorv32_gpio_port_set(byte);
    neorv32_aux_delay_ms(clk_hz, 500);   // 500 ms per frame
  }
  neorv32_aux_delay_ms(clk_hz, 800);     // extra pause
  neorv32_gpio_port_set(0x00);           // LEDs off
}

// Multiply-heavy kernel
static uint32_t mul_kernel(uint32_t N) {
  volatile uint32_t acc = 0, a = 12345, b = 6789;
  for (uint32_t i = 0; i < N; i++) {
    acc += a * b;
    a += 1;
    b ^= acc;
  }
  return acc;
}

int main(void) {
  // Get clock from SYSINFO (requires SYSINFO enabled in hardware)
const uint32_t CLK_HZ = (uint32_t)NEORV32_SYSINFO->CLK;  // SYSINFO is a pointer [9]
  neorv32_gpio_port_set(0x00);

  // Warm-up
  (void)mul_kernel(1000);

  // Measure
  const uint32_t N = 5000000;
  uint64_t c0 = rdcycle64();
  uint64_t i0 = rdinstret64();
  volatile uint32_t sink = mul_kernel(N);
  uint64_t c1 = rdcycle64();
  uint64_t i1 = rdinstret64();
  (void)sink;

  uint64_t delta_cycles = c1 - c0;
  uint64_t delta_insts  = i1 - i0;

  // Display cycles then instret
  show_u64_on_leds(delta_cycles, CLK_HZ);
  neorv32_aux_delay_ms(CLK_HZ, 1000);
  show_u64_on_leds(delta_insts,  CLK_HZ);

  // Loop display
  while (1) {
    neorv32_aux_delay_ms(CLK_HZ, 1000);
    show_u64_on_leds(delta_cycles, CLK_HZ);
    neorv32_aux_delay_ms(CLK_HZ, 1000);
    show_u64_on_leds(delta_insts,  CLK_HZ);
  }
}
