#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CAPACIDAD_INICIAL 16

/* Cada clave mantiene una lista enlazada con todos sus valores. */
typedef struct Nodo {
    char *valor;
    struct Nodo *siguiente;
} Nodo;

/* Las entradas se encadenan para resolver colisiones en las entrada. */
typedef struct Entrada {
    char *clave;
    Nodo *valores;
    Nodo *ultimo_valor;
    struct Entrada *siguiente;
} Entrada;

/* La tabla almacena una lista de entradas en cada entrada. */
typedef struct {
    Entrada **entrada;
    size_t capacidad;
    size_t cantidad;
} TablaHash;

/* Crea una copia propia de una cadena para que la tabla controle su memoria. */
static char *copiar_cadena(const char *texto) {
    size_t longitud = strlen(texto) + 1;
    char *copia = malloc(longitud);

    if (copia != NULL) {
        strcpy(copia, texto);
    }
    return copia;
}

/* Calcula un hash para asignar cada clave a una cubeta. */
static uint64_t calcular_hash(const char *clave) {
    uint64_t hash = UINT64_C(5381);

    while (*clave != '\0') {
        hash = hash * UINT64_C(33) + (unsigned char)*clave;
        clave++;
    }
    return hash;
}

/* Reserva una tabla vacía con capacidad inicial. */
TablaHash *hash_table_create(void) {
    TablaHash *tabla = malloc(sizeof(*tabla));
    if (tabla == NULL) {
        return NULL;
    }

    tabla->entrada = calloc(CAPACIDAD_INICIAL, sizeof(*tabla->entrada));
    if (tabla->entrada == NULL) {
        free(tabla);
        return NULL;
    }

    tabla->capacidad = CAPACIDAD_INICIAL;
    tabla->cantidad = 0;
    return tabla;
}

/* Duplica la capacidad y redistribuye las entradas en sus nuevas entrada. */
static int ampliar_tabla_hash(TablaHash *tabla){
    if (SIZE_MAX / 2 < tabla->capacidad) {
        return 0;
    }
    size_t nueva_capacidad = tabla->capacidad * 2;
    Entrada **nueva_entrada = calloc(nueva_capacidad, sizeof(*nueva_entrada));
    if (nueva_entrada == NULL) {
        return 0;
    }

    for (size_t indice = 0; indice < tabla->capacidad; indice++) {
        Entrada *entrada = tabla->entrada[indice];
        while (entrada != NULL) {
            Entrada *siguiente_entrada = entrada->siguiente;
            size_t nuevo_indice = (size_t)(calcular_hash(entrada->clave) % nueva_capacidad);
            entrada->siguiente = nueva_entrada[nuevo_indice];
            nueva_entrada[nuevo_indice] = entrada;
            entrada = siguiente_entrada;
        }
    }

    free(tabla->entrada);
    tabla->entrada = nueva_entrada;
    tabla->capacidad = nueva_capacidad;
    return 1;
}

/* Agrega un valor a la lista de la clave, o crea una entrada nueva. */
int hash_table_add(TablaHash *tabla, const char *clave, const char *valor) {
    if (tabla == NULL || clave == NULL || valor == NULL) {
        return 0;
    }

    /* Busca la clave en la cadena de la entrada correspondiente. */
    size_t indice = (size_t)(calcular_hash(clave) % tabla->capacidad);
    Entrada *entrada = tabla->entrada[indice];
    while (entrada != NULL && strcmp(entrada->clave, clave) != 0) {
        entrada = entrada->siguiente;
    }

    Nodo *nodo = malloc(sizeof(*nodo));
    if (nodo == NULL) {
        free(nodo);
        return 0;
    }
    nodo->valor = copiar_cadena(valor);
    nodo->siguiente = NULL;
    if (nodo->valor == NULL) {
        free(nodo);
        return 0;
    }

    if (entrada != NULL) {
        Nodo *nodo_actual = entrada->valores;
        int valor_existe = 0;
        while (nodo_actual != NULL) {
            if (strcmp(nodo_actual->valor, valor) == 0) {
                valor_existe = 1;
                break;
            }
            nodo_actual = nodo_actual->siguiente;
        }
        
        if(valor_existe) {
            free(nodo->valor);
            free(nodo);
            return 1; /* El valor ya existe, no se agrega duplicado. */
        }
        /* Si la clave ya existe, agrega el valor al final de su lista. */
        entrada->ultimo_valor->siguiente = nodo;
        entrada->ultimo_valor = nodo;
        return 1;
    }

    char *copia_clave = copiar_cadena(clave);
    if (copia_clave == NULL) {
        free(nodo->valor);
        free(nodo);
        return 0;
    }

    if (tabla->cantidad >= tabla->capacidad - tabla->capacidad / 4) {
        /* Mantiene el factor de carga por debajo del 75 %. */
        if (!ampliar_tabla_hash(tabla)) {
            free(copia_clave);
            free(nodo->valor);
            free(nodo);
            return 0;
        }
        indice = (size_t)(calcular_hash(clave) % tabla->capacidad);
    }

    entrada = malloc(sizeof(*entrada));
    if (entrada == NULL) {
        free(copia_clave);
        free(nodo->valor);
        free(nodo);
        return 0;
    }

    entrada->clave = copia_clave;
    entrada->valores = nodo;
    entrada->ultimo_valor = nodo;
    entrada->siguiente = tabla->entrada[indice];
    tabla->entrada[indice] = entrada;
    tabla->cantidad++;
    return 1;
}

