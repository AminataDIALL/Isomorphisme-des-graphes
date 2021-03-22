#include <dirent.h>

#include "nauty27r1/nausparse.h"

#define FORME_MSG "Doit être sous la forme :\n\tComparaison un contre un :\n\t\t\"./comparaison -of path_mol_1 path_mol_2\"\n\
\t\t\"./comparaison -oi id_mol_1 id_mol_2 code_mode path_bdd\"\n\
\tComparasion un contre tous :\n\t\t\"./comparaison -af path_mol_1 path_bdd\"\n\
\t\t\"./comparaison -ai id_mol_1 code_mode path_bdd\"\n"

#define EXT_MSG "Doit être sous la forme :\n\t\"idMode0000.canon\", le 0000 correspondant au code binaire du mode\n"

#define NOT_ISO_MSG(id1, id2)   printf("Les molécules %s et %s ne sont pas analogues structurels sous le mode %s.\n",id1,id2,mode);
#define MODE_EXTENSION_SIZE	14
#define MODE_SIZE 4
#define MODE_INDEX_IN_EXTENSION 4

/**
 * Lit un entier dans le fichier f.
 * Le retourne.
 * Retourne -1 si erreur dans la lecture, et ecrit un message d'erreur dans la console si write est à TRUE.
 */
int read_nv(FILE* f, boolean write) {
	int nv;
	int ret = fread(&nv,sizeof(int),1,f);
	if (ret < 1) {
		if (write)
			fprintf(stderr, "Erreur dans la lecture du nombre de sommets.\n");
		fclose(f);
		return -1;
	}
	return nv;
}

/**
 * Lit un size_t dans le fichier f.
 * Le retourne.
 * Retourne -1 si erreur dans la lecture, et ecrit un message d'erreur dans la console si write est à TRUE.
 */
size_t read_nde(FILE* f, boolean write) {
	size_t nde;
	int ret = fread(&nde,sizeof(size_t),1,f);
	if (ret < 1) {
		if (write)
			fprintf(stderr, "Erreur dans la lecture du nombre d'arêtes.\n");
		fclose(f);
		return -1;
	}
	return nde;
}

/**
 * Lit un tableau de type dans le fichier f.
 * nv correspond à la taille du tableau.
 * Le retourne.
 * Retourne NULL si erreur dans la lecture ou l'allocation, et ecrit un message d'erreur dans la console si write est à TRUE.
 */
int* read_type(int nv, FILE* f, boolean write) {
	int* type;
	type = malloc(nv*sizeof(int));
	if (type == NULL) {
		if (write)
			fprintf(stderr, "Erreur d'allocation du tableau de types.\n");
		fclose(f);
		return NULL;
	}
	int ret = fread(type,sizeof(int),nv,f);
	if(ret < nv) {
		if (write)
			fprintf(stderr, "Erreur dans la lecture du tableau de types.\n");
		free(type);
		fclose(f);
		return NULL;
	}
	return type;
}

/**
 * Lit les trois size_t correspondant aux tailles de tableau dans size_t.
 * Les stocke à leur adresse respective.
 * Retourne TRUE si la lecture s'est passé sans erreur,
 * FALSE sinon, en écrivant le message d'erreur dans la console si write est à true.
 */
boolean read_sparsegraph_tab_sizes(FILE* f, size_t* dlen, size_t* vlen, size_t* elen, boolean write) {
	int ret = fread(dlen,sizeof(size_t),1,f);
	if (ret < 1) {
		if (write)
			fprintf(stderr, "Erreur dans la lecture de la structure sparsegraph : dlen.\n");
		fclose(f);
		return FALSE;
	}
	ret = fread(vlen,sizeof(size_t),1,f);
	if (ret < 1) {
		if (write)
			fprintf(stderr, "Erreur dans la lecture de la structure sparsegraph : vlen.\n");
		fclose(f);
		return FALSE;
	}
	ret = fread(elen,sizeof(size_t),1,f);
	if (ret < 1) {
		if (write)
			fprintf(stderr, "Erreur dans la lecture de la structure sparsegraph : elen.\n");
		fclose(f);
		return FALSE;
	}
	return TRUE;
}

