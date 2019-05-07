import serial
import os
import sys
import getopt
import time
import serial_settings

# Note: commands are taken in with \r AND \n
# Oz request: Make this multidevice - buh how? Guess we gots ta figure out
# Notes: This is designed to be used as a command line script with args (for automation purposes) to communicate over serial to a Microsoft mxchip device.
# In order to accomplish this, we will need to learn how to pull in kwargs, such as the serial port, the baud rate, and any other info relevent to the test or run
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
            serial_settings.baud_rate = arg
        elif opt in ('-p', '--port'):
            serial_settings.port = arg

def write_read(ser, input_file, output_file):
    if input_file:
        with open(input_file) as fp:
            line = fp.readline()
            f = open(output_file, 'w+')
            while line:
                time.sleep(.1)
                print(line)
                ser.write(bytearray((line.strip()+'\r\n').encode('ascii')))
                time.sleep(.01)
                output = ser.readline(ser.in_waiting)
                print(output.decode(encoding='utf-8'))
                f.writelines(output.decode(encoding='utf-8'))
                while(output):
                    output = ser.readline(ser.in_waiting)
                    print(output.decode(encoding='utf-8'))
                    f.writelines(output.decode(encoding='utf-8'))
                line = fp.readline()

            f.close()

def run():
    if 'posix' in os.name:
        # enable read write access on port
        # os.chmod(serial_settings.port, 666)
        pass
    print(serial_settings.baud_rate)
    ser = serial.Serial(port=serial_settings.port, baudrate=serial_settings.baud_rate)

    # print(ser.writable())
    # print(ser.readable())
    time.sleep(.5)
    # Print initial message
    output = ser.readline(ser.in_waiting)
    print(output.strip())
    while(output):
        output = ser.readline(ser.in_waiting)
        print(output.strip())
        print('task')


    write_read(ser, serial_settings.input_file, serial_settings.output_file)

    ser.close()

if __name__ == '__main__':
    parse_opts()
    run()