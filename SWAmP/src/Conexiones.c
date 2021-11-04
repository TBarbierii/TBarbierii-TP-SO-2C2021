#include "Conexiones.h"


int iniciar_servidor_swamp() {

	t_log* logger_nuevo =  log_create("cfg/Servidor.log","Servidor",1,LOG_LEVEL_DEBUG);

	int servidor = iniciar_servidor(ip_swap, puerto_swap); // devuelve el socket del servidor
	
	log_info(logger_nuevo,"Inicializacion del servidor con exito, esperando conexion...");

	while(1){
		int conexion = esperar_cliente(servidor);
		log_info(logger_nuevo,"Solicitud de memoria entrante");
		atender_mensaje_ram(conexion);
	}

	log_destroy(logger_nuevo);
}

uint32_t recibir_operacion(uint32_t socket_cliente) {
	
    uint32_t cod_op;

	if(recv(socket_cliente, &cod_op, sizeof(uint32_t), MSG_WAITALL) != 0){
		return cod_op;
	}else{
		close(socket_cliente);
		return -1;
	}
}

int atender_mensaje_ram(int conexion) {

	t_log* logger_servidor =  log_create("cfg/OperacionesServidor.log","OperacionesServidor",1,LOG_LEVEL_DEBUG);

	t_paquete* paquete = malloc(sizeof(t_paquete));

	if(recv(conexion, &(paquete->codigo_operacion), sizeof(cod_operacion), 0) < 1){
		free(paquete);
		log_error(logger_servidor,"Fallo en recibir la info de la conexion");
		return -1;
	}

	log_info(logger_servidor,"Recibimos la informacion de la RAM");
	log_info(logger_servidor,"El codigo de operacion es: %d",paquete->codigo_operacion);

	paquete->buffer = malloc(sizeof(t_buffer));
	recv(conexion, &(paquete->buffer->size), sizeof(uint32_t), 0);

	if(paquete->buffer->size > 0){
		paquete->buffer->stream = malloc(paquete->buffer->size);
		recv(conexion, paquete->buffer->stream, paquete->buffer->size, 0);
	}
	
	usleep(retardo_swap);

	switch(paquete->codigo_operacion){

        case LECTURA_PAGINA:
			atender_solicitud_pedido_de_pagina(paquete->buffer, conexion, logger_servidor);
			break;

        case ESCRITURA_PAGINA:;
			recibir_pagina(paquete->buffer, logger_servidor);
			notificar_escritura_de_pagina(conexion);
        	break;

		case CERRAR_INSTANCIA:;
			atender_solicitud_cierre_proceso(paquete->buffer, logger_servidor);
			notificar_finalizacion_de_proceso(conexion);
			break;

		case TIPOASIGNACION:;
			recibir_tipo_asignacion(paquete->buffer, logger_servidor);
			close(conexion);
       		break;

		default:;
		log_info(logger_servidor,"Codigo de operacion incorrecto.");
		break;



	}
	
	
	int valorOperacion = paquete->codigo_operacion;

    if(paquete->buffer->size > 0){
	    free(paquete->buffer->stream);
    }

	free(paquete->buffer);
	free(paquete);
	log_destroy(logger_servidor);

	return valorOperacion;
	
}

uint32_t recibir_tipo_asignacion(t_buffer* buffer, t_log* logger) {

	void* data = buffer->stream;

	memcpy(&(tipo_asignacion), data, sizeof(uint32_t));
	
	if(tipo_asignacion == 0) {
		log_info(logger, "Tipo de asignacion a utilizar: DINAMICA.");
	}else if(tipo_asignacion == 1) {
		log_info(logger, "Tipo de asignacion a utilizar: FIJA.");		
	}else{
		log_info(logger, "Tipo de asignacion incorrecto.");
	}
	
	return tipo_asignacion;
}

void recibir_pagina(t_buffer* buffer, t_log *logger) {

	void* data = buffer->stream;
	void* contenido = malloc(tamanio_pagina);
	int desplazamiento = 0;
	uint32_t id_pagina;
	uint32_t PID;

	memcpy(&(PID), data + desplazamiento, sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);
	memcpy(&(id_pagina), data + desplazamiento, sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);
    memcpy(contenido, data + desplazamiento , tamanio_pagina);
	log_info(logger, "Se guarda la pagina: %d del proceso: %d",id_pagina, PID);
	
	escribirContenido(contenido, id_pagina, PID, logger);

}


void enviar_pagina(void* contenido, int conexion) {

    t_paquete *paquete = crear_paquete(LECTURA_PAGINA);

	paquete->buffer->size = tamanio_pagina;
    paquete->buffer->stream = malloc(paquete->buffer->size);
	uint32_t desplazamiento=0;

    memcpy(paquete->buffer->stream + desplazamiento, contenido , tamanio_pagina);

	enviarPaquete(paquete, conexion);
}

void atender_solicitud_pedido_de_pagina(t_buffer* buffer, int conexion, t_log* logger) {

	void* data = buffer->stream;
	int desplazamiento = 0;
	uint32_t id_pagina;
	uint32_t PID;

	memcpy(&(PID), data + desplazamiento, sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);
	memcpy(&(id_pagina), data + desplazamiento, sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);

	log_info(logger,"El proceso: %d, pide leer su pagina numero: %d",PID,id_pagina);

	leer_contenido(PID, id_pagina, conexion, logger_swamp);

}

void notificar_escritura_de_pagina(int conexion) {

	t_paquete *paquete = crear_paquete(ESCRITURA_PAGINA);

	paquete->buffer->size = sizeof(uint32_t);
    paquete->buffer->stream = malloc(paquete->buffer->size);
	uint32_t desplazamiento=0;
	int valor = 1;

    memcpy(paquete->buffer->stream + desplazamiento, &(valor), sizeof(uint32_t));

	enviarPaquete(paquete, conexion);

}

void atender_solicitud_cierre_proceso(t_buffer* buffer, t_log* logger) {

	void* data = buffer->stream;
	int desplazamiento = 0;
	uint32_t PID;

	memcpy(&(PID), data + desplazamiento, sizeof(uint32_t));

	log_info(logger,"El proceso: %d, pide finalizar",PID);

	limpiar_marcos_de_proceso(PID);

}

void notificar_finalizacion_de_proceso(int conexion) {

	t_paquete *paquete = crear_paquete(CERRAR_INSTANCIA);

	paquete->buffer->size = sizeof(uint32_t);
    paquete->buffer->stream = malloc(paquete->buffer->size);
	uint32_t desplazamiento=0;
	int valor = 1;

    memcpy(paquete->buffer->stream + desplazamiento, &(valor), sizeof(uint32_t));

	enviarPaquete(paquete, conexion);

}