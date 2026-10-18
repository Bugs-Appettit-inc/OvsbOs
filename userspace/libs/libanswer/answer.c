/* answer.c — biblioteca de teste para dlopen/dlsym
 * Exporta: int answer(void) { return 42; }
 * Compilar: gcc -shared -fPIC -o libanswer.so answer.c
 */

int answer(void) {
    return 42;
}