/**
 * Lit les trois tableaux de sparsegraph.
 * Les stocke dans sg.
 * Retourne TRUE si la lecture s'est passé sans erreur,
 * FALSE sinon, en écrivant le message d'erreur dans la console si write est à true.
 */
boolean read_sparsegraph_tab(FILE* f, sparsegraph* sg, boolean write) {
	int ret = fread(sg->d,sizeof(int),sg->dlen,f);
	if (ret < sg->dlen) {
		if (write)
			fprintf(stderr, "Erreur dans la lecture de la structure sparsegraph : d.\n");
		fclose(f);
		return FALSE;
	}
	ret = fread(sg->v,sizeof(size_t),sg->vlen,f);
	if (ret < sg->vlen) {
		if (write)
			fprintf(stderr, "Erreur dans la lecture de la structure sparsegraph : v.\n");
		fclose(f);
		return FALSE;
	}
	ret = fread(sg->e,sizeof(int),sg->elen,f);
	if (ret < sg->elen) {
		if (write)
			fprintf(stderr, "Erreur dans la lecture de la structure sparsegraph : e.\n");
		fclose(f);
		return FALSE;
	}
	return TRUE;
}

/**
 * Lit la suite de la structure nauty sparsegraph du canonique contenue dans le fichier f.
 * nv et nde déjà lus sont précisés en arguments.
 * La renvoie.
 * Renvoie une structure avec nv = -1, en cas d'erreur de lecture, et ecrit un message d'erreur dans la console si write est à TRUE.
 */
sparsegraph read_sparsegraph(int nv, size_t nde, FILE* f, boolean write) {
	SG_DECL(sg);
	
	SG_ALLOC(sg,nv,nde,"Erreur dans l'allocation de la structure sparsegraph.\n");
	sg.nv = nv;
	sg.nde = nde;
	
	if (!read_sparsegraph_tab_sizes(f, &sg.dlen, &sg.vlen, &sg.elen, write)) {
		sg.nv = -1;
		return sg;
	}
	
	if (!read_sparsegraph_tab(f, &sg, write)) {
		sg.nv = -1;
		return sg;
	}
	
	return sg;
}

/**
 * Vérifie que la chaine de caractères mode_extension est bien de la forme
 * "Mode0000.canon" avec 0000 code binaire (donc soit 0 soit 1).
 * Retourne TRUE si bien de cette forme là,
 * FALSE, sinon;
 */
boolean verify_mode_extension(char* mode_extension) {
	return mode_extension[0]=='M' && mode_extension[1]=='o'
		&& mode_extension[2]=='d' && mode_extension[3]=='e'
		&& (mode_extension[4]=='0' || mode_extension[4]=='1')
		&& (mode_extension[5]=='0' || mode_extension[5]=='1')
		&& (mode_extension[6]=='0' || mode_extension[6]=='1')
		&& (mode_extension[7]=='0' || mode_extension[7]=='1')
		&& mode_extension[8]=='.' && mode_extension[9]=='c'
		&& mode_extension[10]=='a' && mode_extension[11]=='n'
		&& mode_extension[12]=='o' && mode_extension[13]=='n'
		&& mode_extension[14]=='\0';
}

/**
 * Vérifie que la chaine de caractères mode est bien un code binaire de taille 4.
 * Retourne TRUE si oui,
 * FALSE sinon.
 */
boolean verify_mode(char* mode) {
	if (strlen(mode) != 4) {
		return FALSE;
	}
	return (mode[0]=='0' || mode[0]=='1')
		&& (mode[1]=='0' || mode[1]=='1')
		&& (mode[2]=='0' || mode[2]=='1')
		&& (mode[3]=='0' || mode[3]=='1');
}

/**
 * Extrait le mode du chemin path_mol
 * Et l'écrit dans mode. Et le vérifie avec l'extension.
 * Extrait l'identifiant de la molécule (le nom du fichier, sans le mode_extension)
 * Et l'écrit dans id_mol.
 * Retourne TRUE si l'extension est correcte.
 * FALSE, sinon, et affiche le message.
 */
