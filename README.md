# EduChain

EduChain este o aplicație educațională client-server scrisă în C++17. Interfața
desktop folosește Qt Widgets, serverul aplică regulile de business și de acces,
iar datele persistente sunt stocate în SQLite.

Arhitectura principală este:

```text
Qt / ClientEdu -> ServerEdu -> Service Layer -> Repository Layer -> SQLite
```

## Cerințe

- Windows x64;
- Visual Studio 2026 cu toolset MSVC `v143` (toolset-ul Visual Studio 2022);
- CMake 3.20 sau mai nou;
- Qt 6.11.1, kitul `msvc2022_64`;
- sursele SQLite incluse în `external/`.

Nu sunt necesare instalări SQLite sau alte dependențe externe suplimentare.

## Configurare și build

Backend și teste:

```powershell
cmake -S . -B build -G "Visual Studio 18 2026" -A x64 -T v143
cmake --build build --config Debug
```

Client Qt:

```powershell
cmake -S . -B build-qt -G "Visual Studio 18 2026" -A x64 -T v143 `
  -DEDUCHAIN_BUILD_QT=ON `
  -DCMAKE_PREFIX_PATH="C:/Qt/6.11.1/msvc2022_64"
cmake --build build-qt --config Debug --target EduChainQtClient
```

Dacă versiunea locală de CMake nu oferă încă generatorul Visual Studio 18,
folosiți generatorul `Visual Studio 17 2022` cu același `-T v143`. Acesta
produce aceleași proiecte pentru toolset-ul folosit de EduChain.

## Pornire

Serverul primește opțional portul și calea bazei de date:

```powershell
.\build\Debug\EduChainServer.exe 5050 .\educhain.db
```

Clientul Qt primește adresa și portul serverului:

```powershell
.\build-qt\Debug\EduChainQtClient.exe 127.0.0.1 5050
```

Pentru teste manuale se recomandă întotdeauna o bază temporară, nu
`educhain.db`.

## Teste automate

```powershell
ctest --test-dir build -C Debug --output-on-failure
```

Testele acoperă persistența, serviciile, protocolul, comunicația, înscrierile,
lecțiile, evaluările, încercările și fluxul end-to-end.

## Roluri și funcționalități

### Student

- creare cont și autentificare;
- listarea cursurilor disponibile și înscrierea la curs;
- listarea cursurilor înscrise, lecțiilor și detaliilor acestora;
- listarea evaluărilor accesibile;
- pornirea unei încercări, completarea răspunsurilor și finalizarea evaluării;
- afișarea scorului și notei calculate de server;
- refresh, logout și tratarea conexiunii pierdute.

### Profesor

- creare cont și autentificare;
- listarea cursurilor proprii în fluxurile de lecții și evaluări;
- adăugarea lecțiilor text și video;
- crearea chestionarelor și examenelor finale;
- listarea și adăugarea întrebărilor pentru chestionare;
- înscrierea studenților la cursurile proprii;
- refresh, logout și tratarea conexiunii pierdute.

Serverul validează rolurile, proprietatea cursurilor și accesul studenților.
Interfața nu accesează direct SQLite, iar identificatorii sunt transportați
separat de textele afișate.

## Limitări cunoscute

- API-ul nu oferă istoric pentru rezultatele evaluărilor după refresh;
- răspunsul corect nu este returnat de API-ul public la listarea întrebărilor;
- întrebările sunt permise numai chestionarelor;
- punctajul maxim este derivat din suma punctajelor întrebărilor;
- paginile Qt dedicate creării și administrării generale a cursurilor
  Profesorului sunt încă afișate ca funcționalități viitoare; API-ul backend
  pentru cursuri există și este acoperit de testele end-to-end;
- parolele sunt păstrate momentan în forma compatibilă cu schema existentă;
  pentru producție este necesară migrarea la hashing robust al parolelor.
