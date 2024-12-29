#include<stdio.h>
#include<string.h>
#include<ctype.h>
#include "headers.h"

#define LONGITUD_COMANDO 100

void Bytemaps(EXT_BYTE_MAPS *ext_bytemaps);
int ComprobarComando(char *strcomando, char *orden, char *argumento1, char *argumento2);
void Info(EXT_SIMPLE_SUPERBLOCK *psup);
int BuscaFich(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos, 
              char *nombre);
void Dir(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos);
int Rename(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos, 
              char *nombreantiguo, char *nombrenuevo);
int Print(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos,
             EXT_DATOS *memdatos, char *nombre);
int Remove(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos,
           EXT_BYTE_MAPS *ext_bytemaps, EXT_SIMPLE_SUPERBLOCK *ext_superblock,
           char *nombre,  FILE *fich);
int Copy(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos,
           EXT_BYTE_MAPS *ext_bytemaps, EXT_SIMPLE_SUPERBLOCK *ext_superblock,
           EXT_DATOS *memdatos, char *nombreorigen, char *nombredestino,  FILE *fich);
void Grabarinodosydirectorio(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos, FILE *fich);
void GrabarByteMaps(EXT_BYTE_MAPS *ext_bytemaps, FILE *fich);
void GrabarSuperBloque(EXT_SIMPLE_SUPERBLOCK *ext_superblock, FILE *fich);
void GrabarDatos(EXT_DATOS *memdatos, FILE *fich);

int main()
{
	 char comando[LONGITUD_COMANDO];
	 char orden[LONGITUD_COMANDO];
	 char argumento1[LONGITUD_COMANDO];
	 char argumento2[LONGITUD_COMANDO];
	 
     EXT_SIMPLE_SUPERBLOCK ext_superblock;
     EXT_BYTE_MAPS ext_bytemaps;
     EXT_BLQ_INODOS ext_blq_inodos;
     EXT_ENTRADA_DIR directorio[MAX_FICHEROS];
     EXT_DATOS memdatos[MAX_BLOQUES_DATOS];
     EXT_DATOS datosfich[MAX_BLOQUES_PARTICION];
     int grabardatos;
     FILE *fent;
     
     // Lectura del fichero completo de una sola vez
     
    fent = fopen("particion.bin","r+b");
    fread(&datosfich, SIZE_BLOQUE, MAX_BLOQUES_PARTICION, fent);    
     
    memcpy(&ext_superblock,(EXT_SIMPLE_SUPERBLOCK *)&datosfich[0], SIZE_BLOQUE);
    memcpy(&directorio,(EXT_ENTRADA_DIR *)&datosfich[3], SIZE_BLOQUE);
    memcpy(&ext_bytemaps,(EXT_BLQ_INODOS *)&datosfich[1], SIZE_BLOQUE);
    memcpy(&ext_blq_inodos,(EXT_BLQ_INODOS *)&datosfich[2], SIZE_BLOQUE);
    memcpy(&memdatos,(EXT_DATOS *)&datosfich[4],MAX_BLOQUES_DATOS*SIZE_BLOQUE);
     
    // Bucle de tratamiento de comandos
    for (;;){
	    do {
		printf(">> ");
            fgets(comando, LONGITUD_COMANDO, stdin);
            comando[strcspn(comando, "\n")] = '\0';
		} while (ComprobarComando(comando,orden,argumento1,argumento2) !=0);

        if (strcmp(orden, "info") == 0) {
            Info(&ext_superblock);
            continue;
        }

        if (strcmp(orden, "dir") == 0) {
            Dir(directorio, &ext_blq_inodos);
            continue;
        }

        if (strcmp(orden, "bytemaps") == 0) {
            Bytemaps(&ext_bytemaps);
            continue;
        }

        if (strcmp(orden, "rename") == 0) {
            Rename(directorio, &ext_blq_inodos, argumento1, argumento2);
            continue;
        }

        if (strcmp(orden, "remove") == 0) {
            Remove(directorio, &ext_blq_inodos, &ext_bytemaps, &ext_superblock, argumento1, fent);
            continue;
        }

        if (strcmp(orden, "print") == 0) {
            Print(directorio, &ext_blq_inodos, memdatos, argumento1);
            continue;
        }

        if (strcmp(orden, "copy") == 0) {
            Copy(directorio, &ext_blq_inodos, &ext_bytemaps, &ext_superblock, memdatos, argumento1, argumento2, fent);
            continue;
        }

        // Escritura de metadatos en comandos rename, remove, copy

        Grabarinodosydirectorio(directorio,&ext_blq_inodos,fent);//directorio
        GrabarByteMaps(&ext_bytemaps,fent);
        GrabarSuperBloque(&ext_superblock,fent);
        if (grabardatos)
           GrabarDatos(memdatos,fent);
        grabardatos = 0;

        //Si el comando es salir se habrán escrito todos los metadatos
        //faltan los datos y cerrar

        if (strcmp(orden,"exit")==0){
            GrabarDatos(memdatos,fent);
            fclose(fent);
            return 0;
        }
    }
}

