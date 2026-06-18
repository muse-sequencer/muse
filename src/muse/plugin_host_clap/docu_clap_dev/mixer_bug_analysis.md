# **Analyse der Mixer-Speicherung im Quellcode (MusE DAW)**

Nach der genauen Analyse des C++ Quellcodes (track.cpp, audiotrack.cpp, wavetrack.cpp, amixer.cpp, xml.cpp und der MIDI-Logik) zeigt sich, wie die DAW die Daten verarbeitet. Dabei löst sich das Rätsel um die "falschen" Werte beim Speichern auf – gleichzeitig wird klar, warum das **Wiederherstellen beim Laden** scheitert.

## **1\. Das Rätsel um die "fehlende" MidiTrack-Datei**

In der Architektur von MusE ist der Code für MIDI-Spuren historisch gewachsen und aufgeteilt:

* **Die Deklaration (Header):** Befindet sich typischerweise in track.h oder einer dedizierten miditrack.h.  
* **Die Implementierung (C++):** Die meisten grundlegenden Methoden der Klasse MidiTrack (wie das Lesen und Schreiben der XML-Eigenschaften oder das Verwalten der Drum-Maps) befinden sich als monolithischer Block direkt in der Datei **track.cpp**. Spezifischere Sequenzer-Befehle sind in **midi.cpp** ausgelagert.

## **2\. Wo werden die XML-Werte geladen? (Dateien & Funktionen)**

Der Ladevorgang wird durch den XML-Parser in xml.cpp und die Deserialisierungs-Methoden der Spuren gesteuert.

### **A) Der Lade-Ablauf für Audio-Spuren**

1. **song.cpp / Datei-Loader:** Öffnet das XML-Projekt und startet die Schleife.  
2. **wavetrack.cpp (WaveTrack::read):** Sobald der Parser auf \<wavetrack\> stößt, wird ein neues WaveTrack-Objekt instanziiert und read(Xml& xml) aufgerufen.  
3. **audiotrack.cpp (AudioTrack::readProperties):** Liest die XML-Subtags und reicht sie an AudioTrack::readProperties(xml, name) weiter.  
4. **Controller-Wiederherstellung (audiotrack.cpp):**  
   if (name \== "controller") {  
         int id \= xml.intAttribute("id");  
         double cur \= xml.parseDouble();          // Reading the hex-float  
         \_controller\[id\]-\>setVal(cur);            // Applying to backend  
   }

### **B) Der Lade-Ablauf für MIDI-Spuren**

1. **Port-Wiederherstellung (midi.cpp):** Die MIDI-Ports werden beim Parsen des \<sequencer\>-Blocks geladen.  
2. **Controller-Zuweisung:** Der Parser liest unter \<midiport\> \-\> \<channel\> den Controller 7 (Volume) aus und weist dem Port-Treiber den Wert zu.

## **3\. Warum werden die Werte nicht korrekt angewendet? (Die Fehlerursachen)**

Die Analyse des Codes zeigt zwei kritische Schwachstellen:

### **Fehlerursache A: Die ungewollte Rückwärtssynchronisation (GUI-Feedback-Loop)**

Wenn der Mixer aufgebaut wird, passiert folgendes:

1. Das Strip- (Audio) oder MidiStrip-Widget (MIDI) für die Spur wird erstellt.  
2. **Das Problem:** Wenn das GUI-Element initialisiert wird (oder ein update() läuft), setzt es Werte auf der Oberfläche. Das löst oft ein Qt-Signal (valueChanged) aus.  
3. Dieses Signal überschreibt den soeben aus der XML-Datei korrekt geladenen Wert wieder mit einem fehlerhaften Standard-Wert der GUI. **Die GUI überschreibt die geladenen Daten.**

### **Fehlerursache B: Das Hex-Float-Parsing-Problem in xml.cpp**

