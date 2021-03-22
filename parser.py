# coding=<UTF-8>
from requests import get
from requests.exceptions import HTTPError
import os.path
import os
import sys
import gzip

#Recuperation du fichier chebi pour effectuer le traitement
def fetch_desc(chebi_id, debug = False):
    url = 'https://www.ebi.ac.uk/chebi/saveStructure.do?sdf=true&chebiId=' + chebi_id + '&imageId=0'
    print(url)
    
    mol_data = None
    try:
        response = get(url)
        mol_data = response.text.splitlines()
    except HTTPError as http_err:
        print(f'HTTP error occurred: {http_err}')
    except Exception as err:
        print(f'Other error occurred: {err}')
    finally:
        if debug:
            print(mol_data)
        return mol_data      

#Parsing du fichier chebi
# mdata donnée récupérer dans les fichier
# On return mol_desc qui est un dictionnaire de la molécule
def parse_desc(chebi_id, mdata, debug=False):
    mol_desc = { '_id': chebi_id }
    atab = [] #Liste des atomes présents
    ltab = [] #Liste des liaisons
    ctab = [] #Liste des charges

    count = 0 #Compteur du nombre de lignes présentes 

    # Automate d'état de l'analyseur
    astate = 1 # Init
    for line in mdata[3:]:
        line.strip()
        if debug:
            print(str(count)+"/"+str(astate) + ": " + line)
        count += 1

        # On saute les lignes vides
        if len(line) == 0 or line[0:5].upper() == "NULL" or (len(line) == 1 and line[0:1] == " "):
            continue
        #Etat pour les caractèristiques de la molécule
        if astate == 1:
            items = line.split()
            astate = 2
            atomes = int(items[0])
            if atomes == 0:
                mol_desc.update({'atomes': 0, 'liaisons': 0})
                return None
            elif atomes > 999:
                liaisons = atomes % 1000
                atomes = atomes // 1000
            else:
                liaisons = int(items[1])
            if debug:
                print(atomes)
            mol_desc.update({'atomes': atomes, 'liaisons': liaisons})
        #Etat pour la liste des atomes de la molécule
        elif astate == 2:
            items = line.split()
            atab.append(items[3])
            atomes -= 1
            if atomes == 0:
                mol_desc.update({"atab": atab})
                if liaisons == 0:
                    mol_desc.update({'ltab': None})
                    astate = 4
                else: 
                    astate = 3
        #Etat pour la liste des liaisons de la molécule
        elif astate == 3:
            items = line.split()
            if int(items [0]) > 999:
                items[1] = int(items[0]) % 1000
                items[0] = int(items[0]) // 1000
            ltab.append({'a1': int(items[0]), 'a2': int(items[1]), 'type': int(items[2])})
            liaisons -= 1
            if liaisons == 0:
                mol_desc.update({'ltab': ltab})
                astate = 4
        #Etat pour la liste des charges de la molécule
        elif astate == 4:
            items = line.split()
            if(not len(items) == 1): 
                if items[1] == 'END':
                    mol_desc.update({'ctab': ctab})
                    astate = 5
                elif items[1] == 'CHG':
                    nb_chg = int(items[2])
                    ic = 1
                    while nb_chg > 0:
                        nb_chg -= 1
                        ic += 2
                        ctab.append({'atome': int(items[ic]), 'charge': int(items[ic+1])})
        #Etat pour le nom de la molécule
        elif astate == 5:
            if("<NAME>" in line or "<ChEBI Name>" in line):
                astate = 6
                # print(line)
            elif("<ID>" in line or "<ChEBI ID>" in line):
                astate = 7

        #Etat pour le nom de la molécule
        elif astate == 6:
            mol_desc["name"] = line
            astate = 5
            
        elif astate == 7:
            tmp = line.replace("CHEBI:","")
            mol_desc["_id"] = tmp
            astate = 5
        #Reste du fichier
        else:
            print("???", count, line)

    return mol_desc

def parse_molecule(chebi_id):
    mdata = fetch_desc(chebi_id)
    if mdata is None:
        return None
    else:
        mol_desc = parse_desc(chebi_id, mdata)
        return mol_desc

