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
        print("line is: " + line)
        print("result is: " + str(result))
        if len(result) == 2:
            azure_test_firmware_errors.SDK_ERRORS = result[0]
        else:
            azure_test_firmware_errors.SDK_ERRORS = result[1]        
        
        serial_settings.tests_run = True
        return False

    return True


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
                temp_written = ser.write(bytearray("rz\r\n".encode('ascii')))
                os.system(message)

                # check for completion
                output = ser.readline(ser.in_waiting)
                output = output.decode(encoding='utf-8', errors='ignore')
                # print(output)
                start_time = time.time()
                while ("Transfer" not in output) and (time.time() - start_time) < serial_settings.wait_for_flash:
                    time.sleep(.01)
                    output = ser.readline(ser.in_waiting)
                    output = output.decode(encoding='utf-8', errors='ignore')
                    if "incomplete" in output:
                        serial_settings.tests_run = True
                        azure_test_firmware_errors.SDK_ERRORS += 1
                        return 0
                if "incomplete" in output:
                    serial_settings.tests_run = True
                    azure_test_firmware_errors.SDK_ERRORS += 1
                    return 0

                return temp_written
            # special handling for receiving a file, Note: input file should use rz <filepath> when a file from the RPi is desired
            elif "rz " in message:
                os.system('rz')
                message.replace('rz ', 'sz ')

            buf = bytearray((message).encode('ascii'))
            print(buf)
            bytes_written = ser.write(buf)

            return bytes_written
        else:
            try:
                time.sleep(2)
                ser.open()
                self.serial_write(ser, message, file)
            except:
                return 0

    # Read from serial line with connection monitoring
    # If there is a sudden disconnect, program should report line in input script reached, and close files.
    def serial_read(self, ser, message, file, first_read=False):

        if ser.readable():
            output = ser.readline(ser.in_waiting)
            output = output.decode(encoding='utf-8', errors='ignore').strip()

            check_sdk_errors(output)
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
                #This exists purely if the serial line becomes unreadable during operation, will likely never be hit, if so, check your cable
                time.sleep(2)
                ser.open()
                self.serial_read(ser, message, file, first_read=True)
            except:
                return False

    # Note: the buffer size on the mxchip appears to be 128 Bytes.
    def write_read(self, ser, input_file, output_file):
        session_start = time.time()
        # set bits to cache variable to allow for mxchip to pass bytes into serial buffer
        serial_settings.bits_to_cache = 800
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
                if serial_settings.test_timeout:
                    print("Test input end. Waiting for results. Time Elapsed: ", (time.time() - session_start))
                    while((time.time() - session_start) < serial_settings.test_timeout):
                        time.sleep(0)
                        output = ser.readline(ser.in_waiting)
                        output = output.decode(encoding='utf-8', errors='ignore')
                        check_test_failures(output)

                        if len(output) > 4:
                            print(output)

                        #for now we can assume one test suite is run
                        if " tests run" in output or serial_settings.tests_run:
                            break
                    print("Test iteration ended. Time Elapsed: ", (time.time() - session_start))

                else:
                    while (ser.in_waiting):
                        time.sleep(.2)
                        output = self.serial_read(ser, line, f)
                        check_sdk_errors(output)

                # reset after every test if setting
                if serial_settings.reset_device:
                    ser.write(bytearray("sudo reboot\n".encode("ascii")))

                f.close()
