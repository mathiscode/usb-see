# usb-see

A simple tool written in C to detect changes in USB devices, and optionally execute a command when a change is detected.

This is designed to be used in scripts or other applications, or running as a daemon with the `watch` command.

You may alternatively use `udev` rules to monitor USB events, but this tool is more lightweight and easier to use.

If your goal is to mitigate the risk of USB-based attacks, consider using [usbguard](https://usbguard.github.io), which allows you to define policies for USB devices.

## Installation

Download the latest release from the [releases page](https://github.com/mathiscode/usb-see/releases) and extract the binary to a directory in your PATH.

-- OR --

```sh
git clone --depth=1 https://github.com/mathiscode/usb-see.git
cd usb-see

make && make install-user
# or
make && sudo make install-system

which usb-see # should show the path to the installed binary
```

To uninstall, run `make uninstall-user` or `sudo make uninstall-system` in the usb-see repository, or simply delete the `usb-see` binary.

## Usage

usb-see *[OPTIONS]* **COMMAND**

## Options

- `--file` or `-f` - Path to the USB state file. Default: ~/.config/usb.state
- `--exec` or `-e` - Command to execute when a change is detected. Default: false
  - The command may contain the following placeholders:
    - `_USB_` - The USB device line from `lsusb`
    - `_ACTION_` - The action that occurred (`added` or `removed`)

## Commands

- `freeze` - Freeze the current USB state to the state file
- `scan` - Scan for any added or removed USB devices
- `watch` - Watch for changes in USB devices and execute a command when a change is detected

## Examples

- Freeze the current USB state to the default state file and scan for changes

  ```sh
  usb-see freeze
  usb-see scan
  ```

- Freeze the current USB state to a custom state file

  ```sh
  usb-see --file /tmp/usb.state freeze
  ```

- Watch for changes in USB devices and execute a command when a change is detected

  ```sh
  usb-see --exec 'echo "_USB_ was _ACTION_"' watch
  ```

## Scan Exit Codes

- 0: Success; no changes detected
- 1: Device added
- 2: Device removed
- 255: An error occurred, most likely file access
