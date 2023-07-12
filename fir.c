#include <stdio.h>
#include <math.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#define FILE_SIZE_TYPE int
#define SIGNAL_TYPE float

void printDiagram(const char *name_input_bin, int x1, int x, double s)
{
    //Инициализация файла
    FILE *f_is = fopen(name_input_bin, "r+b");

    //Проверка файловых ошибок и вывод ошибки
    if (!f_is)
    {
        perror("Open file error");
        return;
    }

    //Чтение длины файла
    FILE_SIZE_TYPE fsize_is;
    fread(&fsize_is, sizeof(FILE_SIZE_TYPE), 1, f_is);

    //Проверка возможной ошибки выхода из диапазона
    if (fsize_is <= x1)
    {
        printf("Starting point has gone beyond the signal range\n");
        return;
    }

    //Замена области отрисовки
    int buff = x1 + x >= fsize_is;
    if (buff || x == 0)
    {
        x = fsize_is - x1;
        if (buff)
        {
            printf("The required range went beyond the existing signal range\n");
        }
    }

    //Инициализация массива и выгрузка в него сигнала
    SIGNAL_TYPE *y = malloc(sizeof(SIGNAL_TYPE) * fsize_is);
    fpos_t pos = sizeof(FILE_SIZE_TYPE) + x1 * sizeof(SIGNAL_TYPE);
    fsetpos(f_is, &pos);
    fread(y, sizeof(SIGNAL_TYPE), x, f_is);

    //Поиск минимального и максимального значения сигнала
    SIGNAL_TYPE fmin = y[0], fmax = y[0];
    for (int i = 0; i < x; i++)
    {
        if (fmin > y[i])
        {
            fmin = y[i];
        }
        else if (fmax < y[i])
        {
            fmax = y[i];
        }
    }

    //Вычисление графика в высоту
    int max = round(fmax * s), min = round(fmin * s);
    int line_size = fabs(max - min);

    //Вывод графика в консоль
    for (int i = 0; i < x; i++)
    {
        int j = 0;
        buff = fabs(max - (int)round(y[i] * s));
        while (j < buff)
        {
            putchar('_');
            j++;
        }
        putchar('\n');
    }
    free(y);
}

void fir(const char *name_input_bin, const char *name_output_bin, const char *name_ir_txt)
{
    //Инициализация файлов
    FILE *f_is, *f_os, *f_ir;
    f_is = fopen(name_input_bin, "r+b");
    f_os = fopen(name_output_bin, "w+b");
    f_ir = fopen(name_ir_txt, "r");

    //Проверка на ошибку и вывод ошибки
    if (!(f_is && f_os && f_ir))
    {
        perror("Open files error");
        return;
    }

    //Запись длины всех файлов
    FILE_SIZE_TYPE fsize_is, fsize_ir, fsize_os;
    fread(&fsize_is, sizeof(FILE_SIZE_TYPE), 1, f_is);
    fscanf(f_ir, "%d", &fsize_ir);
    fsize_os = fsize_is - fsize_ir + 1;

    //Проверка на ошибку (длина сигнала меньше длины коэффициентов фильтра)
    if (fsize_os <= 0)
    {
        printf("The signal is less than the filter coefficients\n");
        return;
    }

    //Инициализация массивов для выгрузки данных с файлов и записи
    SIGNAL_TYPE *is, *os, *ir;
    is = malloc(fsize_is * sizeof(SIGNAL_TYPE));
    os = malloc(fsize_os * sizeof(SIGNAL_TYPE));
    ir = malloc(fsize_ir * sizeof(SIGNAL_TYPE));

    //Выгрузка файлов в массивы
    fread(is, sizeof(SIGNAL_TYPE), fsize_is, f_is);
    for (int i = 0; i < fsize_ir; i++)
    {
        fscanf(f_ir, "%f", &ir[i]);
    }
    fclose(f_is);
    fclose(f_ir);

    //Алгоритм КИХ-фильтра
    SIGNAL_TYPE *isn = is;
    for (int i = 0; i < fsize_os; i++)
    {
        os[i] = 0.0;
        for (int j = 0; j < fsize_ir; j++)
        {
            os[i] += isn[j] * ir[j];
        }
        isn = &isn[1];
    }

    //Запись результата в фаил
    fwrite(&fsize_os, sizeof(FILE_SIZE_TYPE), 1, f_os);
    fwrite(os, sizeof(SIGNAL_TYPE), fsize_os, f_os);

    //Зачистка ненужного
    fclose(f_ir);
    free(is);
    free(os);
    free(ir);
}

