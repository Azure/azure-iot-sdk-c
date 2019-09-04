import serial
import os
import sys
import getopt
import time
import subprocess

try:
    import testtools.UART_interface.azure_test_firmware_errors as azure_test_firmware_errors
    import testtools.UART_interface.serial_settings as serial_settings
    import testtools.UART_interface.serial_commands_dict as commands_dict
    import testtools.UART_interface.rpi_uart_interface
    import testtools.UART_interface.mxchip_uart_interface
    import testtools.UART_interface.lin_ipc_interface
    import testtools.UART_interface.interactive_shell
except:
    import azure_test_firmware_errors
    import serial_settings
    import serial_commands_dict as commands_dict
    import rpi_uart_interface
    import mxchip_uart_interface
    import lin_ipc_interface
    import interactive_shell

# Note: commands on MXCHIP have line endings with \r AND \n
# Notes: This is designed to be used as a command line script with args (for automation purposes) to communicate over serial to a Microsoft mxchip device.

uart = None

def usage():
    # Iterates through command dictionary to print out script's opt usage
    usage_txt = "serial_connect.py usage: \r\n"
    for commands in commands_dict.cmds:
        usage_txt += " - %s: " %commands + commands_dict.cmds[commands]['text'] + "\r\n"

    return usage_txt

def parse_opts():
    options, remainder = getopt.gnu_getopt(sys.argv[1:], 'chsri:o:b:p:m:d:t:',
                                           ['console', 'input', 'output', 'help', 'skip', 'baudrate', 'port', 'mxchip_file', 'device', 'timeout', 'reset'])
    # print('OPTIONS   :', options)

    for opt, arg in options:
        if opt in ('-h', '--help'):
            print(usage())
        elif opt in ('-i', '--input'):
            serial_settings.input_file = arg
        elif opt in ('-c', '--console'):
            serial_settings.console_mode = True
        elif opt in ('-o', '--output'):
            serial_settings.output_file = arg
        elif opt in ('-b', '--baudrate'):
            serial_settings.baud_rate = int(arg)
        elif opt in ('-p', '--port'):
            serial_settings.port = arg
        elif opt in ('-m', '--mxchip_file'):
            serial_settings.mxchip_file = "/media/newt/" + arg
        elif opt in ('-s', '--skip'):
            serial_settings.skip_setup = True
        elif opt in ('-d', '--device'):
            serial_settings.device_type = arg
        elif opt in ('-t', '--timeout'):
            serial_settings.test_timeout = int(arg)
        elif opt in ('-r', '--reset'):
            serial_settings.reset_device = True

def check_sdk_errors(line):
    if "ERROR:" in line:
        azure_test_firmware_errors.SDK_ERRORS += 1

def check_firmware_errors(line):
    if azure_test_firmware_errors.iot_init_failure in line:
        print("Failed to connect to saved IoT Hub!")
        azure_test_firmware_errors.SDK_ERRORS += 1

    elif azure_test_firmware_errors.sensor_init_failure in line:
        print("Failed to init mxchip sensor")
        azure_test_firmware_errors.SDK_ERRORS += 1

    elif azure_test_firmware_errors.wifi_failure in line:
        print("Failed to connect to saved WiFi network.")
        azure_test_firmware_errors.SDK_ERRORS += 1


def run():

    if serial_settings.console_mode:
        connection_handle = None
        if 'mxchip' in serial_settings.device_type: #'rpi' in serial_settings.device_type or 'raspi' in serial_settings.device_type:
            connection_handle = serial.Serial(port=serial_settings.port, baudrate=serial_settings.baud_rate)

            shelly = interactive_shell.interactive_shell()
            shelly.setup(mxchip_uart_interface.mxchip_uart_interface(), connection_handle)
            shelly.cmdloop()
            connection_handle.close()

        elif 'rpi' in serial_settings.device_type or 'raspi' in serial_settings.device_type:
            connection_handle = serial.Serial(port=serial_settings.port, baudrate=serial_settings.baud_rate)

            shelly = interactive_shell.interactive_shell()
            shelly.setup(rpi_uart_interface.rpi_uart_interface(), connection_handle)
            shelly.cmdloop()
            connection_handle.close()

        elif 'linux' in serial_settings.device_type or 'win' in serial_settings.device_type or 'windows' in serial_settings.device_type:
            connection_handle = subprocess.Popen('ls', stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE)

            shelly = interactive_shell.interactive_shell()
            shelly.setup(lin_ipc_interface.lin_ipc_interface(), connection_handle)
            shelly.cmdloop()

        print("Num of Errors: %d" % azure_test_firmware_errors.SDK_ERRORS)
        sys.exit(azure_test_firmware_errors.SDK_ERRORS)
        return

    # This is where we will branch between different interfaces
    if 'mxchip' in serial_settings.device_type:
        # Initial wait for mxchip to be flashed
        mxchip_uart_interface.device_setup()

    if 'mxchip' in serial_settings.device_type or 'rpi' in serial_settings.device_type or 'raspi' in serial_settings.device_type:
        # If using a serial interface
        ser = serial.Serial(port=serial_settings.port, baudrate=serial_settings.baud_rate)

        time.sleep(.5)

        # Print initial message
        output = ser.readline(ser.in_waiting)
        output = output.strip().decode(encoding='utf-8', errors='ignore')
        print(output)

        if serial_settings.skip_setup:
            # skip waiting for WiFi and IoT Hub setup
            while (ser.in_waiting):
                time.sleep(.1)
                output = ser.readline(ser.in_waiting)
                output = output.strip().decode(encoding='utf-8', errors='ignore')

                mxchip_uart_interface.check_firmware_errors(output)
                # Do we want to save this output to file if there is an error printed?
                if len(output) > 4:
                    print(output)
        else:

            # Wait for WiFi and IoT Hub setup to complete
            start_time = time.time()

            # for mxchip, branch this away
            while(serial_settings.setup_string not in output) and ((time.time() - start_time) < (3*serial_settings.wait_for_flash + 5)):
                time.sleep(.1)
                output = ser.readline(ser.in_waiting)
                output = output.strip().decode(encoding='utf-8', errors='ignore')

                mxchip_uart_interface.check_firmware_errors(output)
                if len(output) > 4:
                    print(output)


        if 'rpi' in serial_settings.device_type or 'raspi' in serial_settings.device_type:
            uart = rpi_uart_interface.rpi_uart_interface()
        else:
            uart = mxchip_uart_interface.mxchip_uart_interface()


        uart.write_read(ser, serial_settings.input_file, serial_settings.output_file)

        ser.reset_input_buffer()
        ser.reset_output_buffer()
        ser.close()

    elif 'windows' in serial_settings.device_type or 'win' in serial_settings.device_type or 'linux' in serial_settings.device_type:
        interface = lin_ipc_interface.lin_ipc_interface()
        interface.ipc_process = subprocess.Popen('ls', stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        interface.write_read(serial_settings.input_file, serial_settings.output_file)

    print("Num of Errors: %d" %azure_test_firmware_errors.SDK_ERRORS)
    sys.exit(azure_test_firmware_errors.SDK_ERRORS)

if __name__ == '__main__':
    parse_opts()
    run()