boolean get_mode_id(char* path_mol, char* mode, char* id_mol) {
	//Mode extension
	char mode_extension[MODE_EXTENSION_SIZE+1];
	int len = strlen(path_mol);
	strcpy(mode_extension, path_mol+len-MODE_EXTENSION_SIZE);
	
	//Id
	int i = len-MODE_EXTENSION_SIZE+1;
	while(i>=0 && path_mol[i]!='/') {
		i--;
	}
	i++;
	int j;
	for(j=i; j<len-MODE_EXTENSION_SIZE; j++) {
		id_mol[j-i] = path_mol[j];
	}
	id_mol[j-i]='\0';
	
	//Verification mode extension
	if (!verify_mode_extension(mode_extension)) {
		fprintf(stderr, "Erreur dans l'extension du fichier %s\n", path_mol);
		printf(EXT_MSG);
		return FALSE;
	}
	
	//Mode
	for (i=0; i<MODE_SIZE; i++) {
		mode[i]=mode_extension[i+MODE_INDEX_IN_EXTENSION];
	}
	mode[i]='\0';
	return TRUE;
}

/**
 * Extrait les modes des deux chemins de fichiers path1 et path2.
 * Stocke le mode dans mode.
 * Extrait les id et les stocke respectivement dans id1 et id2.
 * Vérifie si les extensions sont correctes, et les modes sont égaux.
 * Retourne TRUE si oui.
 * FALSE, sinon, et affiche le message.
 */
boolean get_verify_and_compare_mode_id(char* path1, char* path2, char* mode, char* id1, char* id2) {
	char mode2[MODE_SIZE+1];
	if (!get_mode_id(path1, mode, id1)) {
		return FALSE;
	}
	if (!get_mode_id(path2, mode2, id2)) {
		return FALSE;
	}
	if (strcmp(mode,mode2)) {
		printf("Mode de comparaison différents entre les deux molécules.\n");
		return FALSE;
	}
	return TRUE;
}

/**
 * Construit le chemin du fichier avec path_bdd/id/idMode0000.canon.
 * Met ce chemin dans path.
 */
void path_construct(char* path_bdd, char* id, char* mode, char* path) {
	sprintf(path, "%s/%s/%sMode%s.canon", path_bdd, id, id, mode);
}

/**
 * Suite à l'ouverture d'un fichier f, teste si celui-ci a été bien ouvert.
 * Si oui, retourne FALSE.
 * Sinon, retourne TRUE et affiche un message dans la console si write est à TRUE.
 * path correspond au chemin du fichier (utile seulement pour l'affichage)
 */
boolean open_error_test(FILE* f, boolean write, char* path) {
	if (f == NULL) {
		if (write) {
			printf("Erreur d'ouverture du fichier ");
			printf(path);
			printf("\n");
		}
		return TRUE;
	}
	return FALSE;
}

/**
 * Lit un entier correspondant au nombre de sommets d'un fichier f,
 * Place celui-ci dans nv2.
 * Le compare avec nv1.
 * Si egaux, retourne TRUE.
 * Sinon ou si erreur, retourne FALSE, ferme f et affiche un message si write est à TRUE.
 */
boolean read_and_compare_nv(int nv1, FILE* f, int* nv2, boolean write) {
	*nv2 = read_nv(f, write);
	if (*nv2 == -1) {
		return FALSE;
	}
	if (nv1 != *nv2) {
		if (write)
			printf("Nombres de sommets différents : Non analogues structurels.\n");
		fclose(f);
		return FALSE;
	}
	return TRUE;
}

/**
 * Lit les entiers correspondant au nombre de sommets de deux fichiers f1 et f2,
 * Place ceux-ci dans nv1 et nv2.
 * Les compare.
 * Si egaux, retourne TRUE.
 * Sinon ou si erreur, retourne FALSE, ferme f1 et f2 et affiche un message dans la console.
 */
