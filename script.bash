#!/bin/bash
var="base/"
nop=0
argMode=""
c=0
cm=0
ch=0
m=0
all=0

for arg in $@
	do
		if [[ $nop -lt 2 ]]
		then
			nop=$(($nop+1))
		else
			argMode=${argMode}" "$arg
			if [[ $arg = "-c" ]]
			then
				c=1
			elif [[ $arg = "-cm" ]]
			then
				cm=1
				c=0
			elif [[ $arg = "-ch" ]]
			then
				ch=1
			elif [[ $arg = "-m" ]]
			then
				m=1
			else
				echo $arg"n'est pas un argument valide"
				exit
			fi
		fi
	done
	if [[ $c -eq 0 ]] && [[ $cm -eq 0 ]] && [[ $ch -eq 1 ]]
	then
		echo "les charges sont inutiles sans colorations : le mode charge est désactivé"
		echo "----------------------------------"
		ch=0
	fi
	
	
if [[ $1 = "-clean" ]]
then
	if [[ -e base_chebi.gz ]]
	then
		rm base_chebi.gz
	fi
	if [[ -e base ]]
	then
		rm -fr base
	fi
	make clean
	exit
fi	
if  [[ $1 = "-all" ]]
then
	if [[ $2 = "-parse" ]]
	then
		python3 parser.py -all
		echo $?
	elif [[ $2 = "-mod" ]]
	then
		python3 mode.py -All $argMode
	elif [[ $2 = "-canon" ]]
	then
		make
		./canonisation -all $argMode
	fi
else
	
	if ! [[ $1 =~ ^[0-9]+$ ]]
	then
		echo "probleme d'argument"
		exit
	fi

	if [[ $2 = "-all" ]]
	then
		echo "all"
		all=1
	else
		if ! [[ $2 =~ ^[0-9]+$ ]]
		then
			echo "probleme d'argument"
		exit
		fi
	fi
	make
	if ! [[ -d ${var} ]]
	then
		mkdir ${var}
		echo "creation de la base"
	fi


	if ! [[ -d ${var}${1} ]]
	then
		mkdir ${var}${1}
		echo "creation du dossier ${var}${1}"
	fi
	if [[ $all -eq 0 ]]
	then
		if ! [[ -d ${var}${2} ]]
		then
			mkdir ${var}${2}
			echo "creation du dossier ${var}${2}"
		fi
	fi
	#########################################Parser

	if [[ -e ${var}${1}"/"${1}".txt" ]]
	then
		echo "la molécule ${1} a déja été parsée"
	else
		echo "lancement du parser sur ${1}"
		python3 parser.py ${1}
		if ! [[ $? == 0 ]]
		then
			rm ${var}${1}
			exit
		fi
	fi

	if [[ $all -eq 0 ]]
	then
		if [[ -e ${var}${2}"/"${2}".txt" ]]
		then
			echo "la molécule ${2} a déja été parsée"
		else
			echo "lancement du parser sur ${2}"
			python3 parser.py ${2}
			if ! [[ $? == 0 ]]
			then
				rm ${var}${2}
				exit
			fi
		fi
	fi
	##########################################Mode

	if [[ -e ${var}${1}"/"${1}"Mode"${c}${cm}${ch}${m} ]]
	then
		echo "le mode a déja été appliqué"
	else
		echo "lancement des modes"
		python3 mode.py ${1} ${argMode}
		if ! [[ $? == 0 ]]
		then
			echo erreur lors de l application des modes sur le fichier ${var}${1}"/"${1}".txt"
			exit
		fi
	fi

	if [[ $all -eq 0 ]]
	then
		if [ -e ${var}${2}"/"${2}"Mode"${c}${cm}${ch}${m} ]
		then
			echo "le mode a déja été appliqué"
		else
			echo "lancement des modes"
			python3 mode.py ${2} ${argMode}
			if ! [[ $? == 0 ]]
			then
				echo erreur lors de l application des modes sur le fichier ${var}${2}"/"${2}".txt"
				exit
			fi
		fi
	fi
	########################################Canonisation
	echo ${var}${1}"/"${1}"Mode"${c}${cm}${ch}${m}".canon"

	if [[ -e ${var}${1}"/"${1}"Mode"${c}${cm}${ch}${m}".canon" ]]
	then
		echo "la molécule avec les modes ${c}${cm}${ch}${m} ont déja été canonisée"
	else
		echo "lancement de la canonisation"
		./canonisation ${var}${1}"/"${1}"Mode"${c}${cm}${ch}${m}
		if ! [[ $? == 0 ]]
			then
				exit
			fi
	fi

	if [[ $all -eq 0 ]]
	then
		if [ -e ${var}${2}"/"${2}"Mode"${c}${cm}${ch}${m}".canon" ]
		then
			echo "la molécule avec les modes ${c}${cm}${ch}${m} ont déja été canonisée"
		else
			echo "lancement de la canonisation"
			./canonisation ${var}${2}"/"${2}"Mode"${c}${cm}${ch}${m}
			if ! [[ $? == 0 ]]
			then
				exit
			fi
		fi
	fi

	############################################comparaison
	if [[ $all -eq 0 ]]
	then
		./comparaison -of ${var}${1}"/"${1}"Mode"${c}${cm}${ch}${m}".canon" ${var}${2}"/"${2}"Mode"${c}${cm}${ch}${m}".canon"
	else
		./comparaison -af  ${var}${1}"/"${1}"Mode"${c}${cm}${ch}${m}".canon" "base"
	fi
fi
