# Build and flash FC1 and FC2

This procedure targets the two Nucleo-L476RG boards used by the physical HIL
bench. Always select a board by its ST-LINK serial number; `/dev/ttyACM0` and
`/dev/ttyACM1` can change after reconnection.

## Identify the boards

List the persistent USB identities:

```bash
ls -l /dev/serial/by-id/usb-STMicroelectronics_STM32_STLink_*-if02
```

The current bench mapping is:

| Role | ST-LINK serial | Stable console device | Baud rate |
| --- | --- | --- | --- |
| FC1 | `066FFF525771555067225635` | `/dev/serial/by-id/usb-STMicroelectronics_STM32_STLink_066FFF525771555067225635-if02` | 460800 |
| FC2 | `066CFF525771555067193653` | `/dev/serial/by-id/usb-STMicroelectronics_STM32_STLink_066CFF525771555067193653-if02` | 115200 |

If a board is replaced, establish and record its new role before flashing it.

## Build and flash FC1

From the repository root:

```bash
west build -b nucleo_l476rg apps/fc1_stm32 -d build/fc1_stm32 --pristine
west flash -d build/fc1_stm32 -r openocd \
  --cmd-pre-init "adapter serial 066FFF525771555067225635"
```

The named outputs are written to `artifacts/stm32/fc1_stm32.elf`,
`artifacts/stm32/fc1_stm32.hex`, and `artifacts/stm32/fc1_stm32.bin`.

## Build and flash FC2

From the repository root:

```bash
west build -b nucleo_l476rg apps/fc2_stm32 -d build/fc2_stm32 --pristine
west flash -d build/fc2_stm32 -r openocd \
  --cmd-pre-init "adapter serial 066CFF525771555067193653"
```

The named outputs are written to `artifacts/stm32/fc2_stm32.elf`,
`artifacts/stm32/fc2_stm32.hex`, and `artifacts/stm32/fc2_stm32.bin`.

`west flash` uses the image in the selected build directory, verifies it, and
resets that target. Rebuild whenever sources change. Use `--skip-rebuild` only
when the selected build directory is known to be current.

If explicit ST-LINK selection is unavailable, disconnect the other Nucleo and
run `west flash -d build/fc1_stm32` or `west flash -d build/fc2_stm32` with only
the intended target attached.

## Verify the firmware roles

Reset both boards and inspect one console at a time with `picocom` or an
equivalent serial terminal:

```bash
picocom -b 460800 /dev/serial/by-id/usb-STMicroelectronics_STM32_STLink_066FFF525771555067225635-if02
picocom -b 115200 /dev/serial/by-id/usb-STMicroelectronics_STM32_STLink_066CFF525771555067193653-if02
```

FC1 must print `firmware=fc1_stm32 role=FC1`. FC2 must print
`firmware=fc2_stm32 role=FC2`. Exit the terminal before launching the HIL runner,
because the FC1 console is also the host HIL transport.

## Run the physical HIL scenario

FC1 and FC2 communicate through USART3 at 115200 baud. FC1 uses its ST-LINK USB
console at 460800 baud for the host HIL protocol. With both boards flashed and
the USART3 link connected, run:

```bash
./artifacts/linux/hil_runner --scenario NOMINAL-001
./artifacts/linux/hil_runner --scenario FAULT_INJECTOR-003
```

The runner automatically selects the trusted FC1 identity. It can also be made
explicit:

```bash
./artifacts/linux/hil_runner --scenario FAULT_INJECTOR-003 \
  --interface /dev/serial/by-id/usb-STMicroelectronics_STM32_STLink_066FFF525771555067225635-if02
```