boolean read_and_compare_nv_2(FILE* f1, FILE* f2, int* nv1, int* nv2) {
	*nv1 = read_nv(f1, TRUE);
	if (*nv1 == -1) {
		fclose(f2);
		return FALSE;
	}
	if (!read_and_compare_nv(*nv1, f2, nv2, TRUE)) {
		fclose(f1);
		return FALSE;
	}
	return TRUE;
}

/**
 * Lit un size_t correspondant au nombre d'arêtes d'un fichier f,
 * Place celui-ci dans nde2.
 * Le compare avec nde1.
 * Si egaux, retourne TRUE.
 * Sinon ou si erreur, retourne FALSE, ferme f et affiche un message si write est à TRUE.
 */
boolean read_and_compare_nde(size_t nde1, FILE* f, size_t* nde2, boolean write) {
	*nde2 = read_nde(f, write);
	if (*nde2 == -1) {
		return FALSE;
	}
	if (nde1 != *nde2) {
		if (write)
			printf("Nombres d'arêtes différents : Non analogues structurels.\n");
		fclose(f);
		return FALSE;
	}
	return TRUE;
}

/**
 * Lit les size_t correspondant au nombre d'arêtes de deux fichiers f1 et f2,
 * Place ceux-ci dans nde1 et nde2.
 * Les compare.
 * Si egaux, retourne TRUE.
 * Sinon ou si erreur, retourne FALSE, ferme f1 et f2 et affiche un message dans la console.
 */
boolean read_and_compare_nde_2(FILE* f1, FILE* f2, size_t* nde1, size_t* nde2) {
	*nde1 = read_nde(f1, TRUE);
	if (*nde1 == -1) {
		fclose(f2);
		return FALSE;
	}
	if (!read_and_compare_nde(*nde1, f2, nde2, TRUE)) {
		fclose(f1);
		return FALSE;
	}
	return TRUE;
}

/**
 * Compare deux tableau d'entier de même taille taille.
 * Si égaux, retourne TRUE.
 * Sinon, retourne FALSE.
 */
boolean compare_type(int* type1, int* type2, int taille) {
	for(int i=0; i<taille; i++) {
		if(type1[i] != type2[i]) {
			return FALSE;
		}
	}
	return TRUE;
}

/**
 * Lit le tableau de types de taille nv d'un fichier f.
 * Le compare avec type1.
 * Si egaux, retourne TRUE.
 * Sinon ou si erreur, retourne FALSE, ferme f et affiche un message si write est à TRUE.
 */
boolean read_and_compare_type(int* type1, int nv, FILE* f, boolean write) {
	int* type2 = read_type(nv, f, write);
	if (type2 == NULL) {
		return FALSE;
	}
	if(!compare_type(type1, type2, nv)) {
		if (write)
			printf("Types différents : Non analogues structurels.\n");
		fclose(f);
		free(type2);
		return FALSE;
	}
	free(type2);
	return TRUE;
}

/**
 * Lit les tableaux de types de taille nv des fichiers f1 et f2.
 * Les compare.
 * Si egaux, retourne TRUE.
 * Sinon ou si erreur, retourne FALSE, ferme f1 et f2 et affiche un message.
 */
boolean read_and_compare_type_2(int nv, FILE* f1, FILE* f2) {
	int* type1, * type2;
	type1 = read_type(nv, f1, TRUE);
	if (type1 == NULL) {
		fclose(f2);
		return FALSE;
	}
	if (!read_and_compare_type(type1, nv, f2, TRUE)) {
		fclose(f1);
		free(type1);
		return FALSE;
	}
	free(type1);
	return TRUE;
}

/**
 * Lit les deux structures sparsegraph de nv sommets et nde arêtes des fichiers f1 et f2.
 * Les compare.
 * Si egaux, retourne TRUE.
 * Sinon ou si erreur, retourne FALSE, ferme f1 et f2 et affiche un message.
 */
