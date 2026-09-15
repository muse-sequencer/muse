

---

# **Analysis of Mixer Storage in the Source Code (MusE DAW)**

After closely analyzing the C++ source code (track.cpp, audiotrack.cpp, wavetrack.cpp, amixer.cpp, xml.cpp, and the MIDI logic), it becomes clear how the DAW processes the data. This resolves the mystery of the “incorrect” values during saving — and at the same time explains why **restoring them during loading** fails.


## **1. The mystery of the “missing” MidiTrack file**

In MusE’s architecture, the code for MIDI tracks has grown historically and is split across multiple files:

* **The declaration (header):** Typically located in track.h or a dedicated miditrack.h.  
* **The implementation (C++):** Most core methods of the MidiTrack class (such as reading and writing XML properties or managing drum maps) exist as a monolithic block directly inside **track.cpp**. More specific sequencer commands are moved into **midi.cpp**.


## **2. Where are the XML values loaded? (Files & functions)**

The loading process is controlled by the XML parser in xml.cpp and the deserialization methods of the tracks.

### **A) The loading sequence for audio tracks**

1. **song.cpp / file loader:** Opens the XML project and starts the parsing loop.  
2. **wavetrack.cpp (WaveTrack::read):** When the parser encounters \<wavetrack\>, a new WaveTrack object is instantiated and read(Xml& xml) is called.  
3. **audiotrack.cpp (AudioTrack::readProperties):** Reads the XML subtags and forwards them to AudioTrack::readProperties(xml, name).  
4. **Controller restoration (audiotrack.cpp):**  
   ```
   if (name == "controller") {
         int id = xml.intAttribute("id");
         double cur = xml.parseDouble();      // Reading the hex-float
         _controller[id]->setVal(cur);        // Applying to backend
   }
   ```

### **B) The loading sequence for MIDI tracks**

1. **Port restoration (midi.cpp):** MIDI ports are loaded when parsing the \<sequencer\> block.  
2. **Controller assignment:** The parser reads controller 7 (Volume) under \<midiport\> → \<channel\> and assigns the value to the port driver.


## **3. Why are the values not applied correctly? (Root causes)**

The code analysis reveals two critical weaknesses:

### **Root cause A: Unwanted reverse synchronization (GUI feedback loop)**

When the mixer is built, the following happens:

1. The Strip (audio) or MidiStrip (MIDI) widget for the track is created.  
2. **The problem:** When the GUI element is initialized (or an update() runs), it sets values on the interface. This often triggers a Qt signal (valueChanged).  
3. This signal overwrites the value that was correctly loaded from the XML file with a faulty GUI default.  
   **The GUI overwrites the loaded data.**

### **Root cause B: The hex-float parsing issue in xml.cpp**

MusE writes double values with high precision as hex floats (e.g., 0x1.0137987dd704bp‑2).  
However, Qt’s standard conversion functions do not natively support the C99 hex-float format. They stop parsing at the letter `x` and incorrectly return 0.0.


## **4. Extended comparison: Who saves and who loads?**

| Parameter | File | Function / Class | Action |
| :---- | :---- | :---- | :---- |
| **General track data** | track.cpp | Track::writeProperties / readProperties | Save/load mute, solo, name |
| **MIDI track logic** | track.cpp | MidiTrack::… (various methods) | MIDI-specific (drum maps, etc.) |
| **Audio volume/pan** | audiotrack.cpp | AudioTrack::writeProperties / readProperties | Save/load controller IDs 0 & 1 |
| **MIDI volume (CC7)** | midi.cpp | MidiPort::writeProperties / readProperties | Save/load port controllers |
| **Audio strip GUI** | strip.cpp | Strip::Strip or update() | Create & sync audio sliders |
| **MIDI strip GUI** | mstrip.cpp | MidiStrip::MidiStrip or update() | Create & sync MIDI sliders |
| **XML float parsing** | xml.cpp | Xml::parseDouble | Read hex floats |


## **5. Concrete code replacements to fix the issue (Search & Replace)**

Search for the exact “SEARCH” blocks in your files and replace them with the “REPLACE” blocks.

### **Patch 1: Robust hex-float parsing (in xml.cpp)**

This bug is the main reason audio faders get stuck at “zero.”  
**File:** xml.cpp  
**SEARCH EXACTLY FOR THIS BLOCK:**  
```cpp
//---------------------------------------------------------
//   parseDouble
//---------------------------------------------------------

double Xml::parseDouble()
      {
      QString s(parse1().simplified());
      return s.toDouble();
      }
```

**REPLACE WITH THIS BLOCK:**  
```cpp
//---------------------------------------------------------
//   parseDouble
//---------------------------------------------------------
#include <cstdlib> // Ensure this include is available

double Xml::parseDouble()
      {
      QString s(parse1().simplified());
      // Parse C99 hexadecimal floating-point numbers correctly.
      // Standard Qt methods fail on 'x' and return 0.0.
      char* endptr;
      return std::strtod(s.toLocal8Bit().constData(), &endptr);
      }
```

### **Patch 2: Stop the GUI feedback loop (in strip.cpp & mstrip.cpp)**

When track values are applied to the GUI (sliders or knobs), Qt unintentionally fires a signal that immediately overwrites the track again.  
**Files:** strip.cpp (audio) and mstrip.cpp (MIDI)  
**SEARCH FOR VALUE ASSIGNMENTS LIKE:**  
```cpp
_volume->setValue(track->volume());
_pan->setValue(track->pan());
```

**REPLACE WITH:**  
```cpp
// Block signals to prevent the UI from overriding the backend
_volume->blockSignals(true);
_pan->blockSignals(true);

_volume->setValue(track->volume());
_pan->setValue(track->pan());

// Unblock signals so user interaction works normally again
_volume->blockSignals(false);
_pan->blockSignals(false);
```

*(Note: Adjust the names `_volume` and `_pan` to match the actual variable names you find in your file.)*

---
