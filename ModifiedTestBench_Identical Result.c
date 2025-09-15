#include <stdint.h>
#include "neorv32.h"

// Set to 1 if board LEDs are active-low (common on many dev kits), else 0.
#ifndef LED_ACTIVE_LOW
#define LED_ACTIVE_LOW 1
#endif

static inline void led_write_byte(uint8_t b) {
  if (LED_ACTIVE_LOW) b = (uint8_t)~b;
  neorv32_gpio_port_set(b);
}

// MUL-heavy kernel (no DIV) to reflect multiplier performance
__attribute__((noinline))
static uint32_t mul_throughput(uint32_t iters) {
  uint32_t a = 12345u, b = 6789u, c = 42407u, d = 77u, acc = 0u;
  for (uint32_t i = 0; i < iters; i++) {
    uint32_t p0 = a * b;
    uint32_t p1 = c * d;
    uint32_t p2 = (a + 3u) * (b + 5u);
    uint32_t p3 = (c + 7u) * (d + 11u);
    acc += p0 + p1 + p2 + p3;
    a += 1u; c += 1u;
  }
  return acc;
}

int main(void) {
  const uint32_t CLK_HZ = (uint32_t)NEORV32_SYSINFO->CLK; // not used, but handy if you later compute exact time
  (void)CLK_HZ;

  // Make first visible state unambiguous:
  // For active-low: 0xFF means all LEDs off; for active-high: 0x00 is all off.
  led_write_byte(0xFF); // start with LEDs OFF visibly (works when LED_ACTIVE_LOW=1)

  // Optional tiny warm-up to settle pipelines/caches (kept short)
  (void)mul_throughput(1000u);

  const uint32_t N = 5000000u; // keep this identical across baseline/enhanced runs
  volatile uint32_t sink = mul_throughput(N);

  (void)sink;

  // Mark completion with a single, solid "ALL ON" for ~2 seconds
  // Active-low boards see 0x00 as "all on"; active-high will see 0xFF as "all on".
  if (LED_ACTIVE_LOW) {
    led_write_byte(0x00); // ALL ON (active-low)
  } else {
    led_write_byte(0xFF); // ALL ON (active-high)
  }

  // Crude delay using simple busy loop (avoids timers/UART); adjust if needed
  for (volatile uint32_t i = 0; i < 50u * 1000u * 100u; i++) {
    __asm__ volatile("nop");
  }

  // Park forever
  for (;;) { }
  return 0;
}
