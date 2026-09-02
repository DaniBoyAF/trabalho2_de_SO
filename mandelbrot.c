#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <time.h>
#include <pthread.h>
#include <omp.h>

#define LOGIN "abc123"

typedef struct {
    int proxima_linha;
    pthread_mutex_t mutex;
} TrabalhoDinamico;

typedef struct {
    int *pixels;
    int largura;
    int altura;
    int max_iteracoes;
    int num_threads;
    int thread_id;
    TrabalhoDinamico *trabalho; 
} ThreadData;
int converter_int(const char *str, int *valor)
{
    char *fim;
    long resultado;

    errno = 0;
    resultado = strtol(str, &fim, 10);

    if (errno != 0 || *fim != '\0' ||
        resultado < INT_MIN || resultado > INT_MAX) {
        return 0;
    }

    if (resultado < 1) {
        return 0;
    }
    *valor = (int)resultado;
    return 1;
}
int calcular_mandelbrot(double x0, double y0, int max_iteracoes)
{
    double z_real = 0.0;
    double z_imag = 0.0;
    int iteracoes;

    for (iteracoes = 0; iteracoes < max_iteracoes; iteracoes++) {   
        double novo_zreal = z_real * z_real - z_imag * z_imag + x0;
        double novo_zimag = 2.0 * z_real * z_imag + y0;
        z_real = novo_zreal;
        z_imag = novo_zimag;
        if (z_real * z_real + z_imag * z_imag > 4.0) {
            break;
        }
    }                         
    return iteracoes;
}     
double converter_real(int x, int largura)
{
    return -2.0 + (double)x * 3.0 / (double)(largura - 1);
}

double converter_imag(int y, int altura)
{
    return -1.5 + (double)y * 3.0 / (double)(altura - 1);
}
int calcular_intensidade(int iteracoes, int max_iteracoes)
{
    if (iteracoes == max_iteracoes) {
        return 0;
    }
    return (int)(255.0 * iteracoes / max_iteracoes);
}
void mandelbrot_serial(int *pixels, int largura, int altura, int max_iteracoes)
{
    int y;
    for (y = 0; y < altura; y++) {
        int x;
        double c_imag = converter_imag(y, altura);
        for (x = 0; x < largura; x++) {
            double c_real = converter_real(x, largura);
            int iteracoes  = calcular_mandelbrot(c_real, c_imag, max_iteracoes);
            int intensidade = calcular_intensidade(iteracoes, max_iteracoes);
            pixels[y * largura + x] = intensidade;
        }
    }
}
void *pthread_blocos(void *arg)
{
    ThreadData *dados = (ThreadData *)arg;

    int inicio = dados->thread_id * (dados->altura / dados->num_threads);
    int fim    = (dados->thread_id + 1) * (dados->altura / dados->num_threads);

    /* garante que a última thread processa as linhas restantes */
    if (dados->thread_id == dados->num_threads - 1) {
        fim = dados->altura;
    }

    for (int y = inicio; y < fim; y++) {
        double c_imag = converter_imag(y, dados->altura);
        for (int x = 0; x < dados->largura; x++) {
            double c_real   = converter_real(x, dados->largura);
            int iteracoes   = calcular_mandelbrot(c_real, c_imag, dados->max_iteracoes);
            int indice      = y * dados->largura + x;
            dados->pixels[indice] = calcular_intensidade(iteracoes, dados->max_iteracoes);
        }
    }
    return NULL;
}
void *pthread_dinamico(void *arg)
{
    ThreadData *dados = (ThreadData *)arg;
    TrabalhoDinamico *trabalho = dados->trabalho;

    for (;;) {
        pthread_mutex_lock(&trabalho->mutex);
        int y = trabalho->proxima_linha++;
        pthread_mutex_unlock(&trabalho->mutex);

        if (y >= dados->altura) {
            break;
        }

        double c_imag = converter_imag(y, dados->altura);
        for (int x = 0; x < dados->largura; x++) {
            double c_real   = converter_real(x, dados->largura);
            int iteracoes   = calcular_mandelbrot(c_real, c_imag, dados->max_iteracoes);
            int indice      = y * dados->largura + x;
            dados->pixels[indice] = calcular_intensidade(iteracoes, dados->max_iteracoes);
        }
    }
    return NULL;
}