boolean read_and_compare_sparsegraph_2(int nv, size_t nde, FILE* f1, FILE* f2) {
	sparsegraph sg1, sg2;
	sg1 = read_sparsegraph(nv, nde, f1, TRUE);
	if (sg1.nv == -1) {
		fclose(f2);
		SG_FREE(sg1);
		return FALSE;
	}
	sg2 = read_sparsegraph(nv, nde, f2, TRUE);
	if (sg2.nv == -1) {
		fclose(f1);
		SG_FREE(sg1);
		SG_FREE(sg2);
		return FALSE;
	}
	fclose(f1);
	fclose(f2);
	if(!aresame_sg(&sg1, &sg2)) {
		printf("Graphes non isomorphes : Non analogues structurels.\n");
		SG_FREE(sg1);
		SG_FREE(sg2);
		return FALSE;
	}
	else {
		printf("Graphes isomorphes.\n");
		SG_FREE(sg1);
		SG_FREE(sg2);
		return TRUE;
	}
}

/**
 * Comparaison un contre un des graphes canoniques écrit dans les fichiers path1 et path2, selon le mode mode.
 * Les ids des molécules sont respectivement id1 et id2.
 */
void one_to_one(char* path1, char* path2, char* mode, char* id1, char* id2) {
	//Ouverture des fichiers.
	FILE* f1 = fopen(path1,"r");
	if (open_error_test(f1, TRUE, path1))
		return;
	FILE* f2 = fopen(path2,"r");
	if (open_error_test(f2, TRUE, path2)) {
		fclose(f1);
		return;
	}
	
	//Nombre de sommets.
	int nv1, nv2;
	if (!read_and_compare_nv_2(f1, f2, &nv1, &nv2)) {
		NOT_ISO_MSG(id1, id2);
		return;
	}
	
	//Nombre d'arêtes.
	size_t nde1, nde2;
	if (!read_and_compare_nde_2(f1, f2, &nde1, &nde2)) {
		NOT_ISO_MSG(id1, id2);
		return;
	}
	
	//Types.
	if (!read_and_compare_type_2(nv1, f1, f2)) {
		NOT_ISO_MSG(id1, id2);
		return;
	}
	
	//Graphes.
	if(!read_and_compare_sparsegraph_2(nv1, nde1, f1, f2)) {
		NOT_ISO_MSG(id1, id2);
	}
	else {
		printf("Les molécules %s et %s sont analogues structurels sous le mode %s.\n",
			id1,id2,mode);
	}
}

/**
 * Appel de la comparaison un contre un avec en argument les chemins des fichiers.
 * Vérifie l'extension des fichiers et y extrait les ids et mode.
 */
void one_to_one_with_file(char* path1, char* path2) {
	char mode[MODE_SIZE+1];
	char id1[30];
	char id2[30];
	if (get_verify_and_compare_mode_id(path1, path2, mode, id1, id2)) {
		one_to_one(path1, path2, mode, id1, id2);
	}
}

/**
 * Appel de la comparaison un contre un avec en argument les ids.
 * Vérifie le mode.
 * Construit les chemins de fichiers.
 */
void one_to_one_with_id(char* id1, char* id2, char* mode, char* path_bdd) {
	if (!verify_mode(mode)) {
		printf("Mode inconnu.\n");
		return;
	}
	char path1[500];
	char path2[500];
	path_construct(path_bdd, id1, mode, path1);
	path_construct(path_bdd, id2, mode, path2);
	one_to_one(path1, path2, mode, id1, id2);
}



/**
 * Compare la molécule constitué du graphe sg_mol et du tableu de types type_mol
 * Avec celle écrite dans le fichier path_mol_2.
 * Si égales, return TRUE.
 * Sinon, return FALSE.
 */