void create_is(const char *name_bin, const int s)
{
    //Инициализация файла
    FILE *file = fopen(name_bin, "w+b");

    //Проверка на файловую ошибку и вывод ошибки
    if (!file)
    {
        perror("Open file error");
        return;
    }

    //Инициализация массивов
    SIGNAL_TYPE a[5], f[5], fr[5], y[s];

    //Заполнение массива частоты
    fr[0] = (rand() % 10) / 100.0;
    while ((fr[1] = (rand() % 10) / 100.0) == fr[0])
    {
    }
    for (int i = 0; i < 5; i++)
    {
        fr[i] = rand() % 2 ? fr[0] : fr[1];
    }

    //Заполнение массивов фазы и амплитуды
    for (int i = 0; i < 5; i++)
    {
        a[i] = (rand() % 10) / 100.0;
        f[i] = (rand() % 100) / 50.0;
    }

    //Создание полигармонического сигнала из 5 гармонических
    for (int i = 0; i < s; i++)
    {
        y[i] = 0.0;
        for (int j = 0; j < 5; j++)
        {
            y[i] += a[j] * sin(fr[j] * (SIGNAL_TYPE)(j + 1) * (SIGNAL_TYPE)i + f[j]);
        }
    }

    //Запись сигнала в фаил
    fwrite(&s, sizeof(FILE_SIZE_TYPE), 1, file);
    fwrite(y, sizeof(SIGNAL_TYPE), s, file);
    fclose(file);
}

void create_ir(const char *name_txt, const int n)
{
    //Инициализация файла
    FILE *file = fopen(name_txt, "w");

    //Проверка на файловую ошибку и вывод ошибки
    if (!file)
    {
        perror("Open file error");
        return;
    }

    //Запись импульсных характеристикик в фаил
    fprintf(file, "%lu\n", n);
    for (int i = 0; i < n; i++)
    {
        fprintf(file, "%lf\n", 1.0 / (double)n);
    }
    fclose(file);
}

void main(int argc, char *argv[])
{
    //Инициализация переменных с нейтральным значением
    char *i = NULL, *o = NULL, *f = NULL, *c = NULL, *g = NULL, *d = NULL;
    int s = 0, n = 0, x = 0, p = 0;
    double l = 1.0;
    int arg;

    //Запрет вывода optarg ошибок в консоль
    opterr = 0;

    //Поиск аргументов и запись в переменные
    while ((arg = getopt(argc, argv, "i:o:f:g:s:c:n:d:x:p:l:")) != -1)
    {
        switch (arg)
        {

        case 'i':
            i = optarg;
            break;
        case 'o':
            o = optarg;
            break;
        case 'f':
            f = optarg;
            break;

        case 'g':
            g = optarg;
            break;
        case 's':
            s = atoi(optarg); // int
            break;

        case 'c':
            c = optarg;
            break;
        case 'n':
            n = atoi(optarg); // int
            break;

        case 'd':
            d = optarg;
            break;
        case 'x':
            x = atoi(optarg); // int
            break;
        case 'p':
            p = atoi(optarg); // int
            break;
        case 'l':
            l = strtod(optarg, NULL); // double
            break;

        default:
            break;
        }
    }

    //Запуск создания сигнала
    if (g && s)
    {
        printf("Create signal...\n");
        create_is(g, s);
        printf("Completion\n");
    }

    //Запуск создания импульсной характеристики
    if (c && n)
    {
        printf("Create impulse response...\n");
        create_ir(c, n);
        printf("Completion\n");
    }

    //Запуск КИХ-фильтра
    if (i && o && f)
    {
        printf("Launch filter with finite impulse response...\n");
        fir(i, o, f);
        printf("Completion\n");
    }

    //Отрисовка графика сигнала
    if (d)
    {
        printf("Print graph signal...\n");
        printDiagram(d, x, p, l);
        printf("Completion\n");
    }

}

