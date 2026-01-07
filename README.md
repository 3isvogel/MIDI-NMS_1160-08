# MIDI-NMS_1160-08
My documentation for a MIDI controller for the Philips NMS 1160/08 matrix keyboard for MSX 

# Reference documentation

The service manual is present in the [Internet Archive](https://archive.org/details/philipsnms12051160sm), a local copy is kept in this repo as well at [philipsnms12051160sm.pdf](assets/philipsnms12051160sm.pdf)

Based on this reference, pins **1**-**8** are used to activate the scan columns, while pins **9**-**16** are used for scanning the rows.

Pins 17-20 are not used

## Key reading logic

The scan can be performed using two different configurations for the input

### `INPUT_PULLUP`
```mermaid
---
config:
  flowchart:
    curve: stepBefore
---
flowchart TD
        vcc{{"Enable (HIGH)"}}
        pullup@{label: "1kOhm"}
        n1((" "))
        input{{DigitalRead}}
        ground@{shape: flip-tri, label: " "}
        button{button}
        diode[\diode/]

        vcc --- pullup --- n1 --- button --- diode --- ground
        input --- n1
```

### `INPUT_PULLDOWN`
```mermaid
---
config:
  flowchart:
    curve: stepBefore
---
flowchart TD
        vcc@{shape: tri, label: " "}
        pulldown@{label: "1kOhm"}
        n1((" "))
        input{{DigitalRead}}
        ground{{"Enable (LOW)"}}
        button{button}
        diode[\diode/]

        vcc --- button --- diode --- n1 --- pulldown --- ground
        input --- n1
```

Either of the two logics can be used, note that:
- The first schematics uses a pullup resistor, all Arduino-compatible boards should support the use of builting pullup resistors for input using pinMode `INPUT_PULLUP`, making it easy to build a more compact circuit
- Only some implementations support pinMode `INPUT_PULLDOWN` so if you plan on using it check that your chipset supports it
- By using `INPUT_PULLUP` the in-memory representation of the keyboard state becomes more intuitive (look at [Notes Logic](#notes-logic-placeholder))
- If you use `INPUT_PULLUP` remember that the state is the opposite of what you would normally expect (Pressed == LOW)

## Pin mapping

The official documentation repotrs the pin numbering for the keyboard connector (On the MSX side)
![pin mapping](assets/input_output.jpg)

- In <b style="color: red;">RED</b> are the input pins for the keyboard (used to enable the scan columns)
- In <b style="color: blue;">BLUE</b> are the output pins, connected directly to the arduino

## Notes Logic PLACEHOLDER

## Components
I am using an **Arduino pro micro** (MEGA32U4) and a **SN74HC4051N** multiplexer, but any equivalent board/component can be used

## Connections

I found comfortable to keep the two components on a single soldering board side by side (Looking at your board the components should look like this (labels with the correc orientation))
```mermaid
flowchart TD
        c((dot))
        m@{label: "SN74HC4051N"}
        a@{label: "Arduino pro micro"}
        h[port]
```

The multiplexer should have the circle on the left and the arduino the microUSB on the right

Pin numbering in th emultiplexer starts from the bottom left corner, proceding counterclockwise (so the bottom-left pin is pin 1, going right up to pin 8, while the top-right pin is pin 9, going left up to pin 16)

Connections are as follow

```mermaid
flowchart TD
  km{{Keyboard input}}
  a@{label: "Arduino pro micro"}
  ka{{Keyboard output}}
  m@{label: "SN74HC4051N"}

  km ---|15-15| m
  km ---|12-14| m
  km ---|16-13| m
  km ---|11-12| m
  km ---|14-1| m
  km ---|13-2| m
  km ---|9-4| m
  km ---|10-5| m

  m ---|11-16| a
  m ---|10-14| a
  m ---|9-15| a

  a ---|9-3| ka
  a ---|8-1| ka
  a ---|7-2| ka
  a ---|6-6| ka
  a ---|5-8| ka
  a ---|4-5| ka
  a ---|3-7| ka
  a ---|2-4| ka
```

HUH?! how do I read this?

### How to read this

```mermaid
flowchart TD
  A ---|x-y| B ---|z-w| C
```

Pin `x` of `A` is to connect with pin `y` of `B`, pin `z` of `B` is to connect with pin `w` of `C`

In short (`A.x` --> `B.y`,  `B.z` --> `C.w`, and so on)

## Multiplexer power

Additionally, multiplexer's pin `16` is to connect with the `5v` power line, while pins `3`, `6`, `7` and `8` are to connect with `gnd`