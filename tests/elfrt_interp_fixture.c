// SPDX-License-Identifier: MIT
extern int puts(const char *text);

int main(void)
{
    return puts("selinos-elfrt-interpreter-fixture") == 0 ? 0 : 1;
}
