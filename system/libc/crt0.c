/* ♥ CRT0 ~ "Startup do programa! Zera BSS, chama main() e depois exit()!"
 * Dica: O _start é o ponto de entrada~ o linker.ld coloca .text.start primeiro~
 * Se esquecer de chamar exit, o programa vira um zumbi~ hihi baka! kyun~ */
extern int main(int argc, char **argv);
extern char _bss_start, _bss_end;

extern void exit(int code);

__attribute__((section(".text.start")))
void _start(void) {
    for (char *p = &_bss_start; p < &_bss_end; p++) *p = 0;
    int ret = main(0, 0);
    exit(ret);
}
