# Board Ball Game

A tilt-controlled ball game on an STM32. Tilt the board – the ball rolls across a 32×8 LED matrix, bounces off the walls and eats the food that appears in random spots.

<!-- TODO: add a GIF of the game running on the board -->

## How it works

- **Input.** An MPU6050 accelerometer is polled over I2C every 15 ms. The X/Y acceleration (with a dead zone and calibration offsets) becomes the "gravity" acting on the ball.
- **Physics.** The ball has a continuous position and velocity: tilt accelerates it up to a speed cap, friction slows it down when the board is level, and it bounces off the walls with energy loss.
- **Display.** Four daisy-chained MAX7219 8×8 modules over SPI form a 32×8 field with a border. The frame is redrawn only when the ball actually moves to another pixel.
- **Food.** A xorshift RNG seeded from the system tick spawns food anywhere inside the field except under the ball; eat it and the next one appears.
- **Robustness.** If the I2C bus or the sensor fails, the driver reinitializes I2C and the MPU6050 and the game keeps going.

Lightweight MAX7219 and MPU6050 drivers on top of STM32 HAL live in `Lib/`.

## Hardware

| Part | Notes |
| :-- | :-- |
| STM32F411CEU6 board (e.g. Black Pill) | runs at 16 MHz from the internal oscillator |
| 4× MAX7219 8×8 LED matrix modules | chained, 32×8 total |
| MPU6050 module | accelerometer + gyroscope |
| ST-Link | flashing and debugging |

### Wiring

| STM32 | Connected to |
| :-- | :-- |
| PA5 (SPI1 SCK) | MAX7219 CLK |
| PA7 (SPI1 MOSI) | MAX7219 DIN |
| PA4 | MAX7219 CS |
| PB6 (I2C1 SCL) | MPU6050 SCL |
| PB7 (I2C1 SDA) | MPU6050 SDA |

Power the MAX7219 chain from 5 V and the MPU6050 from 3.3 V; all grounds common.

## Build and flash

Requires the Arm GNU toolchain (`arm-none-eabi-gcc`), CMake and Ninja.

```bash
cmake --preset Debug
cmake --build --preset Debug
```

The firmware ends up in `build/Debug/`. Flash it with STM32CubeProgrammer, or open the folder in VS Code with the STM32Cube extension and use the ST-Link launch configuration from `.vscode/launch.json`.

The pin configuration lives in `board_game.ioc` (STM32CubeMX).

## Ideas

- Score counter and a game-over condition
- Buttons on PB12–PB15 (already configured as inputs) for restart and modes
- Obstacles and levels