void Grabarinodosydirectorio(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos, FILE *fich) {
    fseek(fich, 3 * SIZE_BLOQUE, SEEK_SET); // Directory starts at block 3
    fwrite(directorio, SIZE_BLOQUE, 1, fich);
    fseek(fich, 2 * SIZE_BLOQUE, SEEK_SET); // Inodes are at block 2
    fwrite(inodos, SIZE_BLOQUE, 1, fich);
}

void GrabarByteMaps(EXT_BYTE_MAPS *ext_bytemaps, FILE *fich) {
    fseek(fich, SIZE_BLOQUE, SEEK_SET); // Byte maps start at block 1
    fwrite(ext_bytemaps, SIZE_BLOQUE, 1, fich);
}

void GrabarSuperBloque(EXT_SIMPLE_SUPERBLOCK *ext_superblock, FILE *fich) {
    fseek(fich, 0, SEEK_SET); // Superblock starts at block 0
    fwrite(ext_superblock, SIZE_BLOQUE, 1, fich);
}

void GrabarDatos(EXT_DATOS *memdatos, FILE *fich) {
    fseek(fich, 4 * SIZE_BLOQUE, SEEK_SET); // Data starts at block 4
    fwrite(memdatos, SIZE_BLOQUE, MAX_BLOQUES_DATOS, fich);
}


int ComprobarComando(char *strcomando, char *orden, char *argumento1, char *argumento2) {
    // Tokenize the string strcomando
    char *token = strtok(strcomando, " ");

    // Copy the first part of strcomando into orden
    if (token != NULL) {
        strcpy(orden, token);
        token = strtok(NULL, " ");
    } else {
        orden[0] = '\0'; 
    }

    // Copy the second part of strcomando into argumento1
    if (token != NULL) {
        strcpy(argumento1, token);
        token = strtok(NULL, " ");
    } else {
        argumento1[0] = '\0';
    }

    // Copy the third part of strcomando into argumento2
    if (token != NULL) {
        strcpy(argumento2, token);
        token = strtok(NULL, " ");
    } else {
        argumento2[0] = '\0';
    }

    // Check if the commands are valid
   if (strcmp(orden, "info") == 0) {
        return 0;
   } else if (strcmp(orden, "bytemaps") == 0) {
        return 0;
   } else if (strcmp(orden, "dir") == 0) {
        return 0;
   } else if (strcmp(orden, "rename") == 0) {
        return 0;
   } else if (strcmp(orden, "print") == 0) {
        return 0;
   } else if (strcmp(orden, "remove") == 0) {
        return 0;
   } else if (strcmp(orden, "copy") == 0) {
        return 0;
   } else if (strcmp(orden, "exit") == 0) {
        return 0;
   } else {
        printf("\nERROR: illegal command [info,bytemaps,dir,rename,print,remove,copy,exit]\n");
        return 1;
   }
}

int BuscaFich(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos, char *nombre) {
    // Checks every file name to match with nombre
    for (int i = 0; i < MAX_FICHEROS; i++) {
        if (strcmp(directorio[i].dir_nfich, nombre) == 0) {
            return i; // Returns directory position if file is found
        }
    }
    return -1; // Returns -1 when file is not found
}

void Info(EXT_SIMPLE_SUPERBLOCK *psup) { // *psup is the pointer to the EXT_SIMPLE_SUPERBLOCK structure containing metadata about the file system
    printf("Block size: %u\n", (*psup).s_block_size);
    printf("Inodes count: %u\n", (*psup).s_inodes_count);
    printf("Free inodes: %u\n", (*psup).s_free_inodes_count); // printf for each element
    printf("Blocks count: %u\n", (*psup).s_blocks_count);
    printf("Free blocks: %u\n", (*psup).s_free_blocks_count);
    printf("First data block: %u\n\n", (*psup).s_first_data_block);
}