# mdesc dictionnaire de notre molécule
# On retourne le nouveau dictionnaire sans les H
def delete_hydro(mdesc, debug=False):
    if debug:
        print(mdesc)
    if mdesc != None :
        alen = len(mdesc['atab']) #Nombre d'atomes
        atab = []
        ltab = []
        ctab = []
        hcnt = [] #Nombre d'hydrogènes rencontré lors du parcours de la liste d'atome permet de faire la soustraction
                  #pour les atomes qui suivent 
        hc = 0 #Hydrogenes presents

        for i in range(alen):
            atome = mdesc['atab'][i]
            if atome == 'H':
                hc += 1
            else:
                atab.append(atome)
            hcnt.append(hc)

        if debug:
            print(mdesc['atab'])
            print(atab)
            print(hcnt)

        #Gestion des liaisons avec hydrogènes en moins
        if mdesc['ltab'] != None:
            for liaison in mdesc['ltab']:
                if debug:
                    print(liaison)
                a1 = liaison['a1']
                a2 = liaison['a2']
                #on soustrait avec la liste hcnt pour savoir de combien on doit décaler
                #on enleve les liaisons avec les atomes
                if mdesc['atab'][a1-1] != 'H' and mdesc['atab'][a2-1] != 'H':
                    a1 -= hcnt[a1-1]
                    a2 -= hcnt[a2-1]
                    ltab.append({'a1': a1, 'a2': a2, 'type': liaison['type']})

        #Gestion des charges si elles se trouvent sur un hydrogène
        for charge in mdesc['ctab']:
            if debug:
                print(charge)
            ac = charge['atome']
            #on soustrait avec la liste hcnt pour savoir de combien on doit décaler
            #on enleve les charges des hydrogèness
            if mdesc['atab'][ac-1] != 'H':
                ac -= hcnt[ac-1]
                ctab.append({'atome': ac, 'charge': charge['charge']})

        #mise à jour du dict de la molécule
        mdesc.update({'atomes': len(atab), 'liaisons': len(ltab), 'atab': atab, 'ltab': ltab, 'ctab': ctab})
        return mdesc
    return mdesc

#Transformation en notre fichier
# mdesc dictionnaire de notre molécule dans lequel on récupère nos informations
def export_txt(mdesc, all=False, debug=False):
    if mdesc == None:
        print("Fichier non valide")
        sys.exit(2) #Problème fichier
    else:    
        cid = mdesc['_id']
        if mdesc['atomes'] == 0 :
            if(all):
                os.rmdir("base/"+str(cid))
            else:
                sys.exit(2) #Problème fichier
        else :    
            f = open("base/"+str(cid)+"/" + str(cid) + '.txt', 'w')
            try:
                f.write(str(cid) + '\n')

                f.write(mdesc['name'] + '\n')
                f.write(str(mdesc['atomes']) + ' ' + str(mdesc['liaisons']) + ' '+ str(len(mdesc['ctab'])) + '\n')
                for a in mdesc['atab']:
                    f.write(a + ' ')
                f.write('\n')
                for l in mdesc['ltab']:
                    f.write(str(l['a1']-1) + ' ' + str(l['a2']-1) + ' ' + str(l['type']) + '\n')
                for c in mdesc['ctab']:
                    f.write(str(c['atome']-1) + ' ' + str(c['charge']) + '\n')    
                f.close()
            except:
                print("Fichier n'a pas pu etre cree "+"base/"+str(cid)+"/" + str(cid) + '.txt')
                sys.exit(2) #Probleme fichier
                
#fonction qui parse la base complète de chebi 
def parseAll():
    if (not os.path.exists("base_chebi.gz")):   #télécharge la base si elle n'existe pas en local
        ty = get("http://ftp.ebi.ac.uk/pub/databases/chebi/SDF/ChEBI_lite.sdf.gz")
        with open("base_chebi.gz", "wb") as fp:
            fp.write(ty.content)
    with gzip.open("base_chebi.gz", 'rt') as fi:    #parse de la base complète
        temp = []
        ctmolec = 0
        contERR = 0
        for line in fi:
            if("$$$$" in line):
                ctmolec = ctmolec + 1
                if(ctmolec % 1000 == 0): print(ctmolec)
                res = parse_desc(ctmolec,temp)
                if (not res == None) and (not os.path.exists("base/"+str(res["_id"]))):
                    os.makedirs("base/"+str(res["_id"]))
                    tst = export_txt(delete_hydro(res),True)
                temp = []
            else:
                temp.append(line.replace("\n",""))
    
#Main avec arguments
#py parser.py id1
if __name__ == "__main__":
    if(sys.argv[1] == "-all"):
        parseAll()
    else:
        try:
            export_txt(delete_hydro(parse_molecule(sys.argv[1])))
        except:
            print("Verifier vos arguments ou vos identifiants Chebi")
            sys.exit(1) #Arguments mal écrits 


