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
int calcular_mandelbrot(double x0, double y0,
int max_iteracoes){

    double z_real = 0.0;
    double z_imag = 0.0;
    int iteracoes ;

    for(iteracoes =0 ; iteracoes < max_iteracoes; iteraçoes++){
        double novo_zreal;
        double novo_zimag;
    

    novo_zreal = z_real * z_real - z_imag * z_imag + x0;
    novo_zimag = 2.0 * z_real * z_imag + y0;
    z_real = novo_zreal;
    z_imag = novo_zimag;
    if(z_real * z_real + z_imag * z_imag > 4.0){
        break;
    }
    
    return iteracoes;
}
double converter_real(int x, int largura)
{
    return -2.0 +
           (double)x * 3.0 / (double)(largura - 1);
}


double converter_imag(int y, int altura)
{
    return -1.5 +
           (double)y * 3.0 / (double)(altura - 1);
}

int calcular_intensidade(int iteracoes, int max_iteracoes ){
    
    if(iteracoes == max_iteracoes){
        return 0;
    }
    return (int)(255.0 * iteracoes / max_iteracoes);
}
void mandelbrot_serial(int *pixels, int largura, int altura, int max_iteracoes){
  
    int y;
    for(y=0; y < altura; y++){
        int x;
        double c_imag = converter_imag(y,altura);
        for(x=0; x < largura; x++){
            double c_real = converter_real(x, largura);
            int iteracoes = calcular_mandelbrot(c_real, c_imag, max_iteracoes);
            int intensidade = calcular_intensidade(iteracoes, max_iteracoes);
            pixels[y * largura + x] = intensidade;
        }
    }
}
void *pthread_blocos(void *arg){
    ThreadData *dados =(ThreadData *)arg;

    int inicio;
    int fim;
    

    inicio = dados->thread_id * (dados->altura / dados->num_threads);
    fim = (dados->thread_id + 1) * (dados->altura / dados->num_threads);

    for(int y = inicio; y < fim; y++){
        double c_imag = converter_imag(y, dados->altura);
        for(int x = 0; x < dados->largura; x++){
            double c_real = converter_real(x, dados->largura);
            int iteracoes = calcular_mandelbrot(c_real, c_imag, dados->max_iteracoes);
            int intensidade = calcular_intensidade(iteracoes, dados->max_iteracoes);
            dados->pixels[y * dados->largura + x] = intensidade;
    }
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
