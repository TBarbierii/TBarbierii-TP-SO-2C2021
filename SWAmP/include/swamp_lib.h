#ifndef SWAMP_LIB_H
#define SWAMP_LIB_H

#include <stdio.h>
#include <commons/log.h>
#include <commons/config.h>
#include <stdbool.h>
#include "shared_utils.h"
#include <semaphore.h>
#include <commons/collections/list.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>


/* variables obtenidas del config*/

char* ip_swap;
int puerto_swap;
int tamanio_swap;
int tamanio_pagina;
int marcos_maximos;
int retardo_swap;


t_list* lista_swap_files;

void obtenerValoresDelConfig(t_config* configActual);
void crear_archivos_swap(t_list* archivos_swap);


typedef struct swap_files {
    char* path;
    int fd_swap;
    void* swap_file;
    t_list* particiones_swap;
};

typedef struct particion {
    int pid;
    int numero_marco;
    int esta_libre;
};

#endif