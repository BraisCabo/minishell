#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "parser.h"

// Estructura para guardar el jobs
typedef struct
{
    char instruccion[1024]; // String de la instruccion
    int tamaño;             // Numero de comandos
    pid_t pids[50];         // array de pids
    int comprobatorio[50];  // array que comprueba que pid ha terminado
} jobs;

jobs comandoAMatar; // Variable global que servirá para cuando el usuario quiera traer un comando del fg y ejecute la señal sign

// En esta función se comprueba que el número que nos han introducido para el umask sea válido
bool checkOctal(char *n)
{
    int i;   // Variable para el for
    int aux; // Variable para convertir el char a int
    // Aquí se comprueba que tenga 4 cifras o menos
    if (strlen(n) > 4)
    {
        return false;
    }
    // En el for se comprueba que todo sus dígitos sean menores que 8
    for (i = strlen(n) - 1; i >= 0; i--)
    {
        aux = n[i] - 48;
        if ((aux < 0) || (aux > 7))
        {
            return false;
        }
    }
    return true;
}

// Imprime un prompt personalizado
void prompt(char *us, char *wd, char *hostname)
{
    printf("\033[0;32m"); // verde
    printf("%s@%s", us, hostname);
    printf("\033[0m"); // blanco
    printf(":");
    printf("\033[0;34m"); // azul
    printf("%s", wd);
    printf("\033[0m"); // blanco
    printf("$ ");
}

// Redirecciona la entrada y salida estándar según se pida en la instrucción
void redirect(char *in, char *ou, char *err, bool c1)
{
    FILE *fi, *fo, *fe; // Variables para guardar el FILE de la entrada estándar, la salida estándar y el error estándar
    if (in != NULL)     // Comprueba si se ha pedido que se redireccione la entrada
    {
        fi = fopen(in, "r");
        if (fi != NULL) // Si existe la dirección redirecciona
        {
            dup2(fileno(fi), STDIN_FILENO);
        }
        else // Si no existe salta un error
        {
            fprintf(stderr, "%s: no existe el archivo o el directorio\n", in);
            exit(1);
        }
        fclose(fi);
    }
    if (c1 && ou != NULL) // Comprueba si se ha pedido que se redireccione la salida estandar
    {
        fo = fopen(ou, "w");
        dup2(fileno(fo), STDOUT_FILENO);
        fclose(fo);
    }
    if (c1 && err != NULL) // Comprueba si se ha pedido que se redireccione la salida de error
    {
        fe = fopen(err, "w");
        dup2(fileno(fe), STDERR_FILENO);
        fclose(fe);
    }
}

// Sirve para reprogramar el ctrl + c si no se está ejecutando nada en fg
void crlc()
{
    char wd[1024], us[1024], hostname[1024]; // Variables del prompt
    getcwd(wd, sizeof(wd));                  // Incializamos las variables del prompt
    gethostname(hostname, sizeof(hostname));
    getlogin_r(us, sizeof(us));
    printf("\n");
    prompt(us, wd, hostname);
    fflush(stdout);
}
// Sirve para reprogramar el ctrl + c si se está ejecutando algo en fg
void crlc2()
{
    printf("\n");
}

// Sirve para reprogramar el ctrl + c si se acaba de traer algo del bg al fg
void crlc3()
{
    int i;
    for (i = 0; i < comandoAMatar.tamaño; i++) // mata todos los pid del comando
    {
        kill(comandoAMatar.pids[i], 9);
    }
    printf("\n");
}

