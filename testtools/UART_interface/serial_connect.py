import serial
import os
import sys
import getopt
import time
try:
    import testtools.UART_interface.serial_settings as serial_settings
except:
    import serial_settings


# Note: commands on MXCHIP have line endings with \r AND \n
# Oz request: Make this multidevice - buh how? Guess we gots ta figure out
# Notes: This is designed to be used as a command line script with args (for automation purposes) to communicate over serial to a Microsoft mxchip device.

def parse_opts():
    options, remainder = getopt.gnu_getopt(sys.argv[1:], 'hi:o:b:p:', ['input', 'output', 'help', 'baudrate', 'port'])
    # print('OPTIONS   :', options)

    for opt, arg in options:
        if opt in ('-h', '--help'):
            print('I need somebody')
        elif opt in ('-i', '--input'):
            # print('Setting --file.')
            serial_settings.input_file = arg
        elif opt in ('-o', '--output'):
            # print('Setting --file.')
            serial_settings.output_file = arg
        elif opt in ('-b', '--baudrate'):
            # print('Setting --baudrate.')
            serial_settings.baud_rate = int(arg)
        elif opt in ('-p', '--port'):
            serial_settings.port = arg


# If there is a sudden disconnect, program should report line in input script reached, and close files.

# method to write to serial line with connection monitoring
def serial_write(ser, message, file=None):

    # Check that the device is no longer sending bytes
    if ser.in_waiting:
        serial_read(ser, message, file)

    # Check that the serial connection is open
    if ser.writable():
        bytes_written = ser.write(bytearray((message.strip()+'\r\n').encode('ascii')))
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
    if "send_telemetry" in message:
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
        print(output.decode(encoding='utf-8'))
        try:
            # File must exist to write to it
            file.writelines(output.decode(encoding='utf-8'))
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


def write_read(ser, input_file, output_file):
    if input_file:
        # set wait between read/write
        wait = (serial_settings.bits_to_cache/serial_settings.baud_rate)
        with open(input_file) as fp:
            # initialize input/output_file
            line = fp.readline()
            f = open(output_file, 'w+') # Output file
            while line:
                time.sleep(.1)
                print("Sending %s" %line)

                # Attempt to write to serial port
                if not serial_write(ser, line, f):
                    print("burger king")
                    f.close()
                    break
                time.sleep(wait)

                # Attempt to read serial port
                output = serial_read(ser, line, f, first_read=True)

                while(output):
                    time.sleep(wait)
                    output = serial_read(ser, line, f)
                line = fp.readline()

            f.close()

def run():
    if 'posix' in os.name:
        # enable read write access on port
        # os.chmod(serial_settings.port, 666)

        # check to see that mxchip firmware drop path resolves
        for x in range(serial_settings.wait_for_flash):
            exists = os.path.exists('/media/newt/AZ31661')
            if exists:
                break
            time.sleep(1)
        if x >= (serial_settings.wait_for_flash-1):
            print("MXCHIP drop path did not resolve, chip did not flash successfully.")
            raise FileExistsError

    print(serial_settings.baud_rate)
    ser = serial.Serial(port=serial_settings.port, baudrate=serial_settings.baud_rate)

    # print(ser.writable())
    # print(ser.readable())
    time.sleep(.5)
    # Print initial message
    output = ser.readline(ser.in_waiting)
    print(output.strip().decode(encoding='utf-8'))
    while(output):
        output = ser.readline(ser.in_waiting)
        print(output.strip().decode(encoding='utf-8'))

    write_read(ser, serial_settings.input_file, serial_settings.output_file)

    while(ser.in_waiting):
        time.sleep(.2)
        output = ser.readline(ser.in_waiting)
        print(output.strip().decode(encoding='utf-8'))
        # output = input("howzit")
        # serial_write(ser, output)
    ser.close()

if __name__ == '__main__':
    parse_opts()
    run()