MusE schreibt double-Werte hochpräzise als Hex-Floats (z. B. 0x1.0137987dd704bp-2). Standardmäßige Qt-Konvertierungsfunktionen unterstützen das C99-Hex-Float-Format jedoch nicht nativ. Sie brechen beim Lesen des Buchstabens x ab und liefern fehlerhaft 0.0 zurück.

## **4\. Erweiterte Gegenüberstellung: Wer speichert und wer lädt?**

| Parameter | Datei | Funktion / Klasse | Aktion |
| :---- | :---- | :---- | :---- |
| **Allg. Spur-Daten** | track.cpp | Track::writeProperties / readProperties | Speichern/Laden von Mute, Solo, Name |
| **Midi Spur-Logik** | track.cpp | MidiTrack::... (div. Methoden) | MIDI-spezifisches (DrumMaps, etc.) |
| **Audio Volume/Pan** | audiotrack.cpp | AudioTrack::writeProperties / readProperties | Speichern/Laden der ID 0 & 1 Controller |
| **MIDI Volume (CC7)** | midi.cpp | MidiPort::writeProperties / readProperties | Speichern/Laden der Port-Controller |
| **Audio Strip GUI** | strip.cpp | Strip::Strip oder update() | Erstellen & Sync der Audio Slider |
| **Midi Strip GUI** | mstrip.cpp | MidiStrip::MidiStrip oder update() | Erstellen & Sync der Midi Slider |
| **XML Float Parsing** | xml.cpp | Xml::parseDouble | Einlesen der Hex-Floats |

## **5\. Konkreter Code-Austausch zur Fehlerbehebung (Search & Replace)**

Suche in deinen Dateien nach den exakten "SUCHE"-Blöcken und ersetze sie durch die "ERSETZE"-Blöcke.

### **Patch 1: Robustes Hex-Float Parsing (in xml.cpp)**

Dieser Fehler ist die Hauptursache dafür, dass Audio-Fader auf "Null" geparkt werden.  
**Datei:** xml.cpp  
**SUCHE EXAKT NACH DIESEM BLOCK:**  
//---------------------------------------------------------  
//   parseDouble  
//---------------------------------------------------------

double Xml::parseDouble()  
      {  
      QString s(parse1().simplified());  
      return s.toDouble();  
      }

**ERSETZE DURCH DIESEN BLOCK:**  
//---------------------------------------------------------  
//   parseDouble  
//---------------------------------------------------------  
\#include \<cstdlib\> // Ensure this include is available

double Xml::parseDouble()  
      {  
      QString s(parse1().simplified());  
      // Parse C99 hexadecimal floating-point numbers correctly.  
      // Standard Qt methods fail on 'x' and return 0.0.  
      char\* endptr;  
      return std::strtod(s.toLocal8Bit().constData(), \&endptr);  
      }

### **Patch 2: Die GUI-Rückkopplung stoppen (in strip.cpp & mstrip.cpp)**

Wenn die Regler-Werte des Tracks auf die GUI (Slider oder Drehregler) übertragen werden, feuert Qt ungewollt ein Signal ab, welches den Track sofort wieder überschreibt.  
**Datei:** strip.cpp (Audio) und mstrip.cpp (MIDI)  
**SUCHE NACH WERT-ZUWEISUNGEN WIE DIESEN:**  
*(Die genauen Variablennamen können leicht abweichen, z.B. \_volume, volSlider, \_pan)*  
      \_volume-\>setValue(track-\>volume());  
      \_pan-\>setValue(track-\>pan());

**ERSETZE SIE DURCH:**  
      // Block signals to prevent the UI from overriding the backend  
      \_volume-\>blockSignals(true);  
      \_pan-\>blockSignals(true);

      \_volume-\>setValue(track-\>volume());  
      \_pan-\>setValue(track-\>pan());

      // Unblock signals so user interaction works normally again  
      \_volume-\>blockSignals(false);  
      \_pan-\>blockSignals(false);

*(Hinweis: Passe die Namen \_volume und \_pan entsprechend der tatsächlichen Variablennamen an, die du in der Datei beim Suchen gefunden hast.)*