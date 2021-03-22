#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include "nauty27r1/nausparse.h"  //Je vais chercher nausparse.h dans le dossier nauty que j'ai mis dans le même répertoire que canonisation.c

//Pour compiler rajouter chemin/nauty.a donc pour moi gcc canonisation.c nauty27r1/nauty.a

/**
*Pour lancer le programme, il faut appeler a.out avec comme argument le fichier que l'on souhaite cannoniser. Le résultat sera *nomdufichier*.canon
*/

/**
*On vérifie que le fichier f a bien été ouvert et on renvoie TRUE si c'est le cas et FALSE sinon. 
 */
boolean open_error_test(FILE* f, boolean write) {
	if (f == NULL) {
		if (write) {
			printf("Erreur d'ouverture du fichier \n");						
		}
		return TRUE;
	}
	return FALSE;
}


/**
 * Ecrit la structure nauty sparsegraph du canonique, ainsi que la coloration, dans f
 */
int write_sparsegraph(sparsegraph sg, int* type, FILE* f) {
	int ret = fwrite(&sg.nv,sizeof(int),1,f);
	if(ret < 1) {					
		return 1;		
	}
	ret=fwrite(&sg.nde,sizeof(size_t),1,f);
	if(ret < 1) {					
		return 1;			
	}
	
	ret=fwrite(type,sizeof(int),sg.nv,f); 
	if(ret < sg.nv) {					
		return 1;		
	}

	ret=fwrite(&sg.dlen,sizeof(size_t),1,f);
	if(ret < 1) {					
		return 1;		
	}
	ret=fwrite(&sg.vlen,sizeof(size_t),1,f);
	if(ret < 1) {					
		return 1;			
	}
	ret=fwrite(&sg.elen,sizeof(size_t),1,f);
	if(ret < 1) {					
		return 1;				
	}
	
	ret=fwrite(sg.d,sizeof(int),sg.dlen,f);
	if(ret < sg.dlen) {					
		return 1;			
	}
	ret=fwrite(sg.v,sizeof(size_t),sg.vlen,f);
	if(ret < sg.vlen) {					
		return 1;				
	}
	ret=fwrite(sg.e,sizeof(int),sg.elen,f);
	if(ret < sg.elen) {					
		return 1;			
	}

	return 0;
}

/**
 * Lit la structure nauty sparsegraph 
 */
sparsegraph read_sparsegraph(FILE* f, int* read) {
	int nv, ret;
	size_t nde;	
	SG_DECL(sg);
	
	ret=ret = fread(&nv,sizeof(int),1,f);
	if(ret < 1) {					
		*read = 1;
		return sg;				
	}

	ret=fread(&nde,sizeof(size_t),1,f);	
	if(ret < 1) {					
		*read = 1;	
		return sg;			
	}

			
	SG_ALLOC(sg,nv,nde,"malloc sg");	
	sg.nv = nv;
	sg.nde = nde;
	
	ret=fread(&sg.dlen,sizeof(size_t),1,f);
	if(ret < 1) {					
		*read = 1;	
		return sg;			
	}

	ret=fread(&sg.vlen,sizeof(size_t),1,f);
	if(ret < 1) {					
		*read = 1;	
		return sg;			
	}

	ret=fread(&sg.elen,sizeof(size_t),1,f);
	if(ret < 1) {					
		*read = 1;
		return sg;				
	}
	
	ret=fread(sg.d,sizeof(int),sg.dlen,f);
	if(ret < sg.dlen) {					
		*read = 1;	
		return sg;			
	}

	ret=fread(sg.v,sizeof(size_t),sg.vlen,f);
	if(ret < sg.vlen) {					
		*read = 1;
		return sg;				
	}

	ret=fread(sg.e,sizeof(int),sg.elen,f);
	if(ret < sg.elen) {					
		*read = 1;
		return sg;				
	}

	*read = 0;
	
	return sg;
}


/**
* Lit le type depuis le fichier
*/
boolean read_type(FILE* f, int* type, int nv) {
	
	int ret = fread(type,sizeof(int),nv,f);
	if(ret < nv) {					
		return TRUE;				
	}
	return FALSE;
	
}