void Dir(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos) {
    for (int i = 0; i < MAX_FICHEROS; i++) {                        
        if ((*directorio).dir_inodo != NULL_INODO) { // Check if the directory entry is not empty
            EXT_SIMPLE_INODE inodo = (*inodos).blq_inodos[(*directorio).dir_inodo]; // Get the inode associated with the current directory
            
            if(inodo.size_fichero != 0) { // Only display files that are not marked as deleted (size > 0), just for prevention
                printf("File: %s    \t Size: %u bytes   \t Inode: %u   Blocks: ", (*directorio).dir_nfich, inodo.size_fichero, (*directorio).dir_inodo);

                for (int j = 0; j < MAX_NUMS_BLOQUE_INODO; j++) { // Loop through the inodes block numbers and print the valid ones
                    if (inodo.i_nbloque[j] != NULL_BLOQUE) {
                        printf("%u ", inodo.i_nbloque[j]);
                    }
                }
            printf("\n");
            }
        }
        directorio++; // Moving to the next directory entry
    }
    printf("\n");
}

void Bytemaps(EXT_BYTE_MAPS *ext_bytemaps) {
    printf("Block Bytemap [0-25]:\n");
    for (int i = 0; i < 25; i++) {
        printf("%u ", (*ext_bytemaps).bmap_bloques[i]); // Print the block bytemap, shows (0) for free blocks, (1) for occupied
    }
    printf("\nInode Bytemap:\n");
    for (int i = 0; i < MAX_INODOS; i++) {
        printf("%u ", (*ext_bytemaps).bmap_inodos[i]); // Print the inode bytemap, shows (0) for free inodes, (1) for occupied
    }
    printf("\n\n");
}

int Rename(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos, char *nombreantiguo, char *nombrenuevo) {
    int index_antiguo = -1;

    // Find the file in the directory
    for (int i = 0; i < MAX_FICHEROS; i++) {
        if (strcmp(directorio[i].dir_nfich, nombreantiguo) == 0) {
            index_antiguo = i;
        }
    }

    // Check if the source file name already exists
    if (index_antiguo == -1) {
        printf("Error: File '%s' not found.\n\n", nombreantiguo);
        return -1;
    }

    // Check if the new name already exists
    for (int i = 0; i < MAX_FICHEROS; i++) {
        if (strcmp(directorio[i].dir_nfich, nombrenuevo) == 0) {
            printf("Error: File '%s' already exists.\n\n", nombrenuevo);
            return -1;
        }
    }

    // Change the name of the directory
    strncpy(directorio[index_antiguo].dir_nfich, nombrenuevo, LEN_NFICH - 1);
    directorio[index_antiguo].dir_nfich[LEN_NFICH - 1] = '\0'; // Asegurar terminación nula

    printf("File '%s' has been renamed to '%s'.\n\n", nombreantiguo, nombrenuevo);
    return 0;
}

int Print(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos, EXT_DATOS *memdatos, char *nombre) {
    // Find the file in the directory
    int i = BuscaFich(directorio, inodos, nombre);
    if (i < 0) {
        printf("Error: File '%s' not found.\n\n", nombre);
        return -1;
    }

    // Find the inode of the file
    EXT_SIMPLE_INODE inodo = inodos->blq_inodos[directorio[i].dir_inodo];

    // Check if the file is empty
    if (inodo.size_fichero == 0) {
        printf("Error: File '%s' is empty.\n\n", nombre);
        return 0;
    }

    // Buffer to concatanate the content
    char contenido[SIZE_BLOQUE * MAX_NUMS_BLOQUE_INODO] = {0};

    // Read the blocks of the file
    for (int j = 0; j < MAX_NUMS_BLOQUE_INODO; j++) {
        unsigned short int bloque = inodo.i_nbloque[j];
        if (bloque == NULL_BLOQUE) break; // Break the loop if a block is invalid

        // Concatanate the contents of the block to the buffer
        strncat(contenido, (char *)memdatos[bloque - PRIM_BLOQUE_DATOS].dato, SIZE_BLOQUE);
    }

    printf("File '%s' content:\n%s\n\n", nombre, contenido);

    return 0;
}

int Remove(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos, EXT_BYTE_MAPS *ext_bytemaps, EXT_SIMPLE_SUPERBLOCK *ext_superblock, char *nombre, FILE *fich) {
    int inodo_index = -1;  // Index of the inode
    int dir_index = -1;    // Index of the directory position

    // Find the file in the directory
    for (int i = 0; i < MAX_FICHEROS; i++) {
        if (strcmp(directorio[i].dir_nfich, nombre) == 0) {
            inodo_index = directorio[i].dir_inodo; // Obtain the associated inode
            dir_index = i; // Save the index of the directory position
            break;
        }
    }

    // Check if the file exists
    if (inodo_index == -1) {
        printf("Error: File '%s' not found.\n\n", nombre);
        return -1;
    }

    // Find the inode of the file
    EXT_SIMPLE_INODE *inodo = &inodos->blq_inodos[inodo_index];

    // Free the occupied blocks
    for (int i = 0; i < MAX_NUMS_BLOQUE_INODO; i++) {
        if (inodo->i_nbloque[i] != NULL_BLOQUE) {
            // Mark block as free
            ext_bytemaps->bmap_bloques[inodo->i_nbloque[i]] = 0;
            // Increment the free blocks in the superblock
            ext_superblock->s_free_blocks_count++;
            // Mark block as NULL
            inodo->i_nbloque[i] = NULL_BLOQUE;
        }
    }

    // Free inode
    ext_bytemaps->bmap_inodos[inodo_index] = 0;  // Mark inode as free
    ext_superblock->s_free_inodes_count++;       // Increment free inodes
    inodo->size_fichero = 0;                     // Make the size 0

    // Delete the position for the directory
    memset(directorio[dir_index].dir_nfich, 0, LEN_NFICH); // Empty name
    directorio[dir_index].dir_inodo = NULL_INODO;          // Mark position as empty

    printf("File '%s' has been successfully removed.\n\n", nombre);
    return 0;
}

