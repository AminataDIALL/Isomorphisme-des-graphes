compil :
	make canonisation
	make comparaison

#Compilation
canonisation : canonisation.c
	gcc -g canonisation.c -o canonisation nauty27r1/nauty.a

comparaison : comparaison.c
	gcc -g comparaison.c -o comparaison nauty27r1/nauty.a

#Clean
clean :
	rm -f canonisation
	rm -f comparaison
