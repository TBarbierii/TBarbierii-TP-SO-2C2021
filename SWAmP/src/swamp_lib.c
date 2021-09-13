#include "swamp.h"
#include "swamp_lib.h"

void obtenerValoresDelConfig(t_config* configActual){

    int contador = 0;

    ip_swap = config_get_string_value(configActual, "IP");
    puerto_swap = config_get_int_value(configActual, "PUERTO");
    tamanio_swap = config_get_int_value(configActual, "TAMANIO_SWAP");
    tamanio_pagina = config_get_int_value(configActual, "TAMANIO_PAGINA");
    char** file_swap = config_get_array_value(configActual, "ARCHIVOS_SWAP");
    marcos_maximos = config_get_int_value(configActual, "MARCOS_MAXIMOS");
    retardo_swap = config_get_int_value(configActual, "RETARDO_SWAP");
    
    t_list* archivos_swap = list_create();

    while(file_swap[contador] != NULL) {
        list_add(archivos_swap, file_swap[contador]);
        contador++;
    }

    crear_archivos_swap(archivos_swap);

    list_destroy(archivos_swap);
    free(file_swap);
}

void crear_archivos_swap(t_list* archivos_swap) {

    while(! list_is_empty(archivos_swap)) {
        char* path_swap = (char*) list_remove(archivos_swap, 0);

        int fd = open(path_swap, O_CREAT | O_RDWR, (mode_t) 0777);

        truncate(path_swap, tamanio_swap);

        swap_file = mmap(NULL, tamanio_swap, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

        int estado = stat(path_swap);

    }



}