// Ejecuta n comandos, recibe los comandos a ejecutar, el estado del jobs y el número de comandos en bg
void executeNComands(tline *line, jobs **lljobs, int num)
{
    // Declaraciones
    jobs *ljobs;             // Va a servir para guardar la ed de procesos en bg
    ljobs = (jobs *)*lljobs; // Accedemos a la memoria de la ed para los procesos en bg
    pid_t pid;               // Variable para ir guardando los pid de los hijos
    int ppar[2], pimpar[2];  // Pipes que vamos a usar para comunicar a los hijos
    int j = 0;               // Variable para saber si ese hijo es par o impar
    pid_t *pidAux = malloc(line->ncommands);
    // Ejecución
    if (line->background == 1)
    {
        signal(SIGINT, SIG_IGN);
    }
    else
    {
        signal(SIGINT, crlc2);
    }
    if (line->ncommands > 1) // Si hay más de un comando inicializamos la primera pipe
    {
        pipe(ppar);
    }
    // signal(SIGINT, crlc2); // Reprogramamos la señal ctrl + c para que haga un print(\n)
    pid = fork(); // Creamos el hijo
    if (pid == 0) // Si es el hijo ejecutamos
    {
        redirect(line->redirect_input, line->redirect_output, line->redirect_error, j == (line->ncommands - 1)); // Redirijimos la entrada y salida etandar si es necesario
        if (line->ncommands > 1)                                                                                 // Si hay más de un comando
        {
            close(ppar[0]);
            dup2(ppar[1], STDOUT_FILENO); // Cambiamos el descriptor de fichero de la salida estándar por la pipe
            close(ppar[1]);
        }
        execvp(line->commands[0].argv[0], line->commands[0].argv);                      // Ejecutamos el comando
        fprintf(stderr, "%s: No se encuentra el mandato\n", line->commands[j].argv[0]); // Si llegamos aquí es que ha habido un error, lo lanzamos y nos salimos
        exit(1);
    }
    else
    {
        if (line->background == 1) // Comprueba si se ha pedido que se ejecuten en bg los procesos
        {
            signal(SIGINT, SIG_IGN);             // Ignoramos la señal ctrl + c
            ljobs[num].tamaño = line->ncommands; // Guardamos el numero de comandos en nuestra ed de procesos en bg
            ljobs[num].pids[0] = pid;            // Guardamos el pid del primer hijo
        }
        else // Si no se ha pedido que se haga en background, guardamos en una ed local
        {
            pidAux[j] = pid;
        }
        if (line->ncommands > 1) // Si hay más de un comando cerramos la tubería que estaba abierta y no se va a usar
        {
            close(ppar[1]);
        }
        for (j = 1; j < line->ncommands; j++) // Para todos los comandos que quedaban por ejecutar
        {
            if (j % 2 == 0) // Si el numero de comando es par se va a abrir la tuberia ppar para escribir en ella y leer de pimpar
            {
                pipe(ppar);
            }
            else // Si el numero de comando es impar se va a abrir la tuberia pimpar para escribir en ella y leer de la tuberia ppar
            {
                pipe(pimpar);
            }
            pid = fork(); // Creamos un hijo
            if (pid == 0) // Hace que lo ejecute el hijo
            {
                redirect(NULL, line->redirect_output, line->redirect_error, j == (line->ncommands - 1)); // Redireccionamos entrada y salida en caso de ser necesario
                if (j % 2 == 0)                                                                          // Si el numero de comando es par
                {
                    dup2(pimpar[0], STDIN_FILENO); // Cambiamos el descriptor de fichero de la entrada estándar por el que estaba en la pipe pimpar
                    if (j < line->ncommands - 1)   // Si el comando no es el ultimo
                    {
                        dup2(ppar[1], STDOUT_FILENO); // Cambiamos el descriptor de fichero de la salida estándar y lo guardamos en ppar
                    }
                }
                else // Si el numero de comando es impar se hace lo mismo que si es par pero leyendo de ppar y escribiendo en pimpar
                {
                    dup2(ppar[0], STDIN_FILENO);
                    if (j < line->ncommands - 1)
                    {
                        dup2(pimpar[1], STDOUT_FILENO);
                    }
                }
                close(ppar[0]); // Cerramos todas las tuberías para que no se quede nada abierto
                close(pimpar[1]);
                close(pimpar[0]);
                close(ppar[1]);
                execvp(line->commands[j].argv[0], line->commands[j].argv);                      // Ejecutamos el comando correspondiente
                fprintf(stderr, "%s: No se encuentra el mandato\n", line->commands[j].argv[0]); // Si hemos llegado aquí es que algo ha salido mal, tenemos que lanzar un error
                exit(1);
            }
            else // Este código lo ejecuta el padre
            {
                if (j % 2 == 0) // Si el número del último proceso hijo es par
                {
                    dup2(pimpar[0], ppar[1]); // Apuntamos la entrada estándar del siguiente proceso hijo (ppar) a la salida del proceso hijo que cabamos de crear (pimpar)
                    close(pimpar[0]);         // Cerramos las tuberias que ya no necesitamos
                    close(ppar[1]);
                    if (j == (line->ncommands - 1)) // Si es el último comando
                    {
                        close(ppar[0]); // Cerramos esta tubería ya que no vamos a tener que ningún proceso va a tener que leer la salida de este
                    }
                }
                else // Si el numero de proceso es impar se hace lo mismo pero intercambiando ppar y pimpar
                {
                    dup2(ppar[0], pimpar[1]);
                    close(ppar[0]);
                    close(pimpar[1]);
                    if (j == (line->ncommands - 1))
                    {
                        close(pimpar[0]);
                    }
                }
                if (line->background == 1) // Si hay que ejecutar en bg
                {
                    ljobs[num].pids[j] = pid; // Guardamos el pid del hijo en una ed
                }
                else // Si hay que ejecutar en fg
                {
                    pidAux[j] = pid; // Guardamos el pid del hijo en una ed auxiliar
                }
            }
        }

        if (line->background == 0) // Si ejecutamos en fg
        {
            for (j = 0; j < line->ncommands; j++) // Para cada proceso guardado en la ed auxiliar
            {
                waitpid(pidAux[j], NULL, 0); // Lo esperamos hasta que acabe
            }
        }
    }
    free(pidAux);
    signal(SIGINT, crlc); // Reprogramamos la señal ctrl + c y la dejamos como estaba antes de la función
}

