import os
import sys
import time

try:
    from testtools.UART_interface.base_uart_interface import uart_interface
    import testtools.UART_interface.azure_test_firmware_errors as azure_test_firmware_errors
    import testtools.UART_interface.serial_settings as serial_settings
    import testtools.UART_interface.serial_commands_dict as commands_dict
except:
    import azure_test_firmware_errors
    import serial_settings
    import serial_commands_dict as commands_dict
    from base_uart_interface import uart_interface


# ------- global usecase fcns -------
def device_setup():
    pass

def check_sdk_errors(line):
    if "Error:" in line:
        azure_test_firmware_errors.SDK_ERRORS += 1

def check_test_failures(line):
    if " tests ran" in line:
        result = [int(s) for s in line.split() if s.isdigit()]
        azure_test_firmware_errors.SDK_ERRORS = result[1]
        return result


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

# ------- interface class -------
class rpi_uart_interface(uart_interface):

    # If there is a sudden disconnect, program should report line in input script reached, and close files.
    # method to write to serial line with connection monitoring
    def serial_write(self, ser, message, file=None):

        # Check that the device is no longer sending bytes
        if ser.in_waiting:
            self.serial_read(ser, message, file)

        # Check that the serial connection is open
        if ser.writable():

            # special handling for sending a file
            if "sz " in message:
                round = ser.write(bytearray("rz\r\n".encode('ascii'))) #
                os.system(message)#"sz -a test.sh > /dev/ttyUSB0 < /dev/ttyUSB0")

                # check for completion
                output = ser.readline(ser.in_waiting)
                output = output.decode(encoding='utf-8', errors='ignore')
                # print(output)
                start_time = time.time()
                while ("Transfer" not in output) and (time.time() - start_time) < serial_settings.wait_for_flash:
                    time.sleep(.01)
                    output = ser.readline(ser.in_waiting)
                    output = output.decode(encoding='utf-8', errors='ignore')

                return round
            # special handling for receiving a file, Note: input file should use rz <filepath> when a file from the RPi is desired
            elif "rz " in message:
                os.system('rz')
                message.replace('rz ', 'sz ')

            # wait = serial_settings.mxchip_buf_pause # wait for at least 50ms between 128 byte writes. -- may not be necessary on rpi
            buf = bytearray((message).encode('ascii'))
            print(buf)
            # buf_len = len(buf)  # needed for loop as buf is a destructed list
            # bytes_written = 0
            bytes_written = ser.write(buf)
            # timeout = time.time()
            # while bytes_written < buf_len:
            #     round = ser.write(buf[:128])
            #     buf = buf[round:]
            #     bytes_written += round
            #     # print("bytes written: %d" %bytes_written)
            #     time.sleep(wait)
            #     if (time.time() - timeout > serial_settings.serial_comm_timeout):
            #         break
            # print("final written: %d" %bytes_written)
            return bytes_written
        else:
            try:
                time.sleep(2)
                ser.open()
                self.serial_write(ser, message, file)
            except:
                return False

    # Read from serial line with connection monitoring
    # If there is a sudden disconnect, program should report line in input script reached, and close files.
    def serial_read(self, ser, message, file, first_read=False):

        # # Special per opt handling:
        # if "send_telemetry" in message or "set_az_iothub" in message:
        #     time.sleep(.15)
        # elif "exit" in message and first_read:
        #     time.sleep(serial_settings.wait_for_flash)
        #     output = ser.in_waiting
        #     while output < 4:
        #         time.sleep(1)
        #         output = ser.in_waiting
        #         print("%d bytes in waiting" % output)
        # may want special opt handling for raspi commands

        if ser.readable():
            output = ser.readline(ser.in_waiting)
            output = output.decode(encoding='utf-8', errors='ignore')

            # check_sdk_errors(output)
            check_test_failures(output)
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
                self.serial_read(ser, message, file, first_read=True)
            except:
                return False

    # Note: the buffer size on the mxchip appears to be 128 Bytes.
    def write_read(self, ser, input_file, output_file):
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
                    if not self.serial_write(ser, line, f):
                        print("Failed to write to serial port, please diagnose connection.")
                        f.close()
                        break
                    time.sleep(1)

                    # Attempt to read serial port
                    output = self.serial_read(ser, line, f, first_read=True)

                    while(output):
                        time.sleep(wait)
                        output = self.serial_read(ser, line, f)
                    line = fp.readline()

                # read any trailing output, save to file
                while (ser.in_waiting):
                    time.sleep(.2)
                    output = self.serial_read(ser, line, f)
                    output = output.decode(encoding='utf-8', errors='ignore')
                    check_test_failures(output)
                    print(output)

                f.close()
