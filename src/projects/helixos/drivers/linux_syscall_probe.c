// SPDX-License-Identifier: MIT

#define SELINOS_LINUX_SYS_READ 0ul
#define SELINOS_LINUX_SYS_WRITE 1ul
#define SELINOS_LINUX_SYS_MMAP 9ul
#define SELINOS_LINUX_SYS_MPROTECT 10ul
#define SELINOS_LINUX_SYS_BRK 12ul
#define SELINOS_LINUX_SYS_PREAD64 17ul
#define SELINOS_LINUX_SYS_LSEEK 8ul
#define SELINOS_LINUX_SYS_GETDENTS64 217ul
#define SELINOS_LINUX_SYS_FCNTL 72ul
#define SELINOS_LINUX_F_GETFL 3ul
#define SELINOS_LINUX_SYS_UNAME 63ul
#define SELINOS_LINUX_SYS_ARCH_PRCTL 158ul
#define SELINOS_LINUX_ARCH_SET_FS 0x1002ul
#define SELINOS_LINUX_ARCH_GET_FS 0x1003ul
#define SELINOS_LINUX_SYS_CLOSE 3ul
#define SELINOS_LINUX_SYS_FSTAT 5ul
#define SELINOS_LINUX_SYS_OPENAT 257ul
#define SELINOS_LINUX_SYS_EXIT 60ul
#define SELINOS_LINUX_SYS_GETPID 39ul
#define SELINOS_LINUX_SYS_GETUID 102ul
#define SELINOS_LINUX_SYS_GETGID 104ul
#define SELINOS_LINUX_SYS_GETEUID 107ul
#define SELINOS_LINUX_SYS_GETEGID 108ul
#define SELINOS_LINUX_SYS_GETTID 186ul
#define SELINOS_LINUX_SYS_SET_TID_ADDRESS 218ul
#define SELINOS_LINUX_SYS_GETRLIMIT 97ul
#define SELINOS_LINUX_SYS_FUTEX 202ul
#define SELINOS_LINUX_SYS_CLOCK_GETTIME 228ul
#define SELINOS_LINUX_PAGE_SIZE 4096ul
#define SELINOS_LINUX_PROT_READ_WRITE 3ul
#define SELINOS_LINUX_MAP_PRIVATE_ANONYMOUS 0x22ul
#define SELINOS_LINUX_TEST_PID 4242ul
#define SELINOS_LINUX_TEST_TID 4243ul
#define SELINOS_LINUX_CLOCK_MONOTONIC 1ul
#define SELINOS_LINUX_TEST_TIME_SECONDS 1234ul
#define SELINOS_LINUX_TEST_TIME_NANOSECONDS 0ul
#define SELINOS_LINUX_RLIMIT_NOFILE 7ul
#define SELINOS_LINUX_TEST_RLIMIT_NOFILE 64ul
#define SELINOS_LINUX_FUTEX_WAIT 0ul
#define SELINOS_LINUX_FUTEX_WAKE 1ul
#define SELINOS_LINUX_EAGAIN 11ul
#define SELINOS_LINUX_ENOTDIR 20ul
#define SELINOS_LINUX_AT_FDCWD (-100l)
#define SELINOS_LINUX_SEEK_SET 0ul
#define SELINOS_ROMFS_RELEASE_FD 3ul
#define SELINOS_ROMFS_VERSION_FD 6ul
#define SELINOS_ROMFS_RELEASE_MAGIC 0x53454c494e4f5321ul
#define SELINOS_ROMFS_RELEASE_MAGIC_LENGTH 8ul
#define SELINOS_ROMFS_VERSION_MAGIC 0x53454c494e4f5339ul
#define SELINOS_ROMFS_VERSION_MAGIC_LENGTH 8ul
#define SELINOS_LINUX_M10_STAT_MAGIC 0x53454c5354415430ul
#define SELINOS_LINUX_M10_STAT_SIZE 8ul
#define SELINOS_LINUX_TLS_TEST_WORD 0x544c5353454c494eul

const char selinos_linux_write_message[] =
    "SeLinOS ABI M3: mediated Linux write(1) userspace payload passed.\n";
const char selinos_linux_release_path[] = "/selinos-release";
const char selinos_linux_version_path[] = "/selinos-version";
const char selinos_linux_volatile_state_path[] = "/selinos-state";
const char selinos_linux_invalid_state_path[] = "/selinos-statu";
const char selinos_linux_volatile_state_payload[] = "STATEM0!";
volatile unsigned long selinos_linux_tid_storage;

