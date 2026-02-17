# Controls

## Authentication Screen

- `Tab`: switch active input field (username/password)
- `F1`: toggle mode (Login/Register)
- `Enter`: submit auth request
- `Backspace`: delete one character in active field
- Text input: printable ASCII characters

## Character Selection Screen

- `Left` or `A`: previous class
- `Right` or `D`: next class
- `Enter`: confirm class and start world session

Default classes:

- Warrior
- Mage
- Rogue

## World Screen

- `W` or `Up`: move forward (-Z)
- `S` or `Down`: move backward (+Z)
- `A` or `Left`: move left (-X)
- `D` or `Right`: move right (+X)
- `Esc`: leave world socket and return to auth screen

Movement details:

- Direction is normalized to prevent faster diagonal movement.
- Speed is `4.0` world units/second.
- Movement packets are throttled to 20 Hz.

## Camera Behavior

Camera is automatic and follows the local player; there are no manual mouse camera controls in the current implementation.
