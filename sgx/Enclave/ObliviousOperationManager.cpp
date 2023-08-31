#include "ObliviousOperationManager.h"

int obliviousSelectEq(int val1, int val2, int out1, int out2){
    int out = 0;

    asm ("movl %1, %%eax;"
         "movl %2, %%ebx;"
         "movl %3, %%ecx;"
         "movl %4, %%edx;"
         "xorl %%eax, %%ebx;"
         "cmovz %%ecx, %%edx;"
         "movl %%edx, %0;"
    : "=r" ( out )        /* output */
    : "r" ( val1 ), "r" ( val2 ), "r" (out1), "r" (out2)         /* input */
    : "%edx"         /* clobbered register */
    );
    //printf("out=%d\n", out);
    return out;
}

int obliviousSelectEq(float val1, float val2, int out1, int out2){
    int out = 0;

    asm ("movl %1, %%eax;"
         "movl %2, %%ebx;"
         "movl %3, %%ecx;"
         "movl %4, %%edx;"
         "xorl %%eax, %%ebx;"
         "cmovz %%ecx, %%edx;"
         "movl %%edx, %0;"
    : "=r" ( out )        /* output */
    : "r" ( val1 ), "r" ( val2 ), "r" (out1), "r" (out2)         /* input */
    : "%edx"         /* clobbered register */
    );
    //printf("out=%d\n", out);
    return out;
}

uint64_t __attribute__((noinline))
obliviousSelectEq(uint64_t input1, uint64_t input2) {
    uint64_t ret, one = 1ul, zero = 0ul;
    asm volatile(
    "cmp %[i2], %[i1];"
    "cmove %[one], %[ret];"
    "cmovne %[zero], %[ret];"
    : [ret] "=a" (ret)
    : [i1] "r" (input1),
    [i2] "r" (input2),
    [one] "r" (one),
    [zero] "r" (zero)
    );
    return ret;
}

uint64_t __attribute__((noinline))
obliviousAndOperation(uint64_t input1, uint64_t input2) {
    uint64_t ret, one = 1ul, zero = 0ul;
    asm volatile(
    "and %[i2], %[i1];"
    "cmovnz %[one], %[ret];"
    "cmovz %[zero], %[ret];"
    : [ret] "=a" (ret)
    : [i1] "r" (input1),
    [i2] "r" (input2),
    [one] "r" (one),
    [zero] "r" (zero)
    );
    return ret;
}

int obliviousSelectGt(float val1, float val2, int out1, int out2){
    int out = 0;

    asm ("movl %1, %%eax;"
         "movl %2, %%ebx;"
         "movl %3, %%ecx;"
         "movl %4, %%edx;"
         "cmpl %%eax, %%ebx;"
         "cmovnge %%ecx, %%edx;"
         "movl %%edx, %0;"
    : "=r" ( out )        /* output */
    : "r" ( val1 ), "r" ( val2 ), "r" (out1), "r" (out2)         /* input */
    : "%edx"         /* clobbered register */
    );
    //printf("out=%d\n", out);
    return out;
}

int obliviousSelectLt(float val1, float val2, int out1, int out2){
    int out = 0;

    asm ("movl %1, %%eax;"
         "movl %2, %%ebx;"
         "movl %3, %%ecx;"
         "movl %4, %%edx;"
         "cmpl %%eax, %%ebx;"
         "cmovnle %%ecx, %%edx;"
         "movl %%edx, %0;"
    : "=r" ( out )        /* output */
    : "r" ( val1 ), "r" ( val2 ), "r" (out1), "r" (out2)         /* input */
    : "%edx"         /* clobbered register */
    );
    //printf("out=%d\n", out);
    return out;
}

int obliviousStringLength(char *entry) {
    int c = 0;
    char temp;
    __asm__(
    "dec %[entry]\n"
    "movl %[c], %%eax\n"
    "StrLengthRepeat:\n\t"
    "inc %[entry]\n\t"
    "movb (%[entry]), %[temp]\n\t"  /* Read the next char */
    "testb %[temp], %[temp]\n\t"
    "jz StrLengthEnd\n\t"                  /* Is the char null */
    "inc %%eax\n\t"
    "jmp StrLengthRepeat\n\t"
    "StrLengthEnd:\n"
    "movl %%eax, %[out]\n"
    : [entry] "+r" (entry), [temp] "=r" (temp), [out] "=r" ( c )        /* output */
    : [c] "r" ( c )
    : "%eax"
    );
    //printf("Str Length = %d\n", c);
    return c;
}