/**
* Lit lab
*/
boolean read_lab(FILE* f,int* lab, int nv){
	
	int ret = fread(lab,sizeof(int),nv,f);
	if(ret < nv) {				
		return TRUE;				
	}
	return FALSE;
	
}

/**
* Lit ptn
*/
boolean read_ptn(FILE* f,int* ptn, int nv){
	
	int ret = fread(ptn,sizeof(int),nv,f);
	if(ret < nv) {				
		return TRUE;				
	}
	return FALSE;
	
}

/**
* Libére la mémoire 
*/
#define free_general(lab, lab_sz, ptn, ptn_sz, orbits, orbits_sz, temp, type, sg, cg) \
	DYNFREE(lab,lab_sz); \
	DYNFREE(ptn,ptn_sz);	\
	DYNFREE(orbits,orbits_sz); \
	free(temp);	\
	free(type);	\
	SG_FREE(sg); \
	SG_FREE(cg);\

/**
* Fonction qui cannonise et appelle les autres fonctions
*/
int cannonisation(const char* path, boolean simple){

    //Declaration 
    DYNALLSTAT(int,lab,lab_sz);
    DYNALLSTAT(int,ptn,ptn_sz);
    DYNALLSTAT(int,orbits,orbits_sz);  
    

    SG_DECL(cg);      //Declaration du graphe canonique
    char* temp = malloc(strlen(path)+7*(sizeof(char)));		//On alloue à temp la taille de path+.canon

    static DEFAULTOPTIONS_SPARSEGRAPH(options);
    statsblk stats;

    options.getcanon = TRUE;
    options.defaultptn = FALSE;

    //On lit le fichier   
    FILE* f = fopen(path,"r");
    if (open_error_test(f, TRUE)){
    	free(temp);
    	SG_FREE(cg);
    	if(simple)
            printf("Erreur d'ouverture du fichier\n");
		return 1;
    }

    int read = 0;
    sparsegraph sg = read_sparsegraph(f,&read);				//On lit la première partie du fichier
    if (read)
    {
    	printf("Erreur de lecture du fichier\n");    	
    	//free_general(NULL,0,NULL,0,NULL,0,temp,NULL,sg,cg);
        free(temp);
        SG_FREE(sg);
        SG_FREE(cg);
    	fclose(f);
    	return 1;
    }  

    //On déclare la variable type
    int* type = malloc(sg.nv*sizeof(int));  


    //Allocation de mémoire pour lab ptn et orbits    
    DYNALLOC1(int,lab,lab_sz,sg.nv,"Allocation lab");

    DYNALLOC1(int,ptn,ptn_sz,sg.nv,"Allocation ptn");

    DYNALLOC1(int,orbits,orbits_sz,sg.nv,"Allocation orbits");	
    
	
	//On assigne à lab et ptn les valeurs lues dans le fichier	
      
    if(read_lab(f, lab, sg.nv)){					
    	printf("Erreur de lecture de lab\n");
    	free_general(lab,lab_sz,ptn,ptn_sz,orbits,orbits_sz,temp,type,sg,cg);
    	fclose(f);
    	return 1;
    }  
    if(read_ptn(f, ptn, sg.nv)){					
    	printf("Erreur de lecture de ptn\n");
    	free_general(lab,lab_sz,ptn,ptn_sz,orbits,orbits_sz,temp,type,sg,cg);
    	fclose(f);
    	return 1;
    }
    
		
    
    if (type == NULL) {								//On vérifie que type soit bien alloué
		printf("Erreur d'allocation de type\n");										
		free_general(lab,lab_sz,ptn,ptn_sz,orbits,orbits_sz,temp,type,sg,cg);
    	fclose(f);
		return 1;
	}
    if(read_type(f, type, sg.nv)){					//On lit le type
    	printf("Erreur de lecture de type\n");
    	free_general(lab,lab_sz,ptn,ptn_sz,orbits,orbits_sz,temp,type,sg,cg);
    	fclose(f);
    	return 1;
    }
    

    //--Cannonisation--//

	

    //Appel de la cannonisation    
	sparsenauty(&sg,lab,ptn,orbits,&options,&stats,&cg);		

	//Ecriture
	
		
	if (temp == NULL) {						//On verifie que temp soit bien alloué
		printf("Erreur d'allocation de temp\n");	
		free_general(lab,lab_sz,ptn,ptn_sz,orbits,orbits_sz,temp,type,sg,cg);
    	fclose(f);
		return 1;
	}
	if (strcpy(temp,path)){				//temp prend la valeur de path
		if (strcat(temp,".canon")){									//On rajoute .canon à la fin
			fclose(f);
			f = fopen(temp,"w");		
			if (open_error_test(f, TRUE)){
    			free_general(lab,lab_sz,ptn,ptn_sz,orbits,orbits_sz,temp,type,sg,cg);    			
    			printf("Erreur de création du fichier .canon\n");
				return 1;
    		}							
			if(write_sparsegraph(cg, type, f)){
		    	printf("Erreur d'écriture dans le fichier .canon\n");		    	
		    	free_general(lab,lab_sz,ptn,ptn_sz,orbits,orbits_sz,temp,type,sg,cg);
    			fclose(f);
		    	return 1;
		    }   						
		}
		else{
			free_general(lab,lab_sz,ptn,ptn_sz,orbits,orbits_sz,temp,type,sg,cg);
    		fclose(f);
    		printf("Erreur de concaténation\n");
			return 1;
		}
	}
	else{
		free_general(lab,lab_sz,ptn,ptn_sz,orbits,orbits_sz,temp,type,sg,cg);
    	fclose(f);
    	printf("Erreur de copy dans temp\n");
		return 1;
	}

	//Libération de la mémoire	
	free_general(lab,lab_sz,ptn,ptn_sz,orbits,orbits_sz,temp,type,sg,cg);
    fclose(f);
	return 0;	
}