/* ────────────────────────────────────────────────────
   Lança e aguarda todas as threads pthread
   estrategia 1 = blocos (estático) · 2 = dinâmico (fila de trabalho)
───────────────────────────────────────────────────── */
int executar_pthreads(int *pixels,
                      int largura,
                      int altura,
                      int max_iteracoes,
                      int num_threads,
                      int estrategia)
{
    pthread_t  *threads = malloc(num_threads * sizeof(pthread_t));
    ThreadData *dados   = malloc(num_threads * sizeof(ThreadData));

    if (threads == NULL || dados == NULL) {
        free(threads);
        free(dados);
        return 0;
    }

    /* Estado compartilhado só é usado pela estratégia dinâmica, mas é
       inicializado sempre — custo irrisório e simplifica o código. */
    TrabalhoDinamico trabalho;
    trabalho.proxima_linha = 0;
    pthread_mutex_init(&trabalho.mutex, NULL);

    for (int i = 0; i < num_threads; i++) {
        dados[i].pixels        = pixels;
        dados[i].largura       = largura;
        dados[i].altura        = altura;
        dados[i].max_iteracoes = max_iteracoes;
        dados[i].num_threads   = num_threads;
        dados[i].thread_id     = i;
        dados[i].trabalho      = &trabalho;

        void *(*funcao)(void *) = (estrategia == 1) ? pthread_blocos : pthread_dinamico;

        int resultado = pthread_create(&threads[i], NULL, funcao, &dados[i]);
        if (resultado != 0) {
            fprintf(stderr, "Erro: falha na criacao da thread.\n");
            for (int j = 0; j < i; j++) {
                pthread_join(threads[j], NULL);
            }
            free(threads);
            free(dados);
            pthread_mutex_destroy(&trabalho.mutex);
            return 0;
        }
    }

    for (int i = 0; i < num_threads; i++) {
        if (pthread_join(threads[i], NULL) != 0) {
            fprintf(stderr, "Erro: falha ao aguardar thread.\n");
            free(threads);
            free(dados);
            pthread_mutex_destroy(&trabalho.mutex);
            return 0;
        }
    }

    free(threads);
    free(dados);
    pthread_mutex_destroy(&trabalho.mutex);
    return 1;
}

/* ────────────────────────────────────────────────────
   Salva pixels no formato PGM (P2 — texto)
   FIX 7: cabeçalho PGM adicionado para arquivo válido
───────────────────────────────────────────────────── */
int salvar_imagem(const char *nome_arquivo,
                  int *pixels,
                  int largura,
                  int altura)
{
    FILE *arquivo = fopen(nome_arquivo, "w");
    if (arquivo == NULL) {
        fprintf(stderr,
                "Erro: nao foi possivel criar o arquivo %s.\n",
                nome_arquivo);
        return 0;
    }

    /* FIX 7: cabeçalho obrigatório do formato PGM */
    fprintf(arquivo, "P2\n%d %d\n255\n", largura, altura);

    for (int y = 0; y < altura; y++) {
        for (int x = 0; x < largura; x++) {
            int indice = y * largura + x;
            fprintf(arquivo, "%d", pixels[indice]);
            if (x < largura - 1) {
                fprintf(arquivo, " ");
            }
        }
        fprintf(arquivo, "\n");
    }

    fclose(arquivo);
    return 1;
}

/* ────────────────────────────────────────────────────
   Retorna tempo monotônico em segundos (double)
───────────────────────────────────────────────────── */
double obter_tempo(void)
{
    struct timespec tempo;
    clock_gettime(CLOCK_MONOTONIC, &tempo);
    return (double)tempo.tv_sec + (double)tempo.tv_nsec / 1000000000.0;
}

