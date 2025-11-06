# Protocolul BitTorrent - Simulare cu MPI

## Descrierea proiectului
Acest proiect implementează o simulare a protocolului BitTorrent utilizând MPI (Message Passing Interface). Protocolul BitTorrent este folosit pentru partajarea peer-to-peer de fișiere, permițând utilizatorilor să încarce și să descarce simultan segmente ale unui fișier în mod descentralizat cu un tracker central care gestionează metadatele și coordonează transferurile de fișiere.

## Structura proiectului
Proiectul conține următoarele componente principale:

- Tracker:

    Rol: Administrează metadatele fișierelor și menține o evidență a utilizatorilor (peers) care dețin anumite segmente ale fișierelor.

    Funcții: Primește lista de fișiere deținute de la fiecare peer. Răspunde solicitărilor peer-urilor privind lista utilizatorilor care dețin segmentele dorite ale fișierelor.Trimite un semnal de început către peers pentru a începe descărcările. Primeste informații despre segmentele descărcate și actualizează informațiile despre swarm-uri. Transmite semnale de finalizare pentru a inchide firele de upload.

- Peer:

    Rol: Participă la descărcarea și partajarea fișierelor.

    Funcționalități:
    Citește fișierele deținute și dorite dintr-un fișier de configurare. Transmite tracker-ului informații despre fișierele pe care le deține. Se conectează cu alte peers pentru a solicita și descărca segmentele dorite. Servește cererile de segmente de la alți peers.


- Thread-ul de download:
    Solicită și primește segmentele necesare de la alți peers.
    Verifică integritatea segmentelor utilizând hash-uri.
    Marchează fișierele/segmentele complete și disponibile pentru partajare.

    Pentru eficiență, clienții variază cât mai mult posibil nodurile de la care descarcă segmentele fișierelor, aceasta se face prin metoda `Round Robin.`

- Thread-ul de upload:
    Gestionează cererile de segmente primite de la alți peers.
    Simuleaza transmiterea segmentele disponibile.

## Rulare
Pentru rulare, proiectul trebuie compilat într-un executabil numit `tema2`. Acesta poate fi lansat folosind comanda:

```bash
mpirun -np <N> ./tema2
```

Unde `N` reprezintă numărul de task-uri MPI (≥ 3). 
- Task-ul `0` va juca rolul trackerului.
- Task-urile `1, 2, ... N-1` vor fi clienți.

### Exemplu de rulare:

```bash
mpirun -np 4 ./tema2
```

Acest exemplu pornește un tracker și trei clienți.


## Testare automată

Pentru a rula testele automate, rulați scriptul `run_with_docker.sh`:

```bash

./run_with_docker.sh

```


