cmds = {
    "help":         {'short': 'h', 'text': "Print Help text"},
    "skip":         {'short': 's', 'text': "Skip waiting for Wifi and IoT Hub setup"},
    "input":        {'short': 'i', 'text': "arg required: Sets the file used to input commands to device."},
    "output":       {'short': 'o', 'text': "arg required: Sets the file used to save output from device."},
    "port":         {'short': 'p', 'text': "arg required: Sets the port used to connect to serial device."},
    "baudrate":     {'short': 'b', 'text': "arg required: Sets the baud rate of the Serial connection."},
    "mxchip_file":  {'short': 'm', 'text': "arg required: Sets the file used by filesystem to flash mxchip."},
    "device":       {'short': 'd', 'text': "arg required: Sets the type of device connecting to (mxchip or rpi)."},
    "timeout":      {'short': 't', 'text': "arg required: Sets the timeout(in seconds) for rpi test readout."},
    "reset":        {'short': 'r', 'text': "arg required: Sets whether or not to reset the device after test."},
    }