boolean compare_mol(sparsegraph sg_mol, int* type_mol, char* path_mol_2) {
	FILE* f2 = fopen(path_mol_2,"r");
	if (open_error_test(f2, FALSE, NULL))
		return FALSE;
	
	//Nombre de sommets.
	int nv2;
	if (!read_and_compare_nv(sg_mol.nv, f2, &nv2, FALSE))
		return FALSE;
	
	//Nombre d'arêtes.
	size_t nde2;
	if (!read_and_compare_nde(sg_mol.nde, f2, &nde2, FALSE))
		return FALSE;
	
	//Types.
	int* type2;
	if (!read_and_compare_type(type_mol, sg_mol.nv, f2, FALSE))
		return FALSE;
	
	//Graphes.
	sparsegraph sg2;
	sg2 = read_sparsegraph(nv2, nde2, f2, FALSE);
	if (sg2.nv == -1) {
		SG_FREE(sg2);
		return FALSE;
	}
	fclose(f2);
	if(sg2.nv == -1 || !aresame_sg(&sg_mol, &sg2)) {
		SG_FREE(sg2);
		return FALSE;
	}
	SG_FREE(sg2);
	return TRUE;
}

/**
 * Lis la structure sparsegraph et sans tableau de type stockés dans le fichier au chemin path_mol.
 * Les stocke dans sg_mol et type_mol.
 * Retourne TRUE si la ecture s'est passée sans erreur.
 * FALSE sinon, avec message d'erreur dans la console.
 */
boolean sparsegraph_file_treatment(char* path_mol, sparsegraph* sg_mol, int** type_mol) {
	//Ouverture du fichier.
	FILE* f_mol = fopen(path_mol,"r");
	if (open_error_test(f_mol, TRUE, path_mol))
		return FALSE;
	
	//Lecture du path_mol.
	int nv_mol = read_nv(f_mol, TRUE);
	if (nv_mol==-1)
		return FALSE;
	int nde_mol = read_nde(f_mol, TRUE);
	if (nde_mol==-1)
		return FALSE;
	*type_mol = read_type(nv_mol, f_mol, TRUE);
	if (*type_mol==NULL)
		return FALSE;
	*sg_mol = read_sparsegraph(nv_mol, nde_mol, f_mol, TRUE);
	if (sg_mol->nv == -1) {
		free(*type_mol);
		SG_FREE(*sg_mol);
		return FALSE;
	}
	fclose(f_mol);
	return TRUE;
}

/**
 * Parcours le dossier d_bdd.
 * Pour chaque dossier contenu, compare la molécule contenue (pour le même mode) avec sg_mol, type_mol.
 * Si isomorphe, écrit sa référence dans la console.
 * Retourne le nombre d'isomorphismes trouvés.
 */
int repertory_treatment(DIR* d_bdd, char* path_bdd, char* mode, char* id_mol, int* type_mol, sparsegraph sg_mol) {
	char path_mol_2[500];
	struct dirent* dir;
	int nb_iso = 0;
	while ((dir = readdir(d_bdd)) != NULL) {
		path_construct(path_bdd, dir->d_name, mode, path_mol_2);
		if (strcmp(id_mol, dir->d_name)) {
			if (compare_mol(sg_mol, type_mol, path_mol_2)) {
				nb_iso++;
				printf("Analogue structurel %d : %s\n", nb_iso, dir->d_name);
			}
		}
	}
	closedir(d_bdd);
	return nb_iso;
}

/**
 * Comparaison un contre tous.
 * Une molécule écrite dans le fichier path1.
 * Contre toutes celles dans la base situé à path_bdd.
 */
void one_to_all(char* path_mol, char* id_mol, char* mode, char* path_bdd) {
	//Recupération du graphe et type
	sparsegraph sg_mol;
	int* type_mol;
	if (!sparsegraph_file_treatment(path_mol, &sg_mol, &type_mol)) {
		return;
	}
	
	//Dossier correspondant au nombre de sommets.
	DIR* d_bdd = opendir(path_bdd);
	if (d_bdd == NULL) {
		printf("Base de données vide.\n");
		free(type_mol);
		SG_FREE(sg_mol);
		return;
	}
	
	//Parcours du dossier.
	int nb_iso = repertory_treatment(d_bdd, path_bdd, mode, id_mol, type_mol, sg_mol);
	
	free(type_mol);
	SG_FREE(sg_mol);
	printf("Nombre d'analogues structurels trouvés pour %s selon le mode %s : %d\n",
		id_mol, mode, nb_iso);
}

