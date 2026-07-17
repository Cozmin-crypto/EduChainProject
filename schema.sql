-- EduChain - schema relationala SQLite
-- Se activeaza pentru fiecare conexiune SQLite care utilizeaza aceasta schema.
PRAGMA foreign_keys = ON;

BEGIN TRANSACTION;

-- UtilizatorBaza si specializarile Student, Personal, Profesor, Administrator.
CREATE TABLE utilizatori (
    id                  INTEGER PRIMARY KEY,
    email               TEXT NOT NULL UNIQUE,
    parola              TEXT NOT NULL,
    rol                 TEXT NOT NULL CHECK (rol IN ('student', 'profesor', 'administrator')),
    data_ultima_logare  TEXT,
    creat_la            TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE studenti (
    utilizator_id       INTEGER PRIMARY KEY,
    FOREIGN KEY (utilizator_id) REFERENCES utilizatori(id) ON DELETE CASCADE
);

CREATE TABLE personal (
    utilizator_id       INTEGER PRIMARY KEY,
    departament         TEXT NOT NULL,
    data_angajarii      TEXT NOT NULL,
    FOREIGN KEY (utilizator_id) REFERENCES utilizatori(id) ON DELETE CASCADE
);

CREATE TABLE profesori (
    utilizator_id       INTEGER PRIMARY KEY,
    FOREIGN KEY (utilizator_id) REFERENCES personal(utilizator_id) ON DELETE CASCADE
);

CREATE TABLE administratori (
    utilizator_id       INTEGER PRIMARY KEY,
    nivel_acces         INTEGER NOT NULL CHECK (nivel_acces >= 0),
    FOREIGN KEY (utilizator_id) REFERENCES personal(utilizator_id) ON DELETE CASCADE
);

-- Grupurile mentionate in documentatie (de exemplu Erasmus, Cyber, CTF).
CREATE TABLE grupuri (
    id                  INTEGER PRIMARY KEY,
    nume                TEXT NOT NULL UNIQUE,
    descriere           TEXT
);

CREATE TABLE membri_grup (
    grup_id             INTEGER NOT NULL,
    student_id          INTEGER NOT NULL,
    PRIMARY KEY (grup_id, student_id),
    FOREIGN KEY (grup_id) REFERENCES grupuri(id) ON DELETE CASCADE,
    FOREIGN KEY (student_id) REFERENCES studenti(utilizator_id) ON DELETE CASCADE
);

-- Cursurile sunt directoare ierarhice. parinte_id NULL reprezinta radacina.
CREATE TABLE cursuri (
    id                  INTEGER PRIMARY KEY,
    nume                TEXT NOT NULL,
    parinte_id          INTEGER,
    proprietar_id       INTEGER NOT NULL,
    creat_la            TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
    actualizat_la       TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (parinte_id) REFERENCES cursuri(id) ON DELETE CASCADE,
    FOREIGN KEY (proprietar_id) REFERENCES profesori(utilizator_id) ON DELETE RESTRICT,
    UNIQUE (proprietar_id, parinte_id, nume)
);

CREATE INDEX idx_cursuri_parinte ON cursuri(parinte_id);
CREATE INDEX idx_cursuri_nume ON cursuri(nume);

CREATE TABLE inscrieri_curs (
    curs_id             INTEGER NOT NULL,
    student_id          INTEGER NOT NULL,
    inscris_la          TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (curs_id, student_id),
    FOREIGN KEY (curs_id) REFERENCES cursuri(id) ON DELETE CASCADE,
    FOREIGN KEY (student_id) REFERENCES studenti(utilizator_id) ON DELETE CASCADE
);

-- Lectie este baza pentru LectieText si LectieVideo.
CREATE TABLE lectii (
    id                  INTEGER PRIMARY KEY,
    curs_id             INTEGER NOT NULL,
    proprietar_id       INTEGER NOT NULL,
    nume                TEXT NOT NULL,
    tip                 TEXT NOT NULL CHECK (tip IN ('text', 'video')),
    continut            TEXT NOT NULL DEFAULT '',
    dimensiune_octeti   INTEGER NOT NULL DEFAULT 0 CHECK (dimensiune_octeti >= 0),
    creat_la            TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
    actualizat_la       TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (curs_id) REFERENCES cursuri(id) ON DELETE CASCADE,
    FOREIGN KEY (proprietar_id) REFERENCES profesori(utilizator_id) ON DELETE RESTRICT,
    UNIQUE (curs_id, nume)
);

CREATE INDEX idx_lectii_curs ON lectii(curs_id);
CREATE INDEX idx_lectii_nume ON lectii(nume);

CREATE TABLE lectii_text (
    lectie_id           INTEGER PRIMARY KEY,
    numar_cuvinte       INTEGER NOT NULL DEFAULT 0 CHECK (numar_cuvinte >= 0),
    FOREIGN KEY (lectie_id) REFERENCES lectii(id) ON DELETE CASCADE
);

CREATE TABLE lectii_video (
    lectie_id           INTEGER PRIMARY KEY,
    durata              INTEGER NOT NULL CHECK (durata >= 0),
    codec               TEXT NOT NULL,
    FOREIGN KEY (lectie_id) REFERENCES lectii(id) ON DELETE CASCADE
);

-- Permisiuni pentru cursuri/directoare si pentru fisierele cu lectii.
CREATE TABLE permisiuni_curs_utilizator (
    curs_id             INTEGER NOT NULL,
    utilizator_id       INTEGER NOT NULL,
    drept               TEXT NOT NULL CHECK (drept IN ('citire', 'modificare')),
    PRIMARY KEY (curs_id, utilizator_id),
    FOREIGN KEY (curs_id) REFERENCES cursuri(id) ON DELETE CASCADE,
    FOREIGN KEY (utilizator_id) REFERENCES utilizatori(id) ON DELETE CASCADE
);

CREATE TABLE permisiuni_curs_grup (
    curs_id             INTEGER NOT NULL,
    grup_id             INTEGER NOT NULL,
    drept               TEXT NOT NULL CHECK (drept IN ('citire', 'modificare')),
    PRIMARY KEY (curs_id, grup_id),
    FOREIGN KEY (curs_id) REFERENCES cursuri(id) ON DELETE CASCADE,
    FOREIGN KEY (grup_id) REFERENCES grupuri(id) ON DELETE CASCADE
);

CREATE TABLE permisiuni_lectie_utilizator (
    lectie_id           INTEGER NOT NULL,
    utilizator_id       INTEGER NOT NULL,
    drept               TEXT NOT NULL CHECK (drept IN ('citire', 'modificare')),
    PRIMARY KEY (lectie_id, utilizator_id),
    FOREIGN KEY (lectie_id) REFERENCES lectii(id) ON DELETE CASCADE,
    FOREIGN KEY (utilizator_id) REFERENCES utilizatori(id) ON DELETE CASCADE
);

CREATE TABLE permisiuni_lectie_grup (
    lectie_id           INTEGER NOT NULL,
    grup_id             INTEGER NOT NULL,
    drept               TEXT NOT NULL CHECK (drept IN ('citire', 'modificare')),
    PRIMARY KEY (lectie_id, grup_id),
    FOREIGN KEY (lectie_id) REFERENCES lectii(id) ON DELETE CASCADE,
    FOREIGN KEY (grup_id) REFERENCES grupuri(id) ON DELETE CASCADE
);

-- Evaluare este baza pentru Chestionar si ExamenFinal.
CREATE TABLE evaluari (
    id                  INTEGER PRIMARY KEY,
    curs_id             INTEGER NOT NULL,
    profesor_id         INTEGER NOT NULL,
    nume                TEXT NOT NULL,
    tip                 TEXT NOT NULL CHECK (tip IN ('chestionar', 'examen_final')),
    limita_timp         INTEGER NOT NULL DEFAULT 0 CHECK (limita_timp >= 0),
    este_obligatorie    INTEGER NOT NULL DEFAULT 0 CHECK (este_obligatorie IN (0, 1)),
    creat_la            TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (curs_id) REFERENCES cursuri(id) ON DELETE CASCADE,
    FOREIGN KEY (profesor_id) REFERENCES profesori(utilizator_id) ON DELETE RESTRICT,
    UNIQUE (curs_id, nume)
);

CREATE TABLE chestionare (
    evaluare_id         INTEGER PRIMARY KEY,
    numar_intrebari     INTEGER NOT NULL DEFAULT 0 CHECK (numar_intrebari >= 0),
    FOREIGN KEY (evaluare_id) REFERENCES evaluari(id) ON DELETE CASCADE
);

CREATE TABLE examene_finale (
    evaluare_id         INTEGER PRIMARY KEY,
    pondere             REAL NOT NULL CHECK (pondere >= 0.0 AND pondere <= 1.0),
    FOREIGN KEY (evaluare_id) REFERENCES evaluari(id) ON DELETE CASCADE
);

-- Intrebarile si raspunsurile sunt necesare pentru teste si calcularea notelor.
CREATE TABLE intrebari_chestionar (
    id                  INTEGER PRIMARY KEY,
    chestionar_id       INTEGER NOT NULL,
    enunt               TEXT NOT NULL,
    punctaj_maxim       REAL NOT NULL CHECK (punctaj_maxim >= 0.0),
    ordine              INTEGER NOT NULL CHECK (ordine >= 0),
    FOREIGN KEY (chestionar_id) REFERENCES chestionare(evaluare_id) ON DELETE CASCADE,
    UNIQUE (chestionar_id, ordine)
);

CREATE TABLE incercari_evaluare (
    id                  INTEGER PRIMARY KEY,
    evaluare_id         INTEGER NOT NULL,
    student_id          INTEGER NOT NULL,
    inceputa_la         TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
    finalizata_la       TEXT,
    scor_brut           REAL NOT NULL DEFAULT 0.0 CHECK (scor_brut >= 0.0),
    nota_finala         REAL NOT NULL DEFAULT 0.0 CHECK (nota_finala >= 0.0 AND nota_finala <= 10.0),
    FOREIGN KEY (evaluare_id) REFERENCES evaluari(id) ON DELETE CASCADE,
    FOREIGN KEY (student_id) REFERENCES studenti(utilizator_id) ON DELETE CASCADE
);

CREATE INDEX idx_incercari_student ON incercari_evaluare(student_id);
CREATE INDEX idx_incercari_evaluare ON incercari_evaluare(evaluare_id);

CREATE TABLE raspunsuri_chestionar (
    incercare_id        INTEGER NOT NULL,
    intrebare_id        INTEGER NOT NULL,
    raspuns             TEXT NOT NULL,
    punctaj_obtinut     REAL NOT NULL DEFAULT 0.0 CHECK (punctaj_obtinut >= 0.0),
    PRIMARY KEY (incercare_id, intrebare_id),
    FOREIGN KEY (incercare_id) REFERENCES incercari_evaluare(id) ON DELETE CASCADE,
    FOREIGN KEY (intrebare_id) REFERENCES intrebari_chestionar(id) ON DELETE CASCADE
);

CREATE TABLE certificate (
    id                  INTEGER PRIMARY KEY,
    incercare_id        INTEGER NOT NULL UNIQUE,
    continut            TEXT NOT NULL,
    emis_la             TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (incercare_id) REFERENCES incercari_evaluare(id) ON DELETE CASCADE
);

-- Jurnalizare: logari, creare/modificare continut si acces neautorizat.
CREATE TABLE jurnal_evenimente (
    id                  INTEGER PRIMARY KEY,
    utilizator_id       INTEGER,
    tip_eveniment       TEXT NOT NULL,
    mesaj               TEXT NOT NULL,
    creat_la            TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (utilizator_id) REFERENCES utilizatori(id) ON DELETE SET NULL
);

CREATE INDEX idx_jurnal_evenimente_data ON jurnal_evenimente(creat_la);
CREATE INDEX idx_jurnal_evenimente_utilizator ON jurnal_evenimente(utilizator_id);

COMMIT;
