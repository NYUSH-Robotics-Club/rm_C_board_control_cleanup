# Repository Guidance

Read `AGENTS.md` and then `docs/project/PROJECT_MEMO.md` before changing this repository.
The current architecture, extension rules, and debugging workflow are documented
in `docs/architecture/overview.md`.

The low-level STM32/CubeMX baseline is frozen by user requirement. Do not edit
`Inc/`, `Src/`, `Drivers/`, `Middlewares/`, `NYUSH_Infantry.ioc`, the startup
assembly, linker script, or `cmake/stm32cubemx/CMakeLists.txt`.

Supported build configurations are `infantry_standard` and `sentry_swerve`.
Unknown chassis, motor, camera, and Jetson protocol details must remain explicit
unsupported placeholders until their real specifications are supplied.

The current firmware is a bare-metal main loop. RTOS migration is a requested
target documented in `docs/architecture/rtos-migration.md`, but it is not enabled
and must not bypass the frozen low-level boundary.