int obliviousStringCompare(char *s1, char *s2){
    if(obliviousStringLength(s1) != obliviousStringLength(s2)) return 0;
    int out = 1;
    int t = 1;
    int f = 0;
    char t1, t2;
    __asm__ (
    "dec %[s1]\n"
    "dec %[s2]\n"
    "movl %[f], %%eax\n"
    //"movl %[t], %%ebx\n"
    "REPEAT:\n\t"
    "inc %[s1]\n\t"
    "inc %[s2]\n\t"
    "movb (%[s1]), %[t1]\n\t"  /* Read the next char */
    "movb (%[s2]), %[t2]\n\t"  /* Read the next char */
    "testb %[t1], %[t2]\n\t"
    "jz END\n\t"                  /* Is the char null */
    "cmpb %[t1], %[t2]\n\t"
    "cmovnz %%eax, %[out]\n"
    "je REPEAT\n\t"
    "END:\n"
    //"movl %%eax, %[out]\n"
    : [s1] "+r" (s1), [s2] "+r" (s2), [t1] "=r" (t1), [t2] "=r" (t2), [out] "=r" ( out )        /* output */
    : [t] "r" ( t ), [f] "r" ( f )
    : "%eax"
    );
    //printf("out=%d\n", out);
    return out;
}

void obliviousStringCopy(char *src, char *dest, int s_idx, int count){
    __asm__ __volatile__("cld\n"
                         "rep\n"
                         "movsb\n"
                         "xor %%al,%%al\n"
                         "stosb\n"
    :
    :"S"(src + s_idx), "D"(dest), "c"(count)
    :"memory"
    );
    printf("Dest Str = %s\n", dest);
}


/*
	 * o_memcpy_8(uint64_t pred, void *dst, void *src, size_t size);
	 *
	 * copy src -> dst if pred == 1
	 * copy src -> src if pred == 0
	 * copy exactly multipe of 8 bytes (not handing if size % 8 != 0)
	 * always returns size.
	 */
size_t __attribute__((noinline))
o_memcpy_8(uint64_t is_valid_copy, void* dst, void* src, size_t size) {
    size_t cnt = (size >> 3);
    asm volatile(
    // save registers; r15 = loop cnt, r14 = temp reg
    "push   %%r15;"
    "push   %%r14;"
    "push   %%r13;"   // src
    "push   %%r12;"   // dst

    // r15 is loop cnt
    "xor    %%r15, %%r15;"

    // loop head
    "loop_start%=:"
    "cmp    %%r15, %[cnt];"
    // loop exit condition
    "je out%=;"
    "mov    (%[dst], %%r15, 8), %%r12;"
    "mov    (%[src], %%r15, 8), %%r13;"

    // copy src (r13) if 1 (nz), copy dst (r12) if 0 (z)
    "test   %[pred], %[pred];"
    // copy dst to r14
    "cmovz  %%r12, %%r14;"
    // copy src to r14
    "cmovnz %%r13, %%r14;"
    // store to dst
    "mov    %%r14, (%[dst], %%r15, 8);"
    "inc    %%r15;"
    // loop back!
    "jmp    loop_start%=;"

    "out%=:"
    // restore registers
    "pop    %%r12;"
    "pop    %%r13;"
    "pop    %%r14;"
    "pop    %%r15;"
    :
    : [dst] "r" (dst),
    [src] "r" (src),
    [cnt] "r" (cnt),
    [pred] "r" (is_valid_copy)
    : "memory");
    return size;
}


/*
	 * o_memcpy_byte(uint64_t pred, void *dst, void *src, size_t size);
	 *
	 * copy src -> dst if pred == 1
	 * copy src -> src if pred == 0
	 * copy by each byte.
	 * always returns size.
	 */
size_t __attribute__((noinline))
o_memcpy_byte(uint64_t is_valid_copy, void* dst, void* src, size_t size) {
    asm volatile(
    // save registers; r15 = loop cnt, r14 = temp reg
    "push   %%r15;"
    "push   %%r14;"
    "push   %%r13;"   // src
    "push   %%r12;"   // dst

    // r15 is loop cnt
    "xor    %%r15, %%r15;"

    // loop head
    "loop_start%=:"
    "cmp    %%r15, %[cnt];"
    // loop exit condition
    "je out%=;"
    "movb   (%[dst], %%r15, 1), %%r12b;"
    "movb   (%[src], %%r15, 1), %%r13b;"

    // copy src (r13) if 1 (nz), copy dst (r12) if 0 (z)
    "test   %[pred], %[pred];"
    // copy dst to r14
    "cmovz  %%r12, %%r14;"
    // copy src to r14
    "cmovnz %%r13, %%r14;"
    // store to dst
    "movb   %%r14b, (%[dst], %%r15, 1);"
    "inc    %%r15;"
    // loop back!
    "jmp    loop_start%=;"

    "out%=:"
    // restore registers
    "pop    %%r12;"
    "pop    %%r13;"
    "pop    %%r14;"
    "pop    %%r15;"
    :
    : [dst] "r" (dst),
    [src] "r" (src),
    [cnt] "r" (size),
    [pred] "r" (is_valid_copy)
    : "memory");
    return size;
}


size_t o_memcpy(uint32_t pred, void* dst, void* src, size_t size) {
    size_t mult_size = (size >> 3) << 3;
    size_t remaining_size = size & 7;
    size_t copied = 0;
    if (mult_size != 0) {
        copied += o_memcpy_8(pred, dst, src, mult_size);
    }

    if (remaining_size) {
        uint8_t *c_dst = (uint8_t *) dst;
        uint8_t *c_src = (uint8_t *) src;
        copied += o_memcpy_byte(pred, c_dst + mult_size, c_src + mult_size, remaining_size);
    }
    return copied;
}