/* Devuelve el primer nodo de valores; retorna NULL si la clave no existe. Luego se puede recorrer la lista de valores asociados a la clave. */
const Nodo *hash_table_get(const TablaHash *tabla, const char *clave) {
    if (tabla == NULL || clave == NULL) {
        return NULL;
    }
    size_t indice = (size_t)(calcular_hash(clave) % tabla->capacidad);
    Entrada *entrada = tabla->entrada[indice];
    while (entrada != NULL) {
        if (strcmp(entrada->clave, clave) == 0) {
            return entrada->valores;
        }
        entrada = entrada->siguiente;
    }
    return NULL;
}

/* Imprime cada clave seguida por todos los valores asociados. */
void hash_table_print(const TablaHash *tabla) {
    if (tabla == NULL) {
        return;
    }

    for (size_t indice = 0; indice < tabla->capacidad; indice++) {
        for (Entrada *entrada = tabla->entrada[indice]; entrada != NULL;
             entrada = entrada->siguiente) {
            printf("%s:", entrada->clave);
            for (Nodo *valor = entrada->valores; valor != NULL;
                 valor = valor->siguiente) {
                printf(" %s", valor->valor);
            }
            putchar('\n');
        }
    }
}

/* Libera las claves, los valores, las entradas y finalmente la tabla. */
void hash_table_destroy(TablaHash *tabla) {
    if (tabla == NULL) {
        return;
    }

    for (size_t indice = 0; indice < tabla->capacidad; indice++) {
        Entrada *entrada = tabla->entrada[indice];
        while (entrada != NULL) {
            Entrada *siguiente_entrada = entrada->siguiente;
            Nodo *valor = entrada->valores;
            while (valor != NULL) {
                Nodo *siguiente_valor = valor->siguiente;
                free(valor->valor);
                free(valor);
                valor = siguiente_valor;
            }
            free(entrada->clave);
            free(entrada);
            entrada = siguiente_entrada;
        }
    }

    free(tabla->entrada);
    free(tabla);
}

int main(void) {
    TablaHash *tabla = hash_table_create();
    if (tabla == NULL) {
        printf("No se pudo crear la tabla hash.\n");
        return EXIT_FAILURE;
    }

    /* Ejemplo: una misma clave puede tener más de un valor. */
    if (!hash_table_add(tabla, "ERNESTO", "10") ||
        !hash_table_add(tabla, "ERNESTO", "10") ||
        !hash_table_add(tabla, "ERNESTO", "4") ||
        !hash_table_add(tabla, "ERNESTO", "10") ||
        !hash_table_add(tabla, "ERNESTO", "23") ||
        !hash_table_add(tabla, "ERNESTO", "EMPTY") ||
        !hash_table_add(tabla, "CARLOS", "11") ||
        !hash_table_add(tabla, "CARLOS", "11") ||
        !hash_table_add(tabla, "CARLOS", "11") ||
        !hash_table_add(tabla, "HUGO", "11") ||
        !hash_table_add(tabla, "HUGO", "22") ||
        !hash_table_add(tabla, "HUGO", "44") ||
        !hash_table_add(tabla, "HUGO", "11") ||
        !hash_table_add(tabla, "CARLOS", "11") ||
        !hash_table_add(tabla, "CARLOS", "22")) {
        printf("No se pudo agregar un elemento a la tabla hash.\n");
        hash_table_destroy(tabla);
        

        return EXIT_FAILURE;
    }

    hash_table_print(tabla);
    
    hash_table_destroy(tabla);
    return EXIT_SUCCESS;
}
