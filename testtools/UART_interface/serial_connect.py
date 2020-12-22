import serial
import os
import sys
import getopt
import time
try:
    import testtools.UART_interface.azure_test_firmware_errors as azure_test_firmware_errors
    import testtools.UART_interface.serial_settings as serial_settings
    import testtools.UART_interface.serial_commands_dict as commands_dict
    import testtools.UART_interface.rpi_uart_interface
    import testtools.UART_interface.mxchip_uart_interface
    import testtools.UART_interface.esp_arduino_uart_interface
except:
    import azure_test_firmware_errors
    import serial_settings
    import serial_commands_dict as commands_dict
    import rpi_uart_interface
    import mxchip_uart_interface
    import esp_arduino_uart_interface

# Note: This collection of scripts is designed to be used as a command line script with args (for automation purposes)
# to communicate over serial to Azure IoT C SDK supported devices.

uart = None

def usage():
    # Iterates through command dictionary to print out script's opt usage
    usage_txt = "serial_connect.py usage: \r\n"
    for commands in commands_dict.cmds:
        usage_txt += " - %s: " %commands + commands_dict.cmds[commands]['text'] + "\r\n"
    return usage_txt

def parse_opts():
    options, remainder = getopt.gnu_getopt(sys.argv[1:], 'hsri:o:b:p:m:d:t:', ['input', 'output', 'help', 'skip', 'baudrate', 'port', 'mxchip_file', 'device', 'timeout', 'reset'])
    # print('OPTIONS   :', options)

    for opt, arg in options:
        if opt in ('-h', '--help'):
            print(usage())
        elif opt in ('-i', '--input'):
            serial_settings.input_file = arg
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


# Method to write to serial line with connection monitoring
def serial_write(ser, message, file=None):

    # Check that the device is no longer sending bytes
    if ser.in_waiting:
        serial_read(ser, message, file)

    # Check that the serial connection is open
    if ser.writable():
        wait = serial_settings.mxchip_buf_pause # wait for at least 50ms between 128 byte writes.
        buf = bytearray((message.strip()+'\r\n').encode('ascii'))
        buf_len = len(buf) # needed for loop as buf is a destructed list
        bytes_written = 0
        timeout = time.time()

        while bytes_written < buf_len:
            round = ser.write(buf[:128])
            buf = buf[round:]
            bytes_written += round
            # print("bytes written: %d" %bytes_written)
            time.sleep(wait)
            if (time.time() - timeout > serial_settings.serial_comm_timeout):
                break
        # print("final written: %d" %bytes_written)
        return bytes_written
    else:
        try:
            time.sleep(2)
            ser.open()
            serial_write(ser, message, file)
        except:
            return False


# Read from serial line with connection monitoring
# If there is a sudden disconnect, program should report line in input script reached, and close files.
def serial_read(ser, message, file, first_read=False):

    # Special per opt handling:
    if "send_telemetry" in message or "set_az_iothub" in message:
        time.sleep(.15)
    elif "exit" in message and first_read:
        time.sleep(serial_settings.wait_for_flash)
        output = ser.in_waiting
        while output < 4:
            time.sleep(1)
            output = ser.in_waiting
            print("%d bytes in waiting" %output)

    if ser.readable():
        output = ser.readline(ser.in_waiting)
        output = output.decode(encoding='utf-8', errors='ignore')

        check_firmware_errors(output)
        check_sdk_errors(output)
        print(output)
        try:
            # File must exist to write to it
            file.writelines(output)
        except:
            pass

def run():

    if 'mxchip' in serial_settings.device_type:
        mxchip_uart_interface.device_setup()

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
        # for rpi, transfer pipeline/artifact files from agent to device
        # get filepath, walk it
        # for each file, send rz, then sz file to rpi port
        uart = rpi_uart_interface.rpi_uart_interface()
    elif 'mxchip' in serial_settings.device_type:
        uart = mxchip_uart_interface.mxchip_uart_interface()
    elif 'esp32' in serial_settings.device_type or 'esp8266' in serial_settings.device_type:
        uart = esp_arduino_uart_interface.esp_uart_interface()


    uart.write_read(ser, serial_settings.input_file, serial_settings.output_file)

    ser.reset_input_buffer()
    ser.reset_output_buffer()
    ser.close()

    print("Num of Errors: %d" %azure_test_firmware_errors.SDK_ERRORS)
    sys.exit(azure_test_firmware_errors.SDK_ERRORS)

if __name__ == '__main__':
    parse_opts()
    run()
