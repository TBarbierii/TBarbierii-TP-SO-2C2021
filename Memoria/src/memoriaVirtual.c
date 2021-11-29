#include "memoriaVirtual_suspencion.h"

int32_t buscar_TLB(t_pagina* pagina){

	pthread_mutex_lock(TLB_mutex);

	bool buscarPagina(t_pagina *pag){
		return pag->id_pagina == pagina->id_pagina && pag->id_carpincho == pagina->id_carpincho;
	};

	t_pagina* paginaEncontrada = list_find(TLB, (void*)buscarPagina);
	pthread_mutex_unlock(TLB_mutex);

	if(paginaEncontrada == NULL){
		return -1;
	}

		return paginaEncontrada->marco->comienzo;
	
}

t_marco* reemplazarPagina(t_carpincho* carpincho){

	if(strcmp(tipoAsignacion, "FIJA") == 0){

		bool paginasPresentes(t_pagina* pag){
			return pag->presente;
		};
		
		t_list* paginas_a_reemplazar = list_filter(carpincho->tabla_de_paginas, (void*)paginasPresentes);

		t_pagina* victima = algoritmo_reemplazo_MMU(paginas_a_reemplazar, carpincho);   

		log_info(logsObligatorios, "Pagina víctima: Pid: %i, Página: %i, Marco: %i", carpincho->id_carpincho, victima->id_pagina, victima->marco->id_marco);

		void* contenido = malloc(tamanioPagina);
		pthread_mutex_lock(memoria);
		memcpy(contenido, memoriaPrincipal + victima->marco->comienzo, tamanioPagina);
		pthread_mutex_unlock(memoria);

		if(victima->modificado)
		enviar_pagina(carpincho->id_carpincho, victima->id_pagina, contenido);
		

		victima->presente = false;
		victima->marco->estaLibre = true;
		
		pthread_mutex_lock(TLB_mutex);

		bool quitarDeTLB(t_pagina* pag){
			return victima->id_pagina == pag->id_pagina && victima->id_carpincho ==  pag->id_carpincho;
		};
		list_remove_by_condition(TLB, (void*)quitarDeTLB);// se quita directamente la pagina que se mando a swap.
		pthread_mutex_unlock(TLB_mutex);
		free(contenido);

		return victima->marco;	
	}

	if(strcmp(tipoAsignacion, "DINAMICA") == 0){
		
		bool paginasPresentes(t_pagina* pag){
			return pag->presente;
		};

		t_list* paginas_a_reemplazar = list_create();

		pthread_mutex_lock(listaCarpinchos);
		for (int i=0; i<list_size(carpinchos); i++){

			t_carpincho* carp = list_get(carpinchos, i);
			t_list* paginas = list_filter(carp->tabla_de_paginas, (void*)paginasPresentes);
			list_add_all(paginas_a_reemplazar, paginas);
			list_destroy(paginas);
		}
		pthread_mutex_unlock(listaCarpinchos);
	
		
		t_pagina* victima = algoritmo_reemplazo_MMU(paginas_a_reemplazar, carpincho); 

		log_info(logsObligatorios, "Pagina víctima: Pid: %i, Página: %i, Marco: %i", victima->id_carpincho, victima->id_pagina, victima->marco->id_marco);

		list_destroy(paginas_a_reemplazar);  												

		void* contenido = malloc(tamanioPagina);
		pthread_mutex_lock(memoria);
		memcpy(contenido, memoriaPrincipal + victima->marco->comienzo, tamanioPagina);
		pthread_mutex_unlock(memoria);

		if(victima->modificado)
		enviar_pagina(victima->id_carpincho, victima->id_pagina, contenido);
		

		victima->presente = false;
		victima->marco->estaLibre = true;
		victima->marco->proceso_asignado = -1;
		
		pthread_mutex_lock(TLB_mutex);
		bool quitarDeTLB(t_pagina* pag){
			return victima->id_pagina == pag->id_pagina;
		};
		list_remove_by_condition(TLB, (void*)quitarDeTLB);// se quita directamente la pagina que se mando a swap.
		pthread_mutex_unlock(TLB_mutex);
		free(contenido);

		return victima->marco;
		
	}else{
		return NULL;
	}


}