/**
 * Appel de la comparaison un contre tous avec en argument le chemin du fichier.
 * Vérifie l'extension du fichier et y extrait id et mode.
 */
void one_to_all_with_file(char* path_mol, char* path_bdd) {
	char mode[MODE_SIZE+1];
	char id_mol[30];
	if (!get_mode_id(path_mol, mode, id_mol)) {
		return;
	}
	one_to_all(path_mol, id_mol, mode, path_bdd);
}

/**
 * Appel de la comparaison un contre tous avec en argument les ids.
 * Vérife le mode.
 * Construit le chemin du fichier.
 */
void one_to_all_with_id(char* id_mol, char* mode, char* path_bdd) {
	if (!verify_mode(mode)) {
		printf("Mode inconnu.\n");
		return;
	}
	char path_mol[500];
	path_construct(path_bdd, id_mol, mode, path_mol);
	one_to_all(path_mol, id_mol, mode, path_bdd);
}

/**
 * Traitement des arguments de la ligne de commande dans le cas d'une comparaison un contre un.
 * Ne lance la comparaison que si la spécification de la ligne de commande (id ou fichier) est correcte,
 * Et si le nombre d'arguments est strictement égal à celui-ci demandé.
 */
void one_to_one_command_treatment(int argc, char** argv) {
	//-of : avec fichier
	if (argv[1][2]=='f') {
		if (argc > 4) {
			printf("Trop d'arguments.\n");
			printf(FORME_MSG);
			return;
		}
		one_to_one_with_file(argv[2], argv[3]);
	}
	//-oi : avec id
	else if (argv[1][2]=='i') {
		if (argc < 6) {
			printf("Manque des arguments.\n");
			printf(FORME_MSG);
			return;
		}
		if (argc > 6) {
			printf("Trop d'arguments.\n");
			printf(FORME_MSG);
			return;
		}
		one_to_one_with_id(argv[2], argv[3], argv[4], argv[5]);
	}
	else {
		printf("Spéfication d'écriture de la demande de comparaison non reconnue.\n");
		printf(FORME_MSG);
		return;
	}
}

/**
 * Traitement des arguments de la ligne de commande dans le cas d'une comparaison un contre tous.
 * Ne lance la comparaison que si la spécification de la ligne de commande (id ou fichier) est correcte,
 * Et si le nombre d'arguments est strictement égal à celui-ci demandé.
 */
void one_to_all_command_treatment(int argc, char** argv) {
	//-af : avec fichier
	if (argv[1][2]=='f') {
		if (argc > 4) {
			printf("Trop d'arguments.\n");
			printf(FORME_MSG);
			return;
		}
		one_to_all_with_file(argv[2], argv[3]);
	}
	//-ai : avec id
	else if (argv[1][2]=='i') {
		if (argc < 5) {
			printf("Manque des arguments.\n");
			printf(FORME_MSG);
			return;
		}
		if (argc > 5) {
			printf("Trop d'arguments.\n");
			printf(FORME_MSG);
			return;
		}
		one_to_all_with_id(argv[2], argv[3], argv[4]);
	}
	else {
		printf("Spéfication d'écriture de la demande de comparaison non reconnue.\n");
		printf(FORME_MSG);
		return;
	}
}

int main(int argc, char** argv) {
	if (argc < 4) {
		printf("Manque des arguments.\n");
		printf(FORME_MSG);
		return 0;
	}
	
	if (argv[1][0]!='-' || strlen(argv[1])!=3) {
		printf("Mode de comparaison non reconnu.\n");
		printf(FORME_MSG);
		return 0;
	}
	
	//-o : one to one
	if (argv[1][1]=='o') {
		one_to_one_command_treatment(argc, argv);
	}
	//-a : one to all
	else if (argv[1][1]=='a') {
		one_to_all_command_treatment(argc, argv);
	}
	else {
		printf("Mode de comparaison non reconnu.\n");
		printf(FORME_MSG);
		return 0;
	}
	return 0;
}
