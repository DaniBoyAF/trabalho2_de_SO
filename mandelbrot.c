#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <time.h>
#include <pthread.h>
#include <omp.h>

#define LOGIN "SEU_LOGIN"

typedef struct{
    int largura;
    int altura;
    int max_iteracoes;
    int num_threads;
    int *pixels;
    int thread_id;
} MandelbrotArgs;

int converter_int(const char *str, int *valor)
{
    char*fim;
    long resultado;

    errno = 0;
    resultado = strtol(str, &fim, 10);

    if (errno != 0 || *fim != '\0' || resultado < INT_MIN || resultado > INT_MAX) {
        return 0;
    }
    if (resultado < 1 || resultado > INT_MAX) {
        return 0;
    }
    *valor = (int)resultado;
    return 1;
}
int main(int argc, char *argv[]){

    if(argc != 5){
        fprintf(stderr, "Uso: %s [largura] [altura] [max_iteracoes] [num_threads]\n", argv[0]);
        return 1;
    }

    int largura;
    int altura;
    int max_iteracoes;
    int num_threads;

    largura = atoi(argv[1]);
    altura = atoi(argv[2]);
    max_iteracoes = atoi(argv[3]);
    num_threads = atoi(argv[4]);
    if (largura <= 0 || altura <= 0 || max_iteracoes <= 0 || num_threads <= 0) {
        fprintf(stderr, "Erro: parametros invalidos.\n");
        return 1;
    }
     int *pixels;

    pixels = malloc(largura * altura * sizeof(int));

    if (pixels == NULL) {
        fprintf(stderr, "Erro: falha na alocacao de memoria.\n");
        return 1;
    }

    free(pixels);

    return 0;
    }