int main(void)
{
    /* The path is deliberately stack-independent after the first syscall.
     * Root accepts exit(0) only after userspace checks both return values. */
    __asm__ volatile(
        "mov %[getpid], %%rax\n"
        "syscall\n"
        "cmp %[expected_pid], %%rax\n"
        "jne 1f\n"
        "mov %[getuid], %%rax\n"
        "syscall\n"
        "test %%rax, %%rax\n"
        "jnz 1f\n"
        "mov %[getgid], %%rax\n"
        "syscall\n"
        "test %%rax, %%rax\n"
        "jnz 1f\n"
        "mov %[geteuid], %%rax\n"
        "syscall\n"
        "test %%rax, %%rax\n"
        "jnz 1f\n"
        "mov %[getegid], %%rax\n"
        "syscall\n"
        "test %%rax, %%rax\n"
        "jnz 1f\n"
        "mov %[gettid], %%rax\n"
        "syscall\n"
        "cmp %[expected_tid], %%rax\n"
        "jne 1f\n"
        "mov %[set_tid_address], %%rax\n"
        "lea selinos_linux_tid_storage(%%rip), %%rdi\n"
        "syscall\n"
        "cmp %[expected_tid], %%rax\n"
        "jne 1f\n"
        "mov %[write], %%rax\n"
        "mov $1, %%rdi\n"
        "lea selinos_linux_write_message(%%rip), %%rsi\n"
        "mov %[message_length], %%rdx\n"
        "syscall\n"
        "cmp %[message_length], %%rax\n"
        "jne 1f\n"
        "mov %[mmap], %%rax\n"
        "xor %%rdi, %%rdi\n"
        "mov %[page_size], %%rsi\n"
        "mov %[prot], %%rdx\n"
        "mov %[map_flags], %%r10\n"
        "mov $-1, %%r8\n"
        "xor %%r9, %%r9\n"
        "syscall\n"
        "test %%rax, %%rax\n"
        "jz 1f\n"
        "mov %%rax, %%r12\n"
        "mov $63, %%rax\n"
        "mov %%r12, %%rdi\n"
        "syscall\n"
        "test %%rax, %%rax\n"
        "jnz 1f\n"
        "cmpb $'L', (%%r12)\n"
        "jne 1f\n"
        "cmpb $'s', 65(%%r12)\n"
        "jne 1f\n"
        "cmpb $'6', 130(%%r12)\n"
        "jne 1f\n"
        "cmpb $'S', 195(%%r12)\n"
        "jne 1f\n"
        "cmpb $'x', 260(%%r12)\n"
        "jne 1f\n"
        "cmpb $'l', 325(%%r12)\n"
        "jne 1f\n"
        "mov $158, %%rax\n"
        "mov $0x1002, %%rdi\n"
        "mov %%r12, %%rsi\n"
        "syscall\n"
        "test %%rax, %%rax\n"
        "jnz 1f\n"
        "mov $158, %%rax\n"
        "mov $0x1003, %%rdi\n"
        "lea 32(%%r12), %%rsi\n"
        "syscall\n"
        "test %%rax, %%rax\n"
        "jnz 1f\n"
        "cmp %%r12, 32(%%r12)\n"
        "jne 1f\n"
        "mov $0x544c5353454c494e, %%r8\n"
        "mov %%r8, %%fs:0\n"
        "mov %%fs:0, %%r9\n"
        "cmp %%r8, %%r9\n"
        "jne 1f\n"
        "movb $0x5a, (%%r12)\n"
        "mov $228, %%rax\n"
        "mov $1, %%rdi\n"
        "mov %%r12, %%rsi\n"
        "syscall\n"
        "test %%rax, %%rax\n"
        "jnz 1f\n"
        "cmpq $1234, (%%r12)\n"
        "jne 1f\n"
        "cmpq $0, 8(%%r12)\n"
        "jne 1f\n"
        "mov $97, %%rax\n"
        "mov $7, %%rdi\n"
        "lea 16(%%r12), %%rsi\n"
        "syscall\n"
        "test %%rax, %%rax\n"
        "jnz 1f\n"
        "cmpq $64, 16(%%r12)\n"
        "jne 1f\n"
        "cmpq $64, 24(%%r12)\n"
        "jne 1f\n"
        "mov $202, %%rax\n"
        "lea selinos_linux_tid_storage(%%rip), %%rdi\n"
        "xor %%rsi, %%rsi\n"
        "mov $1, %%rdx\n"
        "xor %%r10, %%r10\n"
        "xor %%r8, %%r8\n"
        "xor %%r9, %%r9\n"
        "syscall\n"
        "cmp $-11, %%eax\n"
        "jne 1f\n"
        "mov $202, %%rax\n"
        "lea selinos_linux_tid_storage(%%rip), %%rdi\n"
        "mov $1, %%rsi\n"
        "mov $1, %%rdx\n"
        "xor %%r10, %%r10\n"
        "xor %%r8, %%r8\n"
        "xor %%r9, %%r9\n"
        "syscall\n"
        "test %%rax, %%rax\n"
        "jnz 1f\n"
        "mov %[brk], %%rax\n"
        "xor %%rdi, %%rdi\n"
        "syscall\n"
        "test %%rax, %%rax\n"
        "jz 1f\n"
        "add %[page_size], %%rax\n"
        "mov %%rax, %%rdi\n"
        "mov %[brk], %%rax\n"
        "syscall\n"
        "cmp %%rdi, %%rax\n"
        "jne 1f\n"
        "movb $0x6b, -1(%%rax)\n"
        "mov %[openat], %%rax\n"
        "mov %[at_fdcwd], %%rdi\n"
        "lea selinos_linux_release_path(%%rip), %%rsi\n"
        "xor %%rdx, %%rdx\n"
        "xor %%r10, %%r10\n"
        "syscall\n"
        "cmp %[release_fd], %%rax\n"
        "jne 1f\n"
        "mov %%rax, %%rdi\n"
        "mov %[read], %%rax\n"
        "mov %%r12, %%rsi\n"
        "mov %[magic_length], %%rdx\n"
        "syscall\n"
        "cmp %[magic_length], %%rax\n"
        "jne 1f\n"
        "mov %[release_magic], %%r8\n"
        "cmp %%r8, (%%r12)\n"
        "jne 1f\n"
        "mov %[close], %%rax\n"
        "mov %[release_fd], %%rdi\n"
        "syscall\n"
        "test %%rax, %%rax\n"
        "jnz 1f\n"
        "mov %[openat], %%rax\n"
        "mov %[at_fdcwd], %%rdi\n"
        "lea selinos_linux_version_path(%%rip), %%rsi\n"
        "xor %%rdx, %%rdx\n"
        "xor %%r10, %%r10\n"
        "syscall\n"
        "cmp $6, %%rax\n"
        "jne 1f\n"
        "mov $72, %%rax\n"
        "mov $6, %%rdi\n"
        "mov $3, %%rsi\n"
        "xor %%rdx, %%rdx\n"
        "syscall\n"
        "test %%rax, %%rax\n"
        "jnz 1f\n"
        "mov $6, %%rdi\n"
        "mov %%r12, %%rsi\n"
        "mov %[fstat], %%rax\n"
        "syscall\n"
        "test %%rax, %%rax\n"
        "jnz 1f\n"
        "mov $0x53454c5354415430, %%r8\n"
        "cmp %%r8, (%%r12)\n"
        "jne 1f\n"
        "cmpq $8, 8(%%r12)\n"
        "jne 1f\n"
        "mov $6, %%rdi\n"
        "mov %[read], %%rax\n"
        "mov %%r12, %%rsi\n"
        "mov $8, %%rdx\n"
        "syscall\n"
        "cmp $8, %%rax\n"
        "jne 1f\n"
        "mov $0x53454c494e4f5339, %%r8\n"
        "cmp %%r8, (%%r12)\n"
        "jne 1f\n"
        "mov $217, %%rax\n"
        "mov $6, %%rdi\n"
        "mov %%r12, %%rsi\n"
        "mov $32, %%rdx\n"
        "syscall\n"
        "cmp $-20, %%eax\n"
        "jne 1f\n"
        "mov %[close], %%rax\n"
        "mov $6, %%rdi\n"
        "syscall\n"
        "test %%rax, %%rax\n"
        "jnz 1f\n"
        "mov $257, %%rax\n"
        "mov $-100, %%rdi\n"
        "lea selinos_linux_invalid_state_path(%%rip), %%rsi\n"
        "xor %%rdx, %%rdx\n"
        "xor %%r10, %%r10\n"
        "syscall\n"
        "cmp $-2, %%eax\n"
        "jne 1f\n"
        "mov $257, %%rax\n"
        "mov $-100, %%rdi\n"
        "lea selinos_linux_volatile_state_path(%%rip), %%rsi\n"
        "xor %%rdx, %%rdx\n"
        "xor %%r10, %%r10\n"
        "syscall\n"
        "cmp $5, %%rax\n"
        "jne 1f\n"
        "mov %[mprotect], %%rax\n"
        "mov %%r12, %%rdi\n"
        "mov %[page_size], %%rsi\n"
        "mov %[prot], %%rdx\n"
        "syscall\n"
        "test %%rax, %%rax\n"
        "jnz 1f\n"
        "mov %[lseek], %%rax\n"
        "mov $5, %%rdi\n"
        "xor %%rsi, %%rsi\n"
        "mov %[seek_set], %%rdx\n"
        "syscall\n"
        "test %%rax, %%rax\n"
        "jnz 1f\n"
        "mov %[pread64], %%rax\n"
        "mov $5, %%rdi\n"
        "mov %%r12, %%rsi\n"
        "mov $8, %%rdx\n"
        "xor %%r10, %%r10\n"
        "xor %%r8, %%r8\n"
        "xor %%r9, %%r9\n"
        "syscall\n"
        "cmp $8, %%rax\n"
        "jne 1f\n"
        "mov $0x424f4f544d302121, %%r8\n"
        "cmp %%r8, (%%r12)\n"
        "jne 1f\n"
        "mov $5, %%rdi\n"
        "mov %[read], %%rax\n"
        "mov %%r12, %%rsi\n"
        "mov $8, %%rdx\n"
        "syscall\n"
        "cmp $8, %%rax\n"
        "jne 1f\n"
        "mov $0x424f4f544d302121, %%r8\n"
        "cmp %%r8, (%%r12)\n"
        "jne 1f\n"
        "mov %[write], %%rax\n"
        "mov $5, %%rdi\n"
        "lea selinos_linux_volatile_state_payload(%%rip), %%rsi\n"
        "mov $8, %%rdx\n"
        "syscall\n"
        "cmp $8, %%rax\n"
        "jne 1f\n"
        "mov %[read], %%rax\n"
        "mov $5, %%rdi\n"
        "mov %%r12, %%rsi\n"
        "mov $8, %%rdx\n"
        "syscall\n"
        "cmp $8, %%rax\n"
        "jne 1f\n"
        "mov $0x53544154454d3021, %%r8\n"
        "cmp %%r8, (%%r12)\n"
        "jne 1f\n"
        "mov %[close], %%rax\n"
        "mov $5, %%rdi\n"
        "syscall\n"
        "test %%rax, %%rax\n"
        "jnz 1f\n"
        "mov %[write], %%rax\n"
        "mov $5, %%rdi\n"
        "lea selinos_linux_volatile_state_payload(%%rip), %%rsi\n"
        "mov $8, %%rdx\n"
        "syscall\n"
        "cmp $-9, %%eax\n"
        "jne 1f\n"
        "mov %[read], %%rax\n"
        "xor %%rdi, %%rdi\n"
        "xor %%rsi, %%rsi\n"
        "xor %%rdx, %%rdx\n"
        "syscall\n"
        "test %%rax, %%rax\n"
        "jnz 1f\n"
        "mov %[exit], %%rax\n"
        "xor %%rdi, %%rdi\n"
        "syscall\n"
        "1: jmp 1b\n"
        :
        : [read] "i"(SELINOS_LINUX_SYS_READ),
          [close] "i"(SELINOS_LINUX_SYS_CLOSE),
          [fstat] "i"(SELINOS_LINUX_SYS_FSTAT),
          [openat] "i"(SELINOS_LINUX_SYS_OPENAT),
          [getpid] "i"(SELINOS_LINUX_SYS_GETPID),
          [getuid] "i"(SELINOS_LINUX_SYS_GETUID),
          [getgid] "i"(SELINOS_LINUX_SYS_GETGID),
          [geteuid] "i"(SELINOS_LINUX_SYS_GETEUID),
          [getegid] "i"(SELINOS_LINUX_SYS_GETEGID),
          [gettid] "i"(SELINOS_LINUX_SYS_GETTID),
          [set_tid_address] "i"(SELINOS_LINUX_SYS_SET_TID_ADDRESS),
          [write] "i"(SELINOS_LINUX_SYS_WRITE),
          [mmap] "i"(SELINOS_LINUX_SYS_MMAP),
          [mprotect] "i"(SELINOS_LINUX_SYS_MPROTECT),
          [pread64] "i"(SELINOS_LINUX_SYS_PREAD64),
          [lseek] "i"(SELINOS_LINUX_SYS_LSEEK),
          [brk] "i"(SELINOS_LINUX_SYS_BRK),
          [exit] "i"(SELINOS_LINUX_SYS_EXIT),
          [expected_pid] "i"(SELINOS_LINUX_TEST_PID),
          [expected_tid] "i"(SELINOS_LINUX_TEST_TID),
          [at_fdcwd] "i"(SELINOS_LINUX_AT_FDCWD),
          [release_fd] "i"(SELINOS_ROMFS_RELEASE_FD),
          [release_magic] "i"(SELINOS_ROMFS_RELEASE_MAGIC),
          [magic_length] "i"(SELINOS_ROMFS_RELEASE_MAGIC_LENGTH),
          [message_length] "i"(sizeof(selinos_linux_write_message) - 1u),
          [page_size] "i"(SELINOS_LINUX_PAGE_SIZE),
          [prot] "i"(SELINOS_LINUX_PROT_READ_WRITE),
          [map_flags] "i"(SELINOS_LINUX_MAP_PRIVATE_ANONYMOUS),
          [seek_set] "i"(SELINOS_LINUX_SEEK_SET)
        : "rax", "rdi", "rsi", "rdx", "r8", "r10", "r11", "r12", "rcx", "cc", "memory");
    __builtin_unreachable();
}
