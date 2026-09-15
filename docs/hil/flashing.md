# Build and flash FC1 and FC2

This procedure targets the two Nucleo-L476RG boards used by the physical HIL
bench. Always select a board by its ST-LINK serial number; `/dev/ttyACM0` and
`/dev/ttyACM1` can change after reconnection.

## Identify the boards

Use the [bench identity table and wiring](hardware.md) to identify FC1 and FC2.
The commands below select those ST-LINK identities explicitly.
Build both images using the [build guide](../build/build_targets.md) first.

## Flash FC1

From the repository root:

```bash
west flash -d build/fc1_stm32 -r openocd \
  --cmd-pre-init "adapter serial 066FFF525771555067225635"
```


## Flash FC2

From the repository root:

```bash
west flash -d build/fc2_stm32 -r openocd \
  --cmd-pre-init "adapter serial 066CFF525771555067193653"
```


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

## Run HIL

Close the FC1 serial terminal, then follow the [HIL execution guide](../validation/hil.md).
Check the selected interface in the run header: automatic discovery can fall back
to loopback. A successful loopback run is not a physical-board result.
