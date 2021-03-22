import sys
import numpy as np
import os

Load = False        
cota = {}   #sauvegarde de la table

class molecule:
    #contructeur de la classe molécule
    def __init__(self,nSommet,nArete,listType,listArete,listCharge):
        self.nbSommet = nSommet #entier : nombre de sommet
        self.nbArete = nArete #entier : nombre d'arête
        self.listType = listType #liste de string : couleur des atomes
        self.listArrete = listArete #liste de triplet (sommet, sommet, poid) (dans les deux sens)
        self.listCharge = listCharge #liste des charges
        self.listCouleur = [] #liste de coloration
    
    def __str__(self):
        return "nbSommet:"+str(self.nbSommet)+"\nnbArete:"+str(self.nbArete)+"\nlistColoration:"+str(self.listType)+"\nlistArrete:"+str(self.listArrete)+"\nlistCharge:"+str(self.listCharge)
    
    #fonction qui charge la table de coloration simple
    def loadSimpleColoration(self):
        colorTable = {}
        cont = 0
        #maintient en mémoire de la table de coloration pour le cas ou d'autre molecules doivent être colorées
        global Load
        global cota
        if(Load == True):
            return cota,cont
        else:   #chargement de la table
            with open("colorationSimple.txt","r") as fi:    #lecture du fichier de coloration
                for line in fi:
                    type = line.replace("\n","")
                    colorTable[type] = cont
                    cont = cont + 1
                cota = colorTable
                Load = True
                return colorTable,cont      #ne retournera plus le conteur quant la table de coloration serra complète
    
    #fonction qui charge la table de coloration par liaisons
    def loadMultipleColoration(self):
        colorTable = {}
        cont = 0
        #maintient en mémoire de la table de coloration pour le cas ou d'autre molecules doivent être colorées
        global Load
        global cota
        if(Load == True):
            return cota
        else:   #chargement de la table
            with open("colorationLiaison.txt","r") as fi:    #lecture du fichier de coloration multiple
                for line in fi:
                    type = line.replace("\n","")
                    type = type.split(" ")
                    for tp in type:
                        colorTable[tp] = cont
                    cont = cont + 1
                cota = colorTable
                Load = True
                
        return colorTable
    
    #fonction de coloration simple, si la couleur n'existe pas, elle est ajoutée au fichier coloration simple
    #multiple: si True coloration par liaisons 
    def coloration(self,multiple = False):
        if(multiple):
            colorTable = self.loadMultipleColoration()
        else:
            colorTable,cont = self.loadSimpleColoration()

        #affectation des couleurs
        for type in self.listType:
            if(type in colorTable):
                self.listCouleur.append(colorTable[type])
            else:
                print("Le type " + type + " n'existe pas")
                sys.exit(2)
    
    
    #fonction qui applique les charges sur les types
    def charge(self):
        for chg in self.listCharge:
            if(int(chg[1])>0):
                self.listType[int(chg[0])] = self.listType[int(chg[0])] + chg[1] + "+"
            else:
                self.listType[int(chg[0])] = self.listType[int(chg[0])] + chg[1][1] + chg[1][0]
                
    #coloration des doubles et triples liaisons 
    def colorMultiple(self):
        if(not self.listCouleur): #cas ou la molécule n'est pas colorée
            for i in range(0,self.nbSommet):
                self.listCouleur.append(1)
            for i in range(0,self.nbSommet):
                self.listCouleur.append(1000)
        else:   #cas ou la molecule est colorée
            for i in range(0,self.nbSommet):
                self.listCouleur.append(self.listCouleur[i]+1000)
    
    #fonction qui permet d'utiliser les doubles et triples liaisons 
    def LiaisonsMultiple(self):
        nArr = []
        #création des aretes entre les paire de sommets
        for i in range (0,self.nbSommet):
            nArr.append([i,i+self.nbSommet])
            nArr.append([i+self.nbSommet,i])
        for ar in self.listArrete:
            if int(ar[2]) == 2: #cas d'une double liaison (arete en couche 2)
                temp = []
                temp.append(int(ar[0]) + self.nbSommet)
                temp.append(int(ar[1]) + self.nbSommet)
                nArr.append(temp)
            elif int(ar[2]) == 3:   #cas d'une triple liaison (arete en couche 1 et 2)
                temp = []
                temp.append(int(ar[0]) + self.nbSommet)
                temp.append(int(ar[1]) + self.nbSommet)
                nArr.append(temp)
                nArr.append(ar[:2])
            else:   #tout les autres cas sont traité comme une liason simple (arete en couche 1)
                nArr.append(ar[:2])
        self.colorMultiple()
        self.listArrete = nArr
        self.nbSommet = self.nbSommet*2
        
        
    #fonction applique les différent mode sellectionés 
    #modes : tableau de booléen correpondant aux modes   
    def applyMode(self,modes):
        if(modes[2]):
            if(modes[0] or modes[1]):
                self.charge()
            else:
                print("L'application des charges sans coloration n'est pas utile, basculement du mode charge sur False")
                modes[2] = False
        if(modes[1]):           #coloration multiple
            self.coloration(True)
            modes[0] = False
        elif(modes[0]):         #coloration simple
            self.coloration()
        if(modes[3]):           #liaisons multiples
            self.LiaisonsMultiple()
        
        
