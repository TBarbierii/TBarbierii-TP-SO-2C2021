#include "../../SWAmP/include/swamp_lib.h"
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
#include <string.h>

void enviar_tipo_asignacion(char* tipoAsignacion){//mandar al principio despues de leer config

	uint32_t tipo;

	if(strcmp(tipoAsignacion, "FIJA") == 0) {
		tipo = 1;
	}else if(strcmp(tipoAsignacion, "DINAMICA") == 0) {
		tipo = 0;
	}else{
		tipo = -1;
	}

	t_paquete *paquete = crear_paquete(TIPOASIGNACION);

	paquete->buffer->size = sizeof(uint32_t);
    paquete->buffer->stream = malloc(paquete->buffer->size);
	uint32_t desplazamiento=0;

	memcpy(paquete->buffer->stream + desplazamiento, &(tipo) , sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);

	uint32_t conexionSwamp = crear_conexion("127.0.0.1", "4444");

	enviarPaquete(paquete, conexionSwamp);


}

void enviadoPagina(uint32_t pid, uint32_t id_pagina, void* contenido){

	t_paquete *paquete = crear_paquete(ESCRITURA_PAGINA);
	int tamanioPagina =32;

	paquete->buffer->size = sizeof(uint32_t)*2 + tamanioPagina;
    paquete->buffer->stream = malloc(paquete->buffer->size);
	uint32_t desplazamiento=0;

	memcpy(paquete->buffer->stream + desplazamiento, &(pid) , sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);
	memcpy(paquete->buffer->stream + desplazamiento, &(id_pagina) , sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);
    memcpy(paquete->buffer->stream + desplazamiento, contenido , tamanioPagina);

	uint32_t conexionSwamp = crear_conexion("127.0.0.1", "4444");

	enviarPaquete(paquete, conexionSwamp);



}

void pedirPagina(uint32_t id_pagina, uint32_t pid){

	uint32_t size;

	t_paquete *paquete = crear_paquete(LECTURA_PAGINA);

	paquete->buffer->size = sizeof(uint32_t)*2;
    paquete->buffer->stream = malloc(paquete->buffer->size);
	uint32_t desplazamiento=0;
	
	memcpy(paquete->buffer->stream + desplazamiento, &(pid) , sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);
	memcpy(paquete->buffer->stream + desplazamiento, &(id_pagina) , sizeof(uint32_t));
    
	uint32_t conexionSwamp = crear_conexion("127.0.0.1", "4444");

	enviarPaquete(paquete, conexionSwamp);

/*recibimos el contenido */

	t_paquete* paqueteRecibido = malloc(sizeof(t_paquete));

	if(recv(conexionSwamp, &(paqueteRecibido->codigo_operacion), sizeof(cod_operacion), 0) < 1){
		free(paqueteRecibido);
		printf("Fallo en recibir la info de la conexion");
	}else{

		paqueteRecibido->buffer = malloc(sizeof(t_buffer));
		recv(conexionSwamp, &(paqueteRecibido->buffer->size), sizeof(uint32_t), 0);


		if(paqueteRecibido->buffer->size > 0){
			paqueteRecibido->buffer->stream = malloc(paqueteRecibido->buffer->size);
			recv(conexionSwamp, paqueteRecibido->buffer->stream, paqueteRecibido->buffer->size, 0);
		}
		

		int desplazamiento = 0;
		int tamanioPagina =32;
		void* contenido = malloc(tamanioPagina+1);
		char valor = '\0';
		
		//esto es para que lo pueda mostrar como un char*, ya que si no le ponemos ese \0 al final no funca o el valgrind tira un leak
		memcpy(contenido, paqueteRecibido->buffer->stream + desplazamiento, tamanioPagina);
		memcpy(contenido + tamanioPagina ,&(valor), 1);

		printf("El contenido leido desde SWAmP es: %s",(char*) contenido);
		printf("\n");
		
		free(contenido);
		free(paqueteRecibido->buffer->stream);
		free(paqueteRecibido->buffer);
		free(paqueteRecibido);
	
	}

}


int main() {

    enviar_tipo_asignacion("FIJA");
	enviadoPagina(3,1,"asd");
	pedirPagina(1, 3);
	
    return 0;
}
