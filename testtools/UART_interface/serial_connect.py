import serial
import os
import sys
import getopt
import time
try:
    import testtools.UART_interface.azure_test_firmware_errors as azure_test_firmware_errors
    import testtools.UART_interface.serial_settings as serial_settings
    import testtools.UART_interface.serial_commands_dict as commands_dict
except:
    import azure_test_firmware_errors
    import serial_settings
    import serial_commands_dict as commands_dict

# Note: commands on MXCHIP have line endings with \r AND \n
# Notes: This is designed to be used as a command line script with args (for automation purposes) to communicate over serial to a Microsoft mxchip device.

# Iterates through command dictionary to print out script's opt usage
def usage():
    usage_txt = "serial_connect.py usage: \r\n"
    for commands in commands_dict.cmds:
        usage_txt += " - %s: " %commands + commands_dict.cmds[commands]['text'] + "\r\n"

    return usage_txt


def parse_opts():
    options, remainder = getopt.gnu_getopt(sys.argv[1:], 'hsi:o:b:p:m:', ['input', 'output', 'help', 'skip', 'baudrate', 'port', 'mxchip_file'])
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


# If there is a sudden disconnect, program should report line in input script reached, and close files.
# method to write to serial line with connection monitoring
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

        return output
    else:
        try:
            time.sleep(2)
            ser.open()
            serial_read(ser, message, file, first_read=True)
        except:
            return False


# Note: the buffer size on the mxchip appears to be 128 Bytes.
def write_read(ser, input_file, output_file):
    if input_file:
        # set wait between read/write
        wait = (serial_settings.bits_to_cache/serial_settings.baud_rate)
        with open(input_file) as fp:
            # initialize input/output_file
            line = fp.readline()
            f = open(output_file, 'w+') # Output file
            while line:
                # time.sleep(.1)
                # Print only instruction, not secret content
                if line.split():
                    print("Sending %s" %line.split()[0])

                # Attempt to write to serial port
                if not serial_write(ser, line, f):
                    print("Failed to write to serial port, please diagnose connection.")
                    f.close()
                    break
                time.sleep(1)

                # Attempt to read serial port
                output = serial_read(ser, line, f, first_read=True)

                while(output):
                    time.sleep(wait)
                    output = serial_read(ser, line, f)
                line = fp.readline()

            # read any trailing output, save to file
            while (ser.in_waiting):
                time.sleep(.2)
                output = serial_read(ser, line, f)

            f.close()

def run():
    if 'posix' in os.name:
        # check to see that mxchip firmware drop path resolves
        for x in range(serial_settings.wait_for_flash):
            exists = os.path.exists(serial_settings.mxchip_file)
            if exists:
                break
            time.sleep(1)
        if x >= (serial_settings.wait_for_flash-1):
            # firmware flash path did not resolve
            print("MXCHIP drop path did not resolve, chip did not flash successfully.")
            raise FileExistsError

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
            check_firmware_errors(output)
            # Do we want to save this output to file if there is an error printed?
            if len(output) > 4:
                print(output)
    else:
        # Wait for WiFi and IoT Hub setup to complete
        start_time = time.time()

        while(serial_settings.setup_string not in output) and ((time.time() - start_time) < (3*serial_settings.wait_for_flash + 5)):
            time.sleep(.1)
            output = ser.readline(ser.in_waiting)
            output = output.strip().decode(encoding='utf-8', errors='ignore')
            check_firmware_errors(output)
            if len(output) > 4:
                print(output)

    write_read(ser, serial_settings.input_file, serial_settings.output_file)

    ser.reset_input_buffer()
    ser.reset_output_buffer()
    ser.close()

    print("Num of Errors: %d" %azure_test_firmware_errors.SDK_ERRORS)
    sys.exit(azure_test_firmware_errors.SDK_ERRORS)

if __name__ == '__main__':
    parse_opts()
    run()
