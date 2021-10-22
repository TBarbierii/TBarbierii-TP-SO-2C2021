#include "Memoria.h"

t_config* inicializarConfig(){
    return config_create("cfg/ConfiguracionMemoria.config");
}

void inicializarListas(){
    carpinchos = list_create();
    marcos = list_create();
}

void obtenerValoresDelConfig(t_config* configActual){

    ipSWAmP = config_get_string_value(configActual, "IP_SWAMP");
    puertoSWAmP = config_get_string_value(configActual, "PUERTO_SWAMP");
    tamanio = config_get_int_value(configActual, "TAMANIO");
    tamanioPagina = config_get_int_value(configActual, "TAMANIO_PAGINA");
    algoritmoReemplazoMMU = config_get_string_value(configActual, "ALGORITMO_REEMPLAZO_MMU");
    tipoAsignacion = config_get_string_value(configActual, "TIPO_ASIGNACION");
    marcosMaximos = config_get_int_value(configActual, "MARCOS_POR_PROCESO");
    cantidadEntradasTLB = config_get_int_value(configActual, "CANTIDAD_ENTRADAS_TLB");
    algoritmoReemplazoTLB = config_get_string_value(configActual, "ALGORITMO_REEMPLAZO_TLB");
    retardoAciertoTLB = config_get_int_value(configActual, "RETARDO_ACIERTO_TLB");
    retardoFAlloTLB = config_get_int_value(configActual, "RETARDO_FALLO_TLB");
}

void finalizarConfig(t_config* configUsado){
    config_destroy(configUsado);
}

void inicializarMemoria() {
    memoriaPrincipal = malloc(tamanio);
    crear_marcos();
}

void inicializarTodo(){
    inicializarListas();
    id_pag = 1;
    id_carpincho = 1;
    id_marco = 0;
}

void finalizarMemoria() {
}

int main(){

    t_config* configActual = inicializarConfig();
    obtenerValoresDelConfig(configActual);
    inicializarTodo();
    inicializarMemoria();

    //atender_solicitudes_multihilo();

    t_memalloc *alloc = malloc(sizeof(t_memalloc));
    alloc->pid = 8;
    alloc->tamanio = 45;

    uint32_t direccionLogica = administrar_allocs(alloc);


    void* prueba = "Esta es una prueba de un texto de dos pags\0";

    escribir_memoria(8, direccionLogica, prueba, 44);

    void* hola = leer_memoria(direccionLogica, 8, 44);

    printf("\nSe leyo: %s", (char*)hola);


    finalizarConfig(configActual);
    return 0;
}