// Pasa un comando a foreground
void fgCommand(tcommand *com, jobs ljobs[], int numero)
{
    signal(SIGINT, crlc3); // seleccionamos un manejador cuanto se hace crtl+c
    int i, j;              // inicio varibles para los for
    if (com->argc == 1)    // en caso de que no haya argumento realizamos lo siguiente
    {
        comandoAMatar = ljobs[numero - 1];             // Establecemos la variable global por si el usuario quiere matar el proceso que acaba de mandar a fg
        for (i = 0; i < ljobs[numero - 1].tamaño; i++) // esperamos a todas las partes de ultima instuccion
        {
            waitpid(ljobs[numero - 1].pids[i], NULL, 0);
        }
    }
    else
    {
        comandoAMatar = ljobs[atoi(com->argv[1])];             // Establecemos la variable global por si el usuario quiere matar el proceso que acaba de mandar a fg
        for (i = 0; i < ljobs[atoi(com->argv[1])].tamaño; i++) // esperamos a todas las partes de la instuccion seleccionada
        {
            waitpid(ljobs[atoi(com->argv[1])].pids[i], NULL, 0);
        }
        for (j = i; j < numero; j++) // actuliazmos las lista de las instrucciones
        {
            ljobs[j] = ljobs[j + 1];
        }
    }
    signal(SIGINT, crlc);
}

// Cambia el directorio de trabajo
void cdCommand(tcommand *com)
{
    if (com->argc == 1) // Si no se reciben parámetros lo cambia por el /home/user
    {
        chdir(getenv("HOME"));
    }
    else if (com->argc == 2)
    {
        if (chdir(com->argv[1]) != 0) // Si el directorio es incorrecto salta un error
        {
            fprintf(stderr, "cd: %s no es un directorio\n", com->argv[1]);
        }
    }
    else // Si se recibe más de un parámetro salta un error
    {
        fprintf(stderr, "cd: demasiados argumentos\n");
    }
}

// Ejecuta el comando umask
void umaskCommand(tcommand *com, mode_t *mascara)
{
    int mascaraAux = *mascara;
    int numero = 4;
    int octal;
    if (com->argc == 1) // Si se pasa sin parametros mostramos la máscara actual
    {
        if (mascaraAux == 0)
        {
            numero--;
        }
        while (mascaraAux > 0) // Calculamos el número de ceros a imprimir
        {
            mascaraAux = mascaraAux / 10;
            numero--;
        }
        while (numero > 0) // Imprimimos el número de ceros necesario
        {
            printf("0");
            numero--;
        }
        printf("%i \n", *mascara);
    }
    else // Si recibe algún parámetro cambiamos la máscara
    {
        if (checkOctal(com->argv[1])) // Si el parámetro es un octal correcto cambiamos la máscara
        {
            *mascara = atoi(com->argv[1]);
            sscanf(com->argv[1], "%o", &octal); // Calculamos el octal del número que nos han pasado
            umask(octal);                       // Cambiamos la máscara
        }
        else // Si el parámetro no es un octal correcto devolvemos un error
        {
            fprintf(stderr, "%s: no valido debe ser un número octal de 4 cifras o menos\n", com->argv[1]);
            return;
        }
    }
}

// Comando para cerrar el programa
void exitCommand(int numero, jobs ljobs[])
{
    int i, j;                    // Variables para los for
    for (j = 0; j < numero; j++) // Cerramos todos los procesos en bg
    {
        for (i = 0; i < ljobs[j].tamaño; i++)
        {
            kill(ljobs[j].pids[i], 9);
        }
    }
    free(ljobs); // Liberamos la ed de procesos en background
    exit(0);     // Nos salimos de la minishell
}

