/**
 * @file rvv_detect.c
 * @brief RVV runtime detection and capability reporting
 *
 * Queries the hardware for RVV support (via misa), reads VLEN/VLENB,
 * and prints VL for various SEW/LMUL configurations.
 */

#include "rvv/rvv_detect.h"

#include "console.h"

void rvv_print_info(void)
{
    char buf[32];

    if (!rvv_available()) {
        console_puts("[RVV] Not available (misa V-bit not set)\n");
        return;
    }

    console_puts("[RVV] Available\n");

    /* Enable vector unit before reading vlenb */
    rvv_enable();

    uint64_t vlen = rvv_get_vlen();
    uint64_t vlenb = rvv_get_vlenb();

    console_puts("[RVV] VLEN  = ");
    console_put_dec(vlen, buf, sizeof(buf));
    console_puts(buf);
    console_puts(" bits\n");

    console_puts("[RVV] VLENB = ");
    console_put_dec(vlenb, buf, sizeof(buf));
    console_puts(buf);
    console_puts(" bytes\n");

    /* Query VL for various SEW/LMUL combinations using vsetvli */
    size_t vl;
    uint64_t avl = 1024; /* Request a large AVL to see max VL */

    /* e8, m1: VL = VLEN/8 */
    __asm__ __volatile__("vsetvli %0, %1, e8, m1, ta, ma"
                         : "=r"(vl)
                         : "r"(avl));
    console_puts("[RVV] VL(e8,m1)   = ");
    console_put_dec((uint64_t) vl, buf, sizeof(buf));
    console_puts(buf);
    console_puts("\n");

    /* e32, m1: VL = VLEN/32 */
    __asm__ __volatile__("vsetvli %0, %1, e32, m1, ta, ma"
                         : "=r"(vl)
                         : "r"(avl));
    console_puts("[RVV] VL(e32,m1)  = ");
    console_put_dec((uint64_t) vl, buf, sizeof(buf));
    console_puts(buf);
    console_puts("\n");

    /* e32, m4: VL = 4*VLEN/32 */
    __asm__ __volatile__("vsetvli %0, %1, e32, m4, ta, ma"
                         : "=r"(vl)
                         : "r"(avl));
    console_puts("[RVV] VL(e32,m4)  = ");
    console_put_dec((uint64_t) vl, buf, sizeof(buf));
    console_puts(buf);
    console_puts("\n");

    /* e64, m1: VL = VLEN/64 */
    __asm__ __volatile__("vsetvli %0, %1, e64, m1, ta, ma"
                         : "=r"(vl)
                         : "r"(avl));
    console_puts("[RVV] VL(e64,m1)  = ");
    console_put_dec((uint64_t) vl, buf, sizeof(buf));
    console_puts(buf);
    console_puts("\n");
}
