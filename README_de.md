# Hirmux `hitpag` Kompilierungs- und Bedienungsanleitung

## [Hitmux Offizielle Website https://hitmux.top](https://hitmux.top)

[English](https://github.com/Caokai674674/hitpag/blob/main/README.md) [中文](https://github.com/Caokai674674/hitpag/blob/main/README_zh.md) [日本語](https://github.com/Caokai674674/hitpag/blob/main/README_ja.md) [Deutsch](https://github.com/Caokai674674/hitpag/blob/main/README_de.md) [한국어](https://github.com/Caokai674674/hitpag/blob/main/README_ko.md)


## Einführung

In einer Linux-Umgebung erfordert die Bearbeitung komprimierter Dateien oft das Auswendiglernen verschiedener Befehle und Parameter für Tools wie `tar`, `gzip`, `unzip`, `7z`. Dies kann für neue Benutzer oder diejenigen, die nicht häufig mit Archiven arbeiten, eine erhebliche Herausforderung darstellen. **`hitpag` wurde geschaffen, um dieses Problem zu lösen.**

Hitmux `hitpag` ist ein intelligentes und benutzerfreundliches Kommandozeilen-Tool, das von Hitmux entwickelt wurde, um die Datei-Komprimierungs- und -Dekomprimierungsoperationen unter Linux zu vereinfachen. Seine Kernphilosophie dreht sich um **automatische Erkennung und benutzerfreundliche Interaktion**. Egal, ob Sie ein heruntergeladenes Archiv dekomprimieren oder lokale Dateien zum Teilen packen müssen, `hitpag` identifiziert intelligent den Dateityp basierend auf seiner Erweiterung und ruft automatisch die zugrunde liegenden System-Tools zur Verarbeitung auf, wodurch Sie sich das Merken komplexer Befehle ersparen. Darüber hinaus bietet es einen hilfreichen **interaktiven Modus**, der es Ihnen ermöglicht, Operationen durch einfache Fragen und Antworten leicht abzuschließen, selbst wenn Sie mit den Kommandozeilenparametern nicht vertraut sind.

---

## Kompilierungsanleitung

### Umgebungsanforderungen

* **Betriebssystem**: Linux (erfolgreich getestet unter Ubuntu 22.04 und Debian 12)
* **Compiler**: GCC/G++ mit Unterstützung für den C++17-Standard
* **Build-Tool**: CMake 3.10 oder höher
* **Abhängigkeitsbibliothek**: `stdc++fs` (C++ Filesystem-Bibliothek), wird normalerweise automatisch mit GCC/G++ installiert

### Kompilierungsschritte

1.  **System aktualisieren und Build-Tools installieren**:
    Stellen Sie zunächst sicher, dass Ihr System auf dem neuesten Stand ist und die erforderlichen Kompilierungstools installiert sind:

    ```bash
    sudo apt-get update
    sudo apt-get install -y g++ cmake make
    ```

2.  **In das Projektverzeichnis wechseln und Build-Ordner erstellen**:
    Navigieren Sie zum Stammverzeichnis des `hitpag`-Projekts und erstellen Sie dann den `build`-Ordner und wechseln Sie hinein:

    ```bash
    cd hitpag
    mkdir -p build
    cd build
    ```

3.  **Build-Dateien mit CMake generieren**:
    Führen Sie CMake aus, um das Projekt zu konfigurieren und das Makefile zu generieren:

    ```bash
    cmake ..
    ```

4.  **Projekt kompilieren**:
    Führen Sie den Befehl `make` aus, um `hitpag` zu kompilieren:

    ```bash
    make
    ```

5.  ** (Optional) Im System installieren**:
    Wenn Sie möchten, dass `hitpag` von jedem Verzeichnis aus ausführbar ist, können Sie es in den System-PATH installieren:

    ```bash
    sudo make install
    ```

---

## Bedienungsanleitung

### Grundlegende Verwendung

Die grundlegende Syntax für das `hitpag`-Tool ist sehr einfach:

```bash
hitpag [Optionen] quellpfad zielpfad
```

### Kommandozeilenoptionen

* `-i`: **Interaktiver Modus**. Wenn Sie unsicher sind, wie Sie vorgehen sollen, führt Sie das Tool Schritt für Schritt.
* `-h, --help`: Hilfeinformationen anzeigen.
* `-v`: Versionsinformationen anzeigen.

### Anwendungsbeispiele

1.  **Dateien dekomprimieren**:
    `hitpag` identifiziert automatisch den Archivtyp und dekomprimiert ihn.

    ```bash
    hitpag archive.tar.gz ./extracted_dir
    ```
    Dieser Befehl dekomprimiert `archive.tar.gz` in das Verzeichnis `./extracted_dir`.

2.  **Dateien/Verzeichnisse komprimieren**:
    `hitpag` wählt das Komprimierungsformat automatisch basierend auf der Dateiendung des Zielnamens.

    ```bash
    hitpag ./my_folder my_archive.zip
    ```
    Dieser Befehl komprimiert das Verzeichnis `./my_folder` in `my_archive.zip`.

3.  **Interaktiver Modus**:
    Im interaktiven Modus stellt `hitpag` Fragen und wartet auf Ihre Eingabe.

    ```bash
    hitpag -i big_file.rar
    ```
    Sie werden geleitet, um den Dekomprimierungstyp, den Zielpfad und mehr auszuwählen.

### Unterstützte Komprimierungsformate

`hitpag` unterstützt die folgenden gängigen Komprimierungs-/Dekomprimierungsformate, indem es vorhandene System-Tools aufruft:

* `tar` (unkomprimiert)
* `tar.gz` / `tgz` (gzip-komprimiert)
* `tar.bz2` / `tbz2` (bzip2-komprimiert)
* `tar.xz` / `txz` (xz-komprimiert)
* `zip`
* `rar`
* `7z`

### Wichtige Hinweise

1.  **Stellen Sie sicher, dass die entsprechenden Komprimierungs-/Dekomprimierungs-Tools installiert sind**:
    `hitpag` ist auf die auf Ihrem System installierten Komprimierungstools angewiesen. Führen Sie bitte den folgenden Befehl aus, um sicherzustellen, dass sie alle installiert sind:

    ```bash
    sudo apt-get install -y tar gzip bzip2 xz-utils zip unzip rar unrar p7zip-full
    ```

2.  **Automatische Verzeichniserstellung**:
    Beim Dekomprimieren erstellt `hitpag` automatisch das angegebene Zielverzeichnis für Sie, falls es nicht existiert.

---

## Fehlerbehandlung

Wenn ein Vorgang fehlschlägt, zeigt `hitpag` klare Fehlermeldungen auf Deutsch an, um Ihnen bei der Diagnose des Problems zu helfen. Häufige Fehlermeldungen sind:

* `Fehler: Fehlende Argumente`: Unzureichende Kommandozeilenargumente.
* `Fehler: Quellpfad existiert nicht`: Die angegebene Quelldatei oder das Verzeichnis existiert nicht.
* `Fehler: Ungültiger Zielpfad`: Das Zielpfadformat ist falsch oder nicht zugänglich.
* `Fehler: Unbekanntes Dateiformat`: Das Komprimierungsformat der angegebenen Datei kann nicht identifiziert werden.
* `Fehler: Erforderliches Tool nicht gefunden`: Dem System fehlt das zur Ausführung des Vorgangs erforderliche Komprimierungs-/Dekomprimierungs-Tool.
* `Fehler: Operation fehlgeschlagen`: Die Komprimierungs-/Dekomprimierungsoperation selbst konnte nicht ausgeführt werden.
* `Fehler: Zugriff verweigert`: Keine ausreichenden Berechtigungen für den Zugriff auf die Datei oder das Verzeichnis.
* `Fehler: Nicht genügend Speicherplatz`: Nicht genügend Festplattenspeicher, um den Vorgang abzuschließen.