# RoboMaster Control

Firmware and tooling for the NYUSH Robotics Club RoboMaster C Board (STM32F4).

- Documentation index: see [docs/README.md](docs/README.md)
- Quick start: see [Windows/macOS/Linux setup and just commands](docs/quickstart.md)
- Toolchain: CMake + Ninja, ARM GNU Toolchain, OpenOCD + ST-Link/SWD
- Version policy: install current published releases; record versions per build, without old documentation pins
- Target: STM32F407

## Documentation

All maintained technical documentation is organized under [`docs/`](docs/README.md).
The main entry points are:

- **[Project memory](docs/project/PROJECT_MEMO.md)** — constraints, decisions, unknown hardware, and validation status
- **[Architecture overview](docs/architecture/overview.md)** — current contracts, services, adapters, and extension rules
- **[FreeRTOS runtime](docs/architecture/rtos-migration.md)** — runtime design; current startup remains bare-metal (see validation record)
- **[Environment validation](docs/environment-validation.md)** — current build results and RTOS startup findings
- **[Setup guide](docs/tutorials/setup-guide.md)** — development environment, build, and flashing
- **[Message center](docs/protocols/message-center.md)** — publish/subscribe behavior and limits
- **[Legacy vision protocol](docs/protocols/seasky-vision.md)** — current USB CDC/Seasky compatibility protocol

## Repository layout

- `application/` — robot behavior controllers
- `runtime/rtos/` — FreeRTOS configuration, static tasks, and scheduler startup
- `core/`, `services/`, `adapters/` — contracts, decoupled services, and hardware/protocol adapters
- `bsp/` — board-level CAN, SPI, UART, USB, DMA, time, and critical-section access
- `modules/` — reusable device drivers, algorithms, protocols, and message center
- `config/robots/` — robot configuration definitions
- `Inc/`, `Src/` — frozen STM32CubeMX hardware baseline and current bare-metal startup
- `Drivers/`, `Middlewares/` — STM32 libraries plus vendored FreeRTOS Kernel V11.3.0
- `cmake/`, `CMakeLists.txt` — CMake configuration
- `Debug/`, `build/` — build outputs (generated)
- `docs/` — documentation index, project memory, architecture, guides, protocols, tutorials, and archive
- `tests/host/` — hardware-independent core tests

## Build & flash (summary)

### Build

- Configure with the ARM toolchain and choose `infantry_standard` or `sentry_swerve`.
- See the Setup Guide for toolchain requirements and the ARM build commands.

### Flash
Use `just flash` with OpenOCD and ST-Link/SWD. It builds and checks the selected
firmware, validates chip identity/capacity before erasing, and verifies the write.
The default leaves the MCU halted after verification. `just doctor` and
`just flash-plan` do not connect to or reset the MCU.

OpenOCD is the recommended workflow, including Linux ARM64. pyOCD is an alternative
for other workflows; CubeProgrammer is not recommended for this project's daily
development and is neither required nor invoked by the tools.
