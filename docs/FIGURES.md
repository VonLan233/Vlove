# Vlove System Diagrams

## Figure 1: System Architecture Overview

```mermaid
flowchart TB
    subgraph ESP32["ESP32 Firmware"]
        Sensors["5x Potentiometers"]
        Filter["AnalogFilter"]
        Calib["Calibration"]

        subgraph Modes["Operation Modes"]
            Gesture["Gesture Recognition"]
            Piano["Air Piano"]
            Raw["Raw Data"]
            VR["OpenGloves/VR"]
        end

        Comm["Communication"]
    end

    subgraph PC["Python Client"]
        Client["VloveClient"]
        Audio["AudioPlayer"]
    end

    Sensors --> Filter
    Filter --> Calib
    Calib --> Modes
    Modes --> Comm
    Comm -->|"USB/Bluetooth"| Client
    Client --> Audio
```

## Figure 2: Main Control Loop

```mermaid
flowchart TD
    Start([Loop Start]) --> Cmd{Serial Command?}
    Cmd -->|Yes| Process[Process Command]
    Cmd -->|No| IMU[Update IMU]
    Process --> IMU

    IMU --> Read[Read Filtered Sensors]
    Read --> CalCheck{Calibrating?}

    CalCheck -->|Yes| CalUpdate[Update Histogram]
    CalUpdate --> Delay[Delay 10ms]

    CalCheck -->|No| Map[Map to Calibrated Range]
    Map --> ModeSwitch{Current Mode}

    ModeSwitch -->|HOME| Delay
    ModeSwitch -->|GESTURE| GestureProc[Process Gesture]
    ModeSwitch -->|PIANO| PianoProc[Process Piano]
    ModeSwitch -->|RAW| RawProc[Send Raw Data]
    ModeSwitch -->|VR| VRProc[Send OpenGloves]

    GestureProc --> Send[Send via Comm]
    PianoProc --> Send
    RawProc --> Send
    VRProc --> Send

    Send --> Delay
    Delay --> Start
```

## Figure 3: Signal Filtering Pipeline

```mermaid
flowchart LR
    ADC["Raw ADC\n(0-4095)"] --> OS["Oversampling\n(4x Average)"]
    OS --> Median["Median Filter\n(Window=5)"]
    Median --> EMA["EMA Smoothing\n(α=0.3)"]
    EMA --> DZ["Deadzone\n(±15)"]
    DZ --> Output["Filtered\nOutput"]

    style ADC fill:#ffcccc
    style Output fill:#ccffcc
```

## Figure 4: Calibration Process

```mermaid
flowchart TD
    Start([CAL Command]) --> Init[Initialize Histogram\n256 bins per finger]
    Init --> Prompt[/"Move fingers through\nfull range"/]

    Prompt --> Sample[Read Sensor Value]
    Sample --> Stable{Value Stable?\nΔ < 50}

    Stable -->|No| Sample
    Stable -->|Yes| AddHist[Add to Histogram]
    AddHist --> Done{DONE Command?}

    Done -->|No| Sample
    Done -->|Yes| Calc[Calculate Percentiles]

    Calc --> P2["Min = 2nd Percentile"]
    Calc --> P98["Max = 98th Percentile"]

    P2 --> Margin[Add 5% Margin]
    P98 --> Margin

    Margin --> Save[(Save to EEPROM)]
    Save --> End([Calibration Complete])
```

## Figure 5: Static Gesture Matching

```mermaid
flowchart TD
    Input["Finger Values\n[0-4095]"] --> Norm["Normalize to\n[0-255]"]
    Norm --> Loop{For Each Gesture\nin Priority Order}

    Loop --> Check["Check 5 Finger\nConstraints"]
    Check --> Match{All Match?}

    Match -->|No| Next{More\nGestures?}
    Next -->|Yes| Loop
    Next -->|No| None["Return NONE"]

    Match -->|Yes| Debounce{Same as\nLast Frame?}
    Debounce -->|No| Store[Store & Count=1]
    Store --> None

    Debounce -->|Yes| Inc[Count++]
    Inc --> Stable{Count ≥ 2?}
    Stable -->|No| None
    Stable -->|Yes| Return["Return Gesture ID\n+ Confidence"]

    style Input fill:#ffcccc
    style Return fill:#ccffcc
    style None fill:#ffffcc
```