/* ────────────────────────────────────────────────────
   Main
───────────────────────────────────────────────────── */
int main(int argc, char *argv[])
{
    int largura, altura, max_iteracoes, num_threads;

    if (argc != 5) {
        fprintf(stderr,
                "Uso: %s [largura] [altura] [max_iteracoes] [num_threads]\n",
                argv[0]);
        return 1;
    }

    if (!converter_int(argv[1], &largura)       ||
        !converter_int(argv[2], &altura)         ||
        !converter_int(argv[3], &max_iteracoes)  ||
        !converter_int(argv[4], &num_threads)) {
        fprintf(stderr, "Erro: parametros invalidos.\n");
        return 1;
    }

    if (largura < 2 || altura < 2) {
        fprintf(stderr, "Erro: largura e altura devem ser maiores que 1.\n");
        return 1;
    }

    size_t quantidade_pixels = (size_t)largura * (size_t)altura;
    if (quantidade_pixels > SIZE_MAX / sizeof(int)) {
        fprintf(stderr, "Erro: tamanho da imagem muito grande.\n");
        return 1;
    }

    int *pixels = malloc(quantidade_pixels * sizeof(int));
    if (pixels == NULL) {
        fprintf(stderr, "Erro: falha na alocacao de memoria.\n");
        return 1;
    }

    /* Nomes dos arquivos de saída */
    char nome_serial[256], nome_openmp[256];
    char nome_pthreads1[256], nome_pthreads2[256];

    snprintf(nome_serial,    sizeof(nome_serial),    "mandelbrot_%s_serial.pgm",    LOGIN);
    snprintf(nome_openmp,    sizeof(nome_openmp),    "mandelbrot_%s_openmp.pgm",    LOGIN);
    snprintf(nome_pthreads1, sizeof(nome_pthreads1), "mandelbrot_%s_pthreads1.pgm", LOGIN);
    snprintf(nome_pthreads2, sizeof(nome_pthreads2), "mandelbrot_%s_pthreads2.pgm", LOGIN);

    double inicio, fim;
    double tempo_serial, tempo_openmp, tempo_pthreads1, tempo_pthreads2;

    /* ── Serial ── FIX 5: executado UMA única vez ── */
    inicio = obter_tempo();
    mandelbrot_serial(pixels, largura, altura, max_iteracoes);
    fim = obter_tempo();
    tempo_serial = fim - inicio;

    if (!salvar_imagem(nome_serial, pixels, largura, altura)) {
        free(pixels);
        return 1;
    }

    /* ── OpenMP ── FIX 4: pragma adicionado ── */
    inicio = obter_tempo();
    #pragma omp parallel for num_threads(num_threads) schedule(static)
    for (int y = 0; y < altura; y++) {
        double c_imag = converter_imag(y, altura);
        for (int x = 0; x < largura; x++) {
            double c_real   = converter_real(x, largura);
            int iteracoes   = calcular_mandelbrot(c_real, c_imag, max_iteracoes);
            int indice      = y * largura + x;
            pixels[indice]  = calcular_intensidade(iteracoes, max_iteracoes);
        }
    }
    fim = obter_tempo();
    tempo_openmp = fim - inicio;

    if (!salvar_imagem(nome_openmp, pixels, largura, altura)) {
        free(pixels);
        return 1;
    }

    /* ── Pthreads — blocos ── */
    inicio = obter_tempo();
    if (!executar_pthreads(pixels, largura, altura, max_iteracoes, num_threads, 1)) {
        free(pixels);
        return 1;
    }
    fim = obter_tempo();
    tempo_pthreads1 = fim - inicio;

    if (!salvar_imagem(nome_pthreads1, pixels, largura, altura)) {
        free(pixels);
        return 1;
    }

    /* ── Pthreads — dinâmico (fila de trabalho) ── */
    inicio = obter_tempo();
    if (!executar_pthreads(pixels, largura, altura, max_iteracoes, num_threads, 2)) {
        free(pixels);
        return 1;
    }
    fim = obter_tempo();
    tempo_pthreads2 = fim - inicio;

    if (!salvar_imagem(nome_pthreads2, pixels, largura, altura)) {
        free(pixels);
        return 1;
    }

    /* ── Salva tempos ── */
    FILE *times = fopen("times.txt", "w");
    if (times == NULL) {
        fprintf(stderr, "Erro: nao foi possivel criar o arquivo times.txt.\n");
        free(pixels);
        return 1;
    }
    fprintf(times, "Tempo serial: %f\n",      tempo_serial);
    fprintf(times, "Tempo OpenMP: %f\n",      tempo_openmp);
    fprintf(times, "Tempo Pthreads 1: %f\n",  tempo_pthreads1);
    fprintf(times, "Tempo Pthreads 2: %f\n",  tempo_pthreads2);
    fclose(times);

    free(pixels);
    return 0;
}