t_pagina* algoritmo_reemplazo_MMU(t_list* paginas_a_reemplazar, t_carpincho* carpincho){
	
	if(strcmp(algoritmoReemplazoMMU, "LRU") == 0){
		
		bool comparator(t_pagina* p1, t_pagina* p2){
			return p1->ultimoUso < p2->ultimoUso;
		};
		
		t_list* paginasOrdenadas = list_sorted(paginas_a_reemplazar, (void*)comparator);

		t_pagina* pag = list_get(paginasOrdenadas, 0);

		list_destroy(paginasOrdenadas);
		return pag;
	}

	if(strcmp(algoritmoReemplazoMMU, "CLOCK-M") == 0){

		
		bool comparator(t_pagina* p1, t_pagina* p2){
			return p1->marco->id_marco < p2->marco->id_marco;
		};
		
		t_list* paginasOrdenadas = list_sorted(paginas_a_reemplazar, (void*)comparator);

		if(strcmp(tipoAsignacion, "FIJA") == 0){

		int puntero = carpincho->punteroClock;

		segundoIntento:

		for(int a = 0; a < list_size(paginasOrdenadas);a++){ //primera vuelta

			t_pagina* candidata = list_get(paginasOrdenadas, puntero);
			if(candidata->uso == 0 && candidata->modificado == 0){
				carpincho->punteroClock++;
				if(carpincho->punteroClock >= list_size(paginasOrdenadas)){
					carpincho->punteroClock = 0;
				}
				free (paginasOrdenadas);
				return candidata;
			}else{
				puntero++;
				if(puntero >= list_size(paginasOrdenadas)){
					puntero = 0;
				}
			}

		}

		for(int a = 0; a < list_size(paginasOrdenadas);a++){ //segunda vuelta

			t_pagina* candidata = list_get(paginasOrdenadas, puntero);
			
			if(candidata->uso == 0 && candidata->modificado == 1){
				carpincho->punteroClock++;
				if(carpincho->punteroClock >= list_size(paginasOrdenadas)){
					carpincho->punteroClock = 0;
				}
				free (paginasOrdenadas);
				return candidata;
				
			}else{
				puntero++;
				candidata->uso = false;
				if(puntero >= list_size(paginasOrdenadas)){
					puntero = 0;
				}
			}

		}

		goto segundoIntento; //si llegó hasta aca es porque hizo las dos vueltas y tiene que empezar de nuevo

		free (paginasOrdenadas);
		}

		if(strcmp(tipoAsignacion, "DINAMICA") == 0){

		punteroClock;

		segundoIntentoDinamica:

		for(int a = 0; a < list_size(paginasOrdenadas);a++){ //primera vuelta

			t_pagina* candidata = list_get(paginasOrdenadas, punteroClock);
			if(candidata->uso == 0 && candidata->modificado == 0){
				punteroClock++;
				if(punteroClock >= list_size(paginasOrdenadas)){
					punteroClock = 0;
				}
				free (paginasOrdenadas);
				return candidata;
			}else{
				punteroClock++;
				if(punteroClock >= list_size(paginasOrdenadas)){
					punteroClock = 0;
				}
			}

		}

		for(int a = 0; a < list_size(paginasOrdenadas);a++){ //segunda vuelta

			t_pagina* candidata = list_get(paginasOrdenadas, punteroClock);
			
			if(candidata->uso == 0 && candidata->modificado == 1){
				punteroClock++;
				if(punteroClock >= list_size(paginasOrdenadas)){
					punteroClock = 0;
				}
				free (paginasOrdenadas);
				return candidata;
				
			}else{
				punteroClock++;
				candidata->uso = false;
				if(punteroClock >= list_size(paginasOrdenadas)){
					punteroClock = 0;
				}
			}

		}

		goto segundoIntentoDinamica; //si llegó hasta aca es porque hizo las dos vueltas y tiene que empezar de nuevo

		free (paginasOrdenadas);
		}

	}

}