## Figure 6: Dynamic Gesture State Machine

```mermaid
stateDiagram-v2
    [*] --> Idle

    Idle --> Phase0: Fingers match Phase 0

    Phase0 --> Phase1: Min duration met\n& Phase 1 matches
    Phase0 --> Idle: Timeout or\nmismatch

    Phase1 --> Phase2: Min duration met\n& Phase 2 matches
    Phase1 --> Idle: Timeout or\nmismatch

    Phase2 --> Phase3: Min duration met\n& Phase 3 matches
    Phase2 --> Idle: Timeout or\nmismatch

    Phase3 --> Triggered: Min duration met\n(Final phase)
    Phase3 --> Idle: Timeout or\nmismatch

    Triggered --> Cooldown: Gesture Reported
    Cooldown --> Idle: Cooldown elapsed
```

## Figure 7: Piano Single Note Mode

```mermaid
flowchart TD
    Input["Finger Value"] --> CheckOn{Value > 1500?}

    CheckOn -->|Yes| WasActive{Note Active?}
    WasActive -->|No| NoteOn["Send NOTE_ON\nCalculate Velocity"]
    WasActive -->|Yes| Hold[Hold Note]

    CheckOn -->|No| CheckOff{Value < 1000?}
    CheckOff -->|Yes| WasOn{Note Active?}
    WasOn -->|Yes| NoteOff[Send NOTE_OFF]
    WasOn -->|No| Idle[Idle]

    CheckOff -->|No| Hysteresis[In Hysteresis Zone\nMaintain State]

    NoteOn --> SetActive[Set Note Active]
    NoteOff --> SetInactive[Set Note Inactive]

    style NoteOn fill:#ccffcc
    style NoteOff fill:#ffcccc
    style Hysteresis fill:#ffffcc
```

## Figure 8: Communication Protocol

```mermaid
flowchart LR
    subgraph ESP32
        GestureOut["Gesture Event"]
        PianoOut["Piano Event"]
        RawOut["Raw Data"]
    end

    subgraph Protocol["Message Format"]
        G["G,id,name"]
        P["P,ON/OFF,note,vel"]
        R["R,raw...,mapped..."]
    end

    subgraph Client["Python Client"]
        Parse["Parse by Prefix"]
        GestureIn["handle_gesture()"]
        PianoIn["handle_piano()"]
        RawIn["handle_raw()"]
    end

    GestureOut --> G
    PianoOut --> P
    RawOut --> R

    G -->|Serial/BT| Parse
    P -->|Serial/BT| Parse
    R -->|Serial/BT| Parse

    Parse -->|"G,"| GestureIn
    Parse -->|"P,"| PianoIn
    Parse -->|"R,"| RawIn
```

## Figure 9: Bluetooth Connection Flow

```mermaid
sequenceDiagram
    participant User
    participant ESP32
    participant PC as PC Bluetooth
    participant Client as Python Client

    User->>ESP32: Connect via USB
    User->>ESP32: Send "BT" command
    ESP32->>ESP32: Enable BluetoothSerial
    ESP32-->>PC: Advertise "vlove-left"

    User->>PC: Pair device
    PC->>ESP32: Pairing request
    ESP32-->>PC: Pairing confirmed

    User->>Client: Run with --bt flag
    Client->>Client: Scan for "vlove-left"
    Client->>PC: Open /dev/cu.vlove-left
    PC->>ESP32: SPP Connection

    loop Data Transfer
        ESP32->>Client: Sensor data
        Client->>ESP32: Commands
    end
```

## Figure 10: Audio Synthesis Pipeline

```mermaid
flowchart LR
    MIDI["MIDI Note\n(0-127)"] --> Freq["Convert to\nFrequency"]
    Freq --> Wave["Generate Waveform\nf + 2f + 3f"]
    Wave --> ADSR["Apply ADSR\nEnvelope"]
    ADSR --> Vol["Scale by\nVelocity"]
    Vol --> Play["sounddevice\nplay()"]

    subgraph Harmonics["Waveform"]
        H1["Fundamental (100%)"]
        H2["2nd Harmonic (50%)"]
        H3["3rd Harmonic (25%)"]
    end

    subgraph Envelope["ADSR"]
        A["Attack 20ms"]
        D["Decay 100ms"]
        S["Sustain 70%"]
        R["Release 150ms"]
    end
```