//fonction qui canonise tout les fichiers parse qui existent
void canonAll(int argc, char const *argv[])
{
	char path_mol[500];
	struct dirent* dir;
	DIR* d_bdd = opendir("base");
	int mode[4];
	int i;
	int cont = 0;
	for(i=0;i<4;i++)
	{
		mode[i] = 0;
	}
	for(i=2;i<argc;i++)
	{
		if(!strcmp(argv[i],"-c")){ mode[0] = 1; }
		else if(!strcmp(argv[i],"-cm")){ mode[1] = 1; }
		else if(!strcmp(argv[i],"-ch")){ mode[2] = 1; }
		else if(!strcmp(argv[i],"-m")){ mode[3] = 1; }
	}
	while ((dir = readdir(d_bdd)) != NULL) {
		//printf("%s\n\n",dir->d_name);
		cont++;
		if(cont%1000 == 0){printf("%d\n",cont);}
		if(strcmp(dir->d_name,".") && strcmp(dir->d_name,".."))
		{
			sprintf(path_mol, "base/%s/%sMode%d%d%d%d", dir->d_name, dir->d_name,mode[0],mode[1],mode[2],mode[3]);  
			if(cannonisation(path_mol, FALSE)){
				printf("Erreur pour la canonisation de %s \n",path_mol);
			}
		}
	}
	closedir(d_bdd);
}

int main(int argc, char const *argv[]) {
	
	if (argc < 2) {
		printf("Il manque des arguments \n");
		printf("Doit être sous la forme ./canonisation idChebi ou ./canonisation -all mode \n avec mode = -c -cm -ch -m \n");		
		return 1;
	}

	if(!strcmp(argv[1],"-all"))
	{
		if(argc > 5){
			printf("Il y a trop d'arguments\n");
			printf("Doit être sous la forme ./canonisation idChebi ou ./canonisation -all mode \n avec mode = -c -cm -ch -m \n");
			return 1;
		}			
		canonAll(argc,argv);
		return 0;
	}
	else
	{
		if (argc > 2)
		{
			printf("Il y a trop d'arguments\n");
			printf("Doit être sous la forme ./canonisation idChebi ou ./canonisation -all mode \n avec mode = -c -cm -ch -m \n");
			return 1;
		}
		if(cannonisation(argv[1], TRUE)){
			printf("Erreur dans la cannonisation\n");
			return 1;
		}
		else{
			return 0;
		}
	}
}
