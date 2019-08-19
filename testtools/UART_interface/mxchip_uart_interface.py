import os
import sys
import time
try:
    from testtools.UART_interface.base_uart_interface import uart_interface
except:
    from base_uart_interface import uart_interface

try:
    import testtools.UART_interface.azure_test_firmware_errors as azure_test_firmware_errors
    import testtools.UART_interface.serial_settings as serial_settings
    import testtools.UART_interface.serial_commands_dict as commands_dict
except:
    import azure_test_firmware_errors
    import serial_settings
    import serial_commands_dict as commands_dict


# ------- global usecase fcns -------
def device_setup():
    if 'posix' in os.name:
        # check to see that mxchip firmware drop path resolves
        for x in range(serial_settings.wait_for_flash):
            exists = os.path.exists(serial_settings.mxchip_file)
            if exists:
                break
            time.sleep(1)
        if x >= (serial_settings.wait_for_flash - 1):
            # firmware flash path did not resolve
            print("MXCHIP drop path did not resolve, chip did not flash successfully.")
            raise FileExistsError

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


# ------- interface class -------
class mxchip_uart_interface(uart_interface):

    # If there is a sudden disconnect, program should report line in input script reached, and close files.
    # method to write to serial line with connection monitoring
    def serial_write(self, ser, message, file=None):

        # Check that the device is no longer sending bytes
        if ser.in_waiting:
            self.serial_read(ser, message, file)

        # Check that the serial connection is open
        if ser.writable():
            wait = serial_settings.mxchip_buf_pause  # wait for at least 50ms between 128 byte writes.
            buf = bytearray((message.strip() + '\r\n').encode('ascii'))
            buf_len = len(buf)  # needed for loop as buf is a destructed list
            bytes_written = 0
            timeout = time.time()

            while bytes_written < buf_len:
                temp_written = ser.write(buf[:128])
                buf = buf[temp_written:]
                bytes_written += temp_written
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
                self.serial_write(ser, message, file)
            except:
                return False

    # Read from serial line with connection monitoring
    # If there is a sudden disconnect, program should report line in input script reached, and close files.
    def serial_read(self, ser, message, file, first_read=False):

        # Special per opt handling:
        if "send_telemetry" in message or "set_az_iothub" in message:
            time.sleep(.15)
        elif "exit" in message and first_read:
            time.sleep(serial_settings.wait_for_flash)
            output = ser.in_waiting
            while output < 4:
                time.sleep(1)
                output = ser.in_waiting
                print("%d bytes in waiting" % output)

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
                self.serial_read(ser, message, file, first_read=True)
            except:
                return False

    # Note: the buffer size on the mxchip appears to be 128 Bytes.
    def write_read(self, ser, input_file, output_file):
        serial_settings.bits_to_cache = 1600
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

                f.close()