int Copy(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos, EXT_BYTE_MAPS *ext_bytemaps, EXT_SIMPLE_SUPERBLOCK *ext_superblock, EXT_DATOS *memdatos, char *nombreorigen, char *nombredestino, FILE *fich) {
    int i, i2, j, k;
    unsigned int bloquesorigen = 0, inodolibre = NULL_INODO, bloquelibre = 0;
    unsigned short int blnumber;

    // Check if source file exists
    i = BuscaFich(directorio, inodos, nombreorigen);
    if (i < 0) {
        printf("Error: File '%s' not found.\n", nombreorigen);
        return -1;
    }
    // Check if new file already exists
    i2 = BuscaFich(directorio, inodos, nombredestino);
    if (i2 != -1) {
        printf("Error: File '%s' already exists.\n", nombredestino);
        return -1;
    }

    // Count blocks occupied by source file
    for (j = 0; j < MAX_NUMS_BLOQUE_INODO; j++) {
        blnumber = inodos->blq_inodos[directorio[i].dir_inodo].i_nbloque[j];
        if (blnumber == NULL_BLOQUE) break;
        bloquesorigen++;
    }

    // Find first free inode
    for (j = 0; j < MAX_INODOS; j++) {
        if (ext_bytemaps->bmap_inodos[j] == 0) { // Free inode found
            inodolibre = j;
            ext_bytemaps->bmap_inodos[j] = 1; // Mark inode as occupied
            break;
        }
    }

    if (inodolibre == NULL_INODO) {
        printf("Error: No free inodes available.\n");
        return -2;
    }

    // Find the first free space in the directory
    for (j = 0; j < MAX_FICHEROS; j++) {
        if (directorio[j].dir_inodo == NULL_INODO) {
            directorio[j].dir_inodo = inodolibre;
            strncpy(directorio[j].dir_nfich, nombredestino, LEN_NFICH - 1);
            directorio[j].dir_nfich[LEN_NFICH - 1] = '\0'; // End array in null
            break;
        }
    }

    if (j == MAX_FICHEROS) {
        printf("Error: No space in the directory left for the new file.\n");
        return -3;
    }

    // Copy blocks from the source file to the new file
    for (j = 0; j < bloquesorigen; j++) {
        unsigned short int bloqueorigen = inodos->blq_inodos[directorio[i].dir_inodo].i_nbloque[j];
        if (bloqueorigen == NULL_BLOQUE) break; // Skip occupied blocks

        bloquelibre = 0;
        for (k = 0; k < MAX_BLOQUES_DATOS; k++) {
            if (ext_bytemaps->bmap_bloques[k] == 0) { // Find free block
                bloquelibre = k;
                ext_bytemaps->bmap_bloques[k] = 1; // Mark block as occupied
                ext_superblock->s_free_blocks_count--;
                break;
            }
        }

        if (bloquelibre == 0) {
            printf("Error: No free blocks available.\n");
            return -4;
        }

        // Clean the free block before copying data
        memset(&memdatos[bloquelibre - PRIM_BLOQUE_DATOS], 0, SIZE_BLOQUE);

        // Copy contents from source block to free block
        memcpy(&memdatos[bloquelibre - PRIM_BLOQUE_DATOS], 
               &memdatos[bloqueorigen - PRIM_BLOQUE_DATOS], 
               SIZE_BLOQUE);

        // Register copied block in the new inode
        inodos->blq_inodos[inodolibre].i_nbloque[j] = bloquelibre;
    }

    // Copies the exact size of the source file
    inodos->blq_inodos[inodolibre].size_fichero = inodos->blq_inodos[directorio[i].dir_inodo].size_fichero;

    printf("File '%s' copied successfully as '%s'.\n", nombreorigen, nombredestino);
    return 0;
}