#fonction qui lie le fichier de données (obtention du graphe brut)
#f : nom du fichier
def readInputFile(f):
    try:
        with open(f,"r") as fi:
            fi.readline()
            fi.readline()
            ligne = fi.readline().split()
            nSommet = int(ligne[0])
            nbArete = int(ligne[1])
            nbCharge = int(ligne[2])
            listColoration = fi.readline().split()
            listArete = []
            for i in range(0,nbArete):  #lecture des aretes
                arc = fi.readline().split()
                arcInt= []
                for i in arc:   
                    arcInt.append(int(i))
                listArete.append(arcInt)
                arc2 = [arcInt[1],arcInt[0],arcInt[2]]
                listArete.append(arc2)
            listArete.sort()
            listCharge = []
            for i in range(0,nbCharge):     #lecture des charges
                listCharge.append(fi.readline().split())
    except:
        print("Le fichier " + f +" n'existe pas")
        sys.exit(1) #retour de l'erreur lecture de fichier
    return molecule(nSommet,nbArete,listColoration,listArete,listCharge)

#fonction qui récupère les argument dans la ligne de commande
#renvoie la liste des mode (tableau de booléen)
#mode -c -cm -ch -m    
def getArg():
    md = sys.argv[2:]
    modTab = [False,False,False,False]
    for i in sys.argv[2:]:
        if(i == "-c"): modTab[0] = True
        elif(i == "-cm"): modTab[1] = True
        elif(i == "-ch"): modTab[2] = True
        elif(i == "-m"): modTab[3] = True
        else: print("L'option "+i+" n'existe pas")
    return sys.argv[1],modTab 

#fonction qui retourne la liste des degrès des sommets (d)
#listArete: la liste des aretes du graphe
#nSommet: un entier corespondant au nombre de sommet du graph
def listeDegres(listArete,nSommet):
    voisin = np.zeros(nSommet)
    for arr in listArete:
        voisin[int(arr[0])] = voisin[int(arr[0])] + 1
    return voisin

#fonction qui renvoi le tableau qui contient la liste des voisins des sommets (e)
#listArete: la liste des aretes du graphe 
def listeVoisins(listArete):
    listVoisin = []
    listArete.sort()
    for arr in listArete:
        listVoisin.append(arr[1])
    return listVoisin

#fonction qui renvoi le tableau qui coresponde au délimitateur de voisins (v)
#degres: tableau des degres des sommets du graph    
def listeDelimVoisins(degres):
    listDelim = []
    val = -1    #set à -1 pour que les indices corespondent 
    for de in degres:
        if(val == -1): val = 0
        listDelim.append(val)
        val = val + de
    return listDelim

#fonction temporaire pour nommer les fichiers
#modTab le tableau d'option
#f: l'id de la molécule
def genNameFile(modTab,f):
    temp = ""
    for mod in modTab:
        if(mod): temp = temp + "1"
        else: temp = temp + "0"
    f = "base/"+f+"/"+f+"Mode"+temp
    return f

