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
            int indice = y*dados->largura + x;
            dados->pixels[indice] = calcular_intensidade(iteracoes, dados->max_iteracoes);
        }
    }
    return NULL;
}

void *pthread_ciclico(void *arg){
    ThreadData *dados = (ThreadData *)arg;

    for(int y = dados->thread_id; y < dados->altura; y += dados->num_threads){
        double c_imag = converter_imag(y, dados->altura);
        for(int x = 0; x < dados->largura; x++){
            double c_real = converter_real(x, dados->largura);
            int iteracoes = calcular_mandelbrot(c_real, c_imag, dados->max_iteracoes);
            int indice = y*dados->largura + x;
            dados->pixels[indice] = calcular_intensidade(iteracoes, dados->max_iteracoes);
        }
    }
    return NULL;
}
int executar_pthreads(int *pixels,
                      int largura,
                      int altura,
                      int max_iteracoes,
                      int num_threads,
                      int estrategia)
{
    pthread_t *threads;

    ThreadData *dados;

    threads =
        malloc(num_threads * sizeof(pthread_t));

    dados =
        malloc(num_threads * sizeof(ThreadData));

    if (threads == NULL || dados == NULL) {

        free(threads);
        free(dados);

        return 0;
    }

    for (int i = 0; i < num_threads; i++) {

        dados[i].pixels = pixels;
        dados[i].largura = largura;
        dados[i].altura = altura;
        dados[i].max_iteracoes = max_iteracoes;
        dados[i].num_threads = num_threads;
        dados[i].thread_id = i;

        void *(*funcao)(void *);

        if (estrategia == 1) {
            funcao = pthread_blocos;
        } else {
            funcao = pthread_ciclico;
        }

        int resultado =
            pthread_create(
                &threads[i],
                NULL,
                funcao,
                &dados[i]
            );

        if (resultado != 0) {

            fprintf(
                stderr,
                "Erro: falha na criacao da thread.\n"
            );

            for (int j = 0; j < i; j++) {
                pthread_join(threads[j], NULL);
            }

            free(threads);
            free(dados);

            return 0;
        }
    }

    for (int i = 0; i < num_threads; i++) {

        if (pthread_join(threads[i], NULL) != 0) {

            fprintf(
                stderr,
                "Erro: falha ao aguardar thread.\n"
            );

            free(threads);
            free(dados);

            return 0;
        }
    }

    free(threads);
    free(dados);

    return 1;
}
int salvar_imagem(const char *nome_arquivo,
                  int *pixels,
                  int largura,
                  int altura)
{
    FILE *arquivo;

    arquivo = fopen(nome_arquivo, "w");

    if (arquivo == NULL) {

        fprintf(
            stderr,
            "Erro: nao foi possivel criar o arquivo %s.\n",
            nome_arquivo
        );

        return 0;
    }

    for (int y = 0; y < altura; y++) {

        for (int x = 0; x < largura; x++) {

            int indice =
                y * largura + x;

            fprintf(
                arquivo,
                "%d",
                pixels[indice]
            );

            if (x < largura - 1) {
                fprintf(arquivo, " ");
            }
        }

        fprintf(arquivo, "\n");
    }

    fclose(arquivo);

    return 1;
}
double obter_tempo(void)
{
    struct timespec tempo;

    clock_gettime(
        CLOCK_MONOTONIC,
        &tempo
    );

    return
        (double)tempo.tv_sec +
        (double)tempo.tv_nsec / 1000000000.0;
}

int main(int argc, char *argv[]){
    int largura;
    int altura;
    int max_iteracoes;
    int num_threads;

    size_t quantidade_pixels;

    int *pixels;

    char nome_serial[256];
    char nome_openmp[256];
    char nome_pthreads1[256];
    char nome_pthreads2[256];

    FILE *times;

    double inicio;
    double fim;

    double tempo_serial;
    double tempo_openmp;
    double tempo_pthreads1;
    double tempo_pthreads2;
    if(argc != 5){
        fprintf(stderr, "Uso: %s [largura] [altura] [max_iteracoes] [num_threads]\n", argv[0]);
        return 1;
    }
    
    if (!converter_int(argv[1], &largura) ||
        !converter_int(argv[2], &altura) ||
        !converter_int(argv[3], &max_iteracoes) ||
        !converter_int(argv[4], &num_threads)) {
        fprintf(stderr, "Erro: parametros invalidos.\n");
        return 1;
    }
    if(largura < 2 || altura < 2){
        fprintf(stderr, "Erro: largura e altura devem ser maiores que 1.\n");
        return 1;
    }

    quantidade_pixels = (size_t)largura * (size_t)altura;

    if(quantidade_pixels > SIZE_MAX / sizeof(int)) {
        fprintf(stderr, "Erro: tamanho da imagem muito grande.\n");
        return 1;
    }
    pixels = malloc(quantidade_pixels * sizeof(int));
    if(pixels == NULL){
        fprintf(stderr, "Erro: falha na alocacao de memoria.\n");
        return 1;
    }
    snprintf(
        nome_serial,
        sizeof(nome_serial),
        "mandelbrot_%s_serial.pgm",
        LOGIN
    );

    snprintf(
        nome_openmp,
        sizeof(nome_openmp),
        "mandelbrot_%s_openmp.pgm",
        LOGIN
    );

    snprintf(
        nome_pthreads1,
        sizeof(nome_pthreads1),
        "mandelbrot_%s_pthreads1.pgm",
        LOGIN
    );

    snprintf(
        nome_pthreads2,
        sizeof(nome_pthreads2),
        "mandelbrot_%s_pthreads2.pgm",
        LOGIN
    );
    inicio = obter_tempo();
    mandelbrot_serial(pixels, largura, altura, max_iteracoes);
    fim = obter_tempo();
    tempo_serial = fim - inicio;
    if(!salvar_imagem(nome_serial, pixels, largura, altura)){
        free(pixels);
        return 1;
    
    }
    
    return 0;
    }
