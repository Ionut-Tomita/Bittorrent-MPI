# 🌐 Protocolul BitTorrent - Simulare cu MPI

![C++](https://img.shields.io/badge/language-C%2B%2B-blue.svg)
![MPI](https://img.shields.io/badge/library-MPI-orange.svg)
![Parallelism](https://img.shields.io/badge/type-Distributed%20Simulation-green.svg)
![Status](https://img.shields.io/badge/status-Completed-success.svg)

---

## 🧠 Descrierea proiectului

Acest proiect implementează o **simulare a protocolului BitTorrent** 🧩 utilizând **MPI (Message Passing Interface)**.  
Protocolul **BitTorrent** este folosit pentru partajarea *peer-to-peer* de fișiere, permițând utilizatorilor să **încarce și să descarce simultan segmente** ale unui fișier în mod descentralizat, având un **tracker central** care gestionează metadatele și coordonează transferurile.

🎯 Scopul proiectului este să redea mecanismele de bază ale protocolului BitTorrent — comunicarea între *peers*, schimbul de segmente, sincronizarea și anunțurile tracker-ului — într-un cadru distribuit controlat prin **MPI**.

---

## 🧩 Structura proiectului

Proiectul este împărțit în două componente principale: **Tracker-ul** și **Peer-ii**, fiecare cu roluri specifice.  
De asemenea, fiecare peer conține două fire de execuție: unul pentru **download** și unul pentru **upload**.

---

### 🧭 Tracker

🧩 **Rol:**  
Administrează **metadatele fișierelor** și menține o evidență a utilizatorilor (*peers*) care dețin anumite segmente ale fișierelor.

⚙️ **Funcționalități:**
- Primește lista de fișiere deținute de la fiecare peer.  
- Răspunde solicitărilor peer-urilor privind lista utilizatorilor care dețin segmentele dorite.  
- Trimite un **semnal de început** către peers pentru a iniția descărcările.  
- Primește informații despre segmentele descărcate și actualizează starea **swarm-urilor**.  
- Transmite **semnale de finalizare** pentru a închide firele de upload.  

---

### 💻 Peer

🧩 **Rol:**  
Participă la descărcarea și partajarea fișierelor în rețea.

⚙️ **Funcționalități:**
- Citește fișierele deținute și dorite dintr-un fișier de configurare.  
- Trimite tracker-ului informații despre fișierele proprii.  
- Se conectează cu alți peers pentru a solicita și descărca segmente.  
- Servește cererile de segmente de la alți peers.  

---

### ⬇️ Thread-ul de Download

- Solicită și primește **segmentele necesare** de la alți peers.  
- Verifică **integritatea segmentelor** utilizând hash-uri.  
- Marchează fișierele / segmentele complete ca fiind disponibile pentru partajare.  
- Pentru eficiență, clienții variază cât mai mult posibil nodurile de la care descarcă segmentele — aceasta se face prin metoda **Round Robin** 🔄.

---

### ⬆️ Thread-ul de Upload

- Gestionează **cererile de segmente primite** de la alți peers.  
- Simulează **transmiterea segmentelor disponibile**.  
- Colaborează cu thread-ul de download pentru a actualiza starea locală a fișierelor.  

---

## 🚀 Rulare

Pentru rulare, proiectul trebuie compilat într-un executabil numit `tema2`.  
Acesta poate fi lansat utilizând comanda:

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


## 📊 Rezumat conceptual

- 🧱 **Tracker-ul** menține metadatele și coordonează swarm-urile.  
- 🔗 **Peers** descarcă și partajează segmente între ei.  
- 🧵 **Firele de upload/download** rulează concurent și comunică prin mesaje MPI.  
- ⚙️ **Round Robin** asigură echilibrarea cererilor de descărcare.  
- 🕹️ **Sincronizarea** se bazează exclusiv pe mesaje MPI, fără shared memory.  

---

## 🏁 Concluzie

Acest proiect demonstrează o **simulare complet funcțională a protocolului BitTorrent** 🧠  
într-un mediu distribuit **MPI**, ilustrând cooperarea între tracker și peers,  
transferul concurent al datelor și actualizarea dinamică a swarm-urilor.  

📡 Este un exemplu excelent de aplicare practică a **comunicării inter-proces**  
și a principiilor de **distribuire a sarcinii** în sisteme paralele.