void algoritmo_reemplazo_TLB(t_pagina* pagina){

	if(list_size(TLB) == cantidadEntradasTLB){

		if(strcmp(algoritmoReemplazoTLB, "LRU") == 0){
			
			pthread_mutex_lock(TLB_mutex);
			bool comparator(t_pagina* p1, t_pagina* p2){
				return p1->ultimoUso < p2->ultimoUso;
			};

			t_list* paginasOrdenadas = list_sorted(TLB, (void*)comparator);

			if(! list_is_empty(paginasOrdenadas)) {
				t_pagina* pag = list_get(paginasOrdenadas,0);
				pag->presente = false;

				void buscarPag(t_pagina* p){
					return p->id_pagina == pag->id_pagina;
				};

				list_remove_by_condition(TLB, (void*)buscarPag);
				
				log_info(logsObligatorios, "Entrada TLB. Victima: PID: %i	Página: %i	Marco: %i", pag->id_carpincho, pag->id_pagina, pag->marco->id_marco);

			}
			pthread_mutex_unlock(TLB_mutex);


			pthread_mutex_lock(TLB_mutex);
			list_add(TLB, pagina);
			pthread_mutex_unlock(TLB_mutex);

			log_info(logsObligatorios, "Entrada TLB. NuevaEntrada: PID: %i	Página: %i	Marco: %i", pagina->id_carpincho, pagina->id_pagina, pagina->marco->id_marco);

		}

		if(strcmp(algoritmoReemplazoTLB, "FIFO") == 0){
			
			pthread_mutex_lock(TLB_mutex);
			
			t_pagina* pag = list_get(TLB,0);
			pag->presente = false;
			log_info(logsObligatorios, "Entrada TLB. Victima: PID: %i	Página: %i	Marco: %i", pag->id_carpincho, pag->id_pagina, pag->marco->id_marco);
			
			list_remove(TLB, 0);

			list_add(TLB, pagina);
			pthread_mutex_unlock(TLB_mutex);

			log_info(logsObligatorios, "Entrada TLB. NuevaEntrada: PID: %i	Página: %i	Marco: %i", pagina->id_carpincho, pagina->id_pagina, pagina->marco->id_marco);

		}

	}else{
		pthread_mutex_lock(TLB_mutex);
		list_add(TLB, pagina);
		pthread_mutex_unlock(TLB_mutex);
		
		log_info(logsObligatorios, "Entrada TLB. NuevaEntrada: PID: %i	Página: %i	Marco: %i", pagina->id_carpincho, pagina->id_pagina, pagina->marco->id_marco);

	}
}

uint32_t swapear(t_carpincho* carpincho, t_pagina* paginaPedida){
	
	pthread_mutex_lock(swap);
	t_marco* marcoLiberado = reemplazarPagina(carpincho);
	uint32_t conexion = pedir_pagina(paginaPedida->id_pagina, carpincho->id_carpincho);
	void* contenido = atender_respuestas_swap(conexion);
	pthread_mutex_unlock(swap);
	paginaPedida->marco = marcoLiberado;
	paginaPedida->marco->estaLibre = false;
	paginaPedida->presente = true;
	paginaPedida->ultimoUso = clock();
	paginaPedida->uso = true;
	paginaPedida->modificado = false;

	log_info(logsObligatorios, "Pagina entrante: Pid: %i, Página: %i, Marco: %i", carpincho->id_carpincho, paginaPedida->id_pagina, paginaPedida->marco->id_marco);


	heapMetadata* heap = malloc(TAMANIO_HEAP);
	memcpy(heap, contenido, TAMANIO_HEAP);

   	pthread_mutex_lock(memoria);
	memcpy(memoriaPrincipal + paginaPedida->marco->comienzo, contenido, tamanioPagina);
	pthread_mutex_unlock(memoria);


	algoritmo_reemplazo_TLB(paginaPedida);

	return marcoLiberado->comienzo;
}

int32_t buscarEnTablaDePaginas(t_carpincho* carpincho, int32_t idPag){

	bool buscarPaginaPresente(t_pagina* pag){
		return pag->id_pagina == idPag && pag->presente;
	};

	t_pagina* pagina = list_find(carpincho->tabla_de_paginas, (void*)buscarPaginaPresente);

	if(pagina == NULL){
		return -1;
	}

	return pagina->marco->comienzo;

}

void reemplazo(int32_t *DF, t_carpincho* carpincho, t_pagina* pagina){

	if(*DF == -1){ //tlb miss
		usleep(retardoFAlloTLB * 1000);
		*DF = buscarEnTablaDePaginas(carpincho, pagina->id_pagina);
		
		if(*DF == -1) *DF = swapear(carpincho, pagina);
			carpincho->tlb_miss++;
			miss_totales++;

	}else{//hit
		
		usleep(retardoAciertoTLB * 1000);
		carpincho->tlb_hit++;
		hits_totales++;
	}

}