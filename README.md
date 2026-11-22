# Vocal Gloves

A wearable gesture-to-speech system that uses motion sensors and wireless communication to recognize hand gestures and play corresponding voice prompts. The project is designed around two sensor gloves and a central audio receiver.

> **Project status:** Hardware and firmware are under development. The left-glove sketch is intentionally a placeholder so its firmware can be added separately.

## Project Photos

Add your project photos to `assets/images/` using the filenames below. The placeholders will display automatically once the images are committed.

### Vocal Gloves — Overview
![Vocal Gloves overview](assets/images/vocal-gloves-overview.jpg)

### Glove Hardware
![Vocal Gloves hardware](assets/images/vocal-gloves-hardware.jpg)

### Receiver Unit
![Vocal Gloves receiver](assets/images/vocal-gloves-receiver.jpg)

## System Architecture

- **Left glove:** Reads motion data from an MPU6050, identifies left-hand gestures, and transmits gesture packets over nRF24L01.
- **Right glove:** Reads motion data from an MPU6050, identifies right-hand gestures, and transmits gesture packets over nRF24L01.
- **Receiver:** Receives gesture packets, selects the corresponding audio track, and plays it through the DFPlayer Mini.

The current receiver firmware expects gesture IDs **G01–G06** from the left glove and **G07–G12** from the right glove. It uses language folder **1 for Nepali** and **2 for English** on the DFPlayer-compatible microSD card.

## Hardware Components

### Each Glove (Left and Right)

| Component | Quantity per glove | Purpose |
|---|---:|---|
| Arduino Nano | 1 | Main controller |
| MPU6050 | 1 | Accelerometer and gyroscope for motion sensing |
| nRF24L01 | 1 | Wireless communication with the receiver |
| TP4056 3.7V LiPo charging module | 1 | LiPo battery charging |
| Voltage booster module | 1 | Power conversion for the glove circuit |
| 500mAh 3.7V LiPo battery | 1 | Portable power source |

### Receiver Unit

| Component | Quantity | Purpose |
|---|---:|---|
| Arduino Nano | 1 | Main receiver controller |
| TP4056 3.7V LiPo charging module | 1 | LiPo battery charging |
| nRF24L01 | 1 | Receives wireless gesture packets |
| microSD card | 1 | Stores audio tracks for the DFPlayer Mini |
| DFPlayer Mini | 1 | Plays the selected voice prompt |
| 500mAh 3.7V LiPo battery | 1 | Portable power source |
| AUX female port | 1 | Audio output connection |

## Firmware Structure

```text
Vocal-Gloves/
├── firmware/
│   ├── left_glove/
│   │   └── left_glove.ino
│   ├── right_glove/
│   │   └── right_glove.ino
│   └── receiver/
│       └── receiver.ino
├── assets/
│   └── images/
│       └── (add project photos here)
├── LICENSE
└── README.md
```

### Firmware Files

| File | Description |
|---|---|
| `firmware/left_glove/left_glove.ino` | Placeholder for the left-glove firmware. Add your code here. |
| `firmware/right_glove/right_glove.ino` | Uploaded right-glove firmware. |
| `firmware/receiver/receiver.ino` | Uploaded receiver firmware. |

## Gesture and Communication Notes

The supplied right-glove sketch assigns its six detected gestures to **G07–G12** and sends packets with hand ID `2`.

The supplied receiver sketch:
- Accepts hand IDs `1` (left) and `2` (right).
- Plays gesture tracks G01–G12 from the selected language folder.
- Supports master enable/disable commands from the left-hand controller.
- Accepts language-folder selection commands from the right-hand controller.

The left-glove firmware should follow the packet format and command conventions used by the receiver and right-glove sketch.

## Software Requirements

Install the following Arduino libraries through the Arduino IDE Library Manager:

- **RF24** by TMRh20
- **DFRobotDFPlayerMini**

The Arduino core also provides `Wire`, `SPI`, and `SoftwareSerial` for the Nano platform.

## Audio Files

Prepare the microSD card in a format supported by the DFPlayer Mini. The supplied receiver code selects:

- Folder `01` — Nepali audio
- Folder `02` — English audio

Each folder should contain numbered audio tracks corresponding to gesture IDs (for example, track 1 for G01 and track 12 for G12). Confirm the DFPlayer Mini's supported file format and naming requirements before loading the card.

## Important Hardware Notes

- The nRF24L01 operates at **3.3V**. Do not connect its VCC pin to the Nano's 5V output. Use a stable 3.3V supply and suitable logic-level interfacing where required.
- Check the voltage booster output and the power requirements of every module before connecting the battery.
- Use a TP4056 module appropriate for the specific LiPo cell and follow safe LiPo charging practices. Do not charge a damaged or swollen battery.
- Verify the actual pin connections against your hardware before powering the circuit. Pin assignments and wiring diagrams will be documented here once finalized.

## Contributing

Contributions, improvements, testing notes, and hardware documentation are welcome. Please keep firmware changes organized by device: left glove, right glove, or receiver.

## License

This project is released under the MIT License. See [LICENSE](LICENSE).