#fonction qui renvoi le tableau de permutation des sommets en fonction de la coloration, renvoi également le tableau ptn
#listCouleur: liste de coloration des sommets
def liste_permutation(listCouleur):
    couleurs = np.unique(listCouleur)
    couleurs.sort()
    permut = []
    ptn = []
    for coul in couleurs: #pour chaque couleur 
        cont = 0
        sizecoul = 0
        for lc in listCouleur:  #on parcours les sommets
            if(lc == coul):
                permut.append(cont)
                sizecoul = sizecoul + 1
            cont = cont + 1
        for i in range(0,sizecoul-1):   #remplissage du tableau ptn
            ptn.append(1)
        ptn.append(0); #marqueur de fin de la couleur dans ptn
    return(permut,ptn)           

#fonction qui écrit le fichier de sortie
#f: nom du fichier de retour
#molec: molecule dont on veut écrire les informations dans le fichier
def writeOutputFile(f,molec):
    d = listeDegres(molec.listArrete,molec.nbSommet)
    e = listeVoisins(molec.listArrete)
    v = listeDelimVoisins(d)
    if(not molec.listCouleur):
        molec.listCouleur = np.zeros(molec.nbSommet)
    lab,ptn = liste_permutation(molec.listCouleur)
    coul = molec.listCouleur;
    coul.sort()
    #écriture du fichier    
    with open(f,"wb") as fi:
        fi.write(molec.nbSommet.to_bytes(4,byteorder='little'))         #écriture du nombre de sommets
        fi.write(len(molec.listArrete).to_bytes(8,byteorder='little'))  #écriture du nombres d'aretes
        fi.write(len(d).to_bytes(8,byteorder='little'))                 #écriture de Dlen
        fi.write(len(v).to_bytes(8,byteorder='little'))                 #écriture de Vlen
        fi.write(len(e).to_bytes(8,byteorder='little'))                 #écriture de Elen
        for val in d:                                                   #écriture du tableau de degres
            fi.write(int(val).to_bytes(4,byteorder='little'))
        for val in v:                                                   #écriture du tableau délimitateur de voisins
            fi.write(int(val).to_bytes(8,byteorder='little'))
        for val in e:                                                   #écriture du tableau de voisins
            fi.write(int(val).to_bytes(4,byteorder='little'))
        for val in lab:                                                 #écriture du tableau lab
            fi.write(int(val).to_bytes(4,byteorder='little'))
        for val in ptn:                                                 #écriture du tableau ptn
            fi.write(int(val).to_bytes(4,byteorder='little'))
        for val in coul:                                                #écriture du tableau de couleur
            fi.write(int(val).to_bytes(4,byteorder='little'))   

#fonction qui mode l'ensemble des fichiers parsés
def baseComplete():
    base = []
    f,mode = getArg()
    cont = 0
    doss = os.listdir("base")
    for line in doss:   
        cont = cont + 1
        if(cont%1000 == 0):
            print(cont)
        id = line.replace("\n","")#récupération de l'id
        try:
            nf = genNameFile(mode,id)
            if (not os.path.exists(nf)):    #si le fichier modeXXXX pour cette molécule n'existe pas on applique les modes
                molec = readInputFile("base/"+ id + "/" + id + ".txt")
                molec.applyMode(mode)
                writeOutputFile(nf,molec)
        except:
            print("Erreur lors de l'application des modes")
            if(os.path.exists(nf)):
                os.remove(nf) #si ereur lors de l'application des mode et fichier existant : supression du fichier  
    print("\n")
    sys.exit(0)

#main du sous programme Mode
def main():
    f,mode = getArg()
    if(f == "-All"):    #application des modes sur l'ensemble des fichiers parsé
        baseComplete()
    else:               #application des modes sur la molécule passé en argument
        molec = readInputFile("base/"+ f + "/" + f + ".txt")
        molec.applyMode(mode)
        writeOutputFile(genNameFile(mode,f),molec)
    sys.exit(0)    

main()