// Función para mostrar los procesos en bg
void mostrarjobs(jobs ljobs[], int *numero, int control)
{
    int i, j, p, cont; // variables de los for, indice de los cambios y contador
    int cambio[50];    // array de pids que han terminado
    p = 0;
    for (i = 0; i < (*numero); i++) // comprobamos todas las instrucciones en bg
    {
        cont = 0;
        for (j = 0; j < ljobs[i].tamaño; j++) // comprobamos todos las partes de las instrucciones
        {
            if ((waitpid(ljobs[i].pids[j], NULL, WNOHANG) == ljobs[i].pids[j]) || (ljobs[i].comprobatorio[j])) // compruebamos si un pid ha terminado
            {
                cont++;
                ljobs[i].comprobatorio[j] = 1;
            }
            else
            {
                if (control) // control es una variable que se activara en el jobs pero no al combrobar despues de una instruccion si algo ha acabado
                {
                    printf("[%d] Ejecutando        %s", i, ljobs[i].instruccion);
                }
                break;
            }
        }
        if (cont == ljobs[i].tamaño)
        {
            printf("[%d]  Hecho        %s", i, ljobs[i].instruccion);
            cambio[p] = i;
            p++;
        }
    }
    if (p > 0)
    {
        while (p > 0)
        {
            for (j = cambio[0]; j <= (*numero); j++)
            {
                ljobs[j] = ljobs[j + 1];
            }
            *numero = *numero - 1;
            p--;
        }
    }
}

int main(void)
{
    // Declaraciones de las variables que vamos a necesitar
    char buffer[1024];                       // Buffer en el que vamos a ir guardando la entrada estandar
    char wd[1024], us[1024], hostname[1024]; // Guardamos variables para moestrar el prompt
    tline *line;                             // Vamos a ir guardando los comandos tokenizados en esta variable
    jobs *ljobs = malloc(sizeof(jobs) * 50); // ED para guardar los procesos en bg
    int numero = 0;                          // Numero de proceso en bg
    mode_t mascara = 22;                     // Máscara actual
    umask(18);                               // Ponemos la máscara a la máscara por defecto de linux

    // Lógica de programa
    signal(SIGINT, crlc); // Reprogramamos la señal ctrl + c

    getcwd(wd, sizeof(wd));                  // Inicializamos variables del prompt
    gethostname(hostname, sizeof(hostname)); //
    getlogin_r(us, sizeof(us));

    prompt(us, wd, hostname); // Imprimimos el prompt

    while (fgets(buffer, 1024, stdin)) // Bucle para leer la entrada estándar
    {
        line = tokenize(buffer);                  // Tokenizamos la entrada estándar
        if (line != NULL && line->ncommands != 0) // Comprobámos que la línea sea correcta y también que no sea solo un intro
        {
            if (line->ncommands == 1) // Comprobamos que se haya introducido un comando
            {
                if (strcmp(line->commands[0].argv[0], "cd") == 0) // Entramos por este if si el comando es un cd
                {
                    cdCommand(line->commands);
                    getcwd(wd, sizeof(wd));
                }
                else if (strcmp(line->commands[0].argv[0], "umask") == 0) // Entramos por este if si el comando es un umask
                {
                    umaskCommand(line->commands, &mascara);
                }
                else if (strcmp(line->commands[0].argv[0], "jobs") == 0) // Entramos por este si el comando es un jobs
                {
                    mostrarjobs(ljobs, &numero, 1);
                }
                else if (strcmp(line->commands[0].argv[0], "fg") == 0) // Entramos por este si el comando es un fg
                {
                    if (numero > 0)
                    {
                        if (line->commands->argc > 1)
                        {
                            if (atoi(line->commands->argv[1]) <= numero - 1)
                            {
                                fgCommand(line->commands, ljobs, numero);
                                numero--;
                            }
                            else
                            {
                                printf("fg: no existe ese trabajo \n");
                            }
                        }
                        else
                        {
                            fgCommand(line->commands, ljobs, numero);
                            numero--;
                        }
                    }
                    else
                    {
                        printf("fg: no existe ese trabajo \n");
                    }
                }
                else if (strcmp(line->commands[0].argv[0], "exit") == 0) // Entramos por este si el comando es un jobs
                {
                    exitCommand(numero, ljobs);
                }
                else // Si no concuerda con ninguno de los anteriores ejecutamos los comandos externos de la shell
                {
                    executeNComands(line, &ljobs, numero);
                    if (line->background == 1)
                    {

                        printf("[%d] %d \n", numero, ljobs[numero].pids[0]);
                        strcpy(ljobs[numero].instruccion, buffer);
                        numero++;
                    }
                }
            }
            else // Si hay más de un comando, ejecutamos los externos de la shell
            {
                executeNComands(line, &ljobs, numero);
                if (line->background == 1)
                {
                    printf("[%d] %d \n", numero, ljobs[numero].pids[0]);
                    strcpy(ljobs[numero].instruccion, buffer);
                    numero++;
                }
            }
        }
        mostrarjobs(ljobs, &numero, 0); // Muestra si hay algún proceso en bg que haya terminado
        prompt(us, wd, hostname);       // Muestra el prompt
    }
}