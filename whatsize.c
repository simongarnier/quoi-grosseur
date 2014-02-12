/*
Nom et prénom: Garnier, Simon
Code permanent: GARS13069209
*/
#include <errno.h>
#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>

#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include "util.h"

typedef struct Node
{
	int 		st_ino;
	int			size; 	//contient la taille total de l'element, incluant les elements enfants dans le cas d'un repertoire
	char 		*name;	
	struct 		Node 	*next;
}element;

/*signature*/
//fonction pour manipuler la liste
element *	creerElement	(int, int, char *);
element *	elemSwitch		(element *, element *);
element *	trierList		(element *);
void 		printList		(element *);
void 		freeList		(element *);
//fonction utilitaire
int 		parcourirRacine	(char *, element *, int);
int 		rechercheInode	(int, element *);
void		printErrno		();

int 
main(int argc, char *argv[])
{	
	if (argc !=2){
		print_erreur(ERR_AUTRE);
		exit(1);
	}
	element *listRacine = creerElement(-1, 0, argv[1]);
	if(parcourirRacine(argv[1], listRacine, 1) == -1){
		printErrno();
		exit(1);
	}
	printList(trierList(listRacine));
	freeList(listRacine);
	exit(0)
}

/*
@param 		int 		le numero d'inode de l'element
@param 		int 		la taille de l'element(incluant les fichier et repertoire enfant)
@param 		char *		le nom de l'element
@return					un pointeur sur l'element creer
*/
element *
creerElement(int st_ino, int size, char *name){
	element *e 	= (element *) malloc(sizeof(element));
	e -> st_ino = st_ino;
	e -> size 	= size;
	e -> name 	= strdup(name);
	return e;
}

/*
@param		element *	le premier element
@param		element *	le deuxieme element
@return 				un pointeur sur l'element ayant pris la place de e1
*/
element *
elemSwitch(element *e1, element *e2){
	e1 -> next = e2 -> next;
	e2 -> next = e1;
	return e2;
}

/*
@param 		element *	la tete de la list a trier
@return 				un pointeur sur le nouvel element place en tete de list
*/
element *
trierList(element *head){
	element *p, *q, *top;
	int changed = 1;
	top = malloc(sizeof(element));
	top -> next = head;
	if (head != NULL && head -> next != NULL){
		while(changed){
			changed = 0;
			q = top;
			p = top -> next;
			while(p -> next != NULL){
				if (p -> size == p -> next -> size ){
					if (strcmp (p->name, q->name) > 0){
						q -> next = elemSwitch(p, p->next);
						changed = 1;
					}
				}else if (p -> size < p -> next -> size ){
					q -> next = elemSwitch(p, p->next);
					changed = 1;
				}
				q = p;
				if (p -> next != NULL)
					p = p -> next;
				
			}
		}
	}
	p = top -> next;
	free( top );
	return p;
}

/*
@param		element *	la tete de la list a imprimer	
*/
void 
printList(element *head) {
    element *iter = head;
    while (iter != NULL) {
        print_element(iter -> size, iter -> name);
        iter = iter->next;
    }
}

/*
@param 		element *	la tete de la list a liberer
*/
void 
freeList(element *head){
	element *tmp;
	while (head != NULL){
		tmp = head;
		free(tmp -> name);
		head = head -> next;
		free(tmp);
	}
}

/*
@param		char *		le path de l'element demande
@param 		element *	la tete de la liste ou seront stocke les element 
@param 		int 		permet de savoir si on est au premier niveau de recursion(1) ou non(0)
@return 				la taille de l'element(incluant les fichier et repertoire enfant)
*/
int
parcourirRacine(char *path, element *head, int racine){
	int 			total = 0;
	struct 	stat 	st;
	struct 	dirent 	*direntp;
	char 			*path_enfant;
	DIR 			*dir;

	errno = 0;
	if((dir = opendir(path)) == NULL){
		return -1;
	}

	element *current = head;
	while((direntp = readdir(dir)) != NULL){
		if (strcmp(direntp->d_name, ".") && strcmp(direntp->d_name, "..")){

			path_enfant = (char *) malloc(strlen(path) + strlen(direntp->d_name) + 2);
			snprintf(path_enfant, strlen(path) + strlen(direntp->d_name) + 2, "%s/%s", path, direntp->d_name);

			if(lstat(path_enfant, &st) == 0){
				if (!rechercheInode(st.st_ino, head)){
					total += st.st_size;
					if (racine){
						current -> next = creerElement(st.st_ino, st.st_size, direntp->d_name);
						current = current -> next;
					}
					if(S_ISDIR(st.st_mode)){
						int result = parcourirRacine(path_enfant, NULL, 0);
						if (result == -1){
							return -1;
						}else{
							if (racine)
								current -> size += result;
						}
					}
				}
			}else{
				return -1;
			}
		}
	}
	if (dir != NULL)
	    closedir(dir);
	return total;
}

/*
@param		int 		le numero d'inode demande
@param		element *	la tete de la list sur laquelle la recherche sera faite
@return 				1 si trouve, 0 sinon
*/
int
rechercheInode(int st_ino, element *head)
{
	element *iter = head;

    while (iter != NULL) {
    	if (iter -> st_ino == st_ino){
    		return 1;
    	}
        iter = iter->next;
    }
    return 0;
}

void
printErrno()
{
	if (errno == 2){
		print_erreur(ERR_FICHIER_REPERTOIRE_INEXISTANT);
	}else if(errno == 13){
		print_erreur(ERR_DROITS_INSUFFISANTS);
	}else{
		print_erreur(ERR_AUTRE);
	}
}

