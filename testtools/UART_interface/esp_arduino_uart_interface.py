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
    local_line = line.lower()
    if "error" in local_line or "fail" in local_line:
        if "epoch time failed!" in local_line: # don't count NTP retries.
            azure_test_firmware_errors.SDK_ERRORS += 0
        else:
            azure_test_firmware_errors.SDK_ERRORS += 1

# missing test_failures method, may not need due to this being a sample,
# not a series of tests

def check_firmware_errors(line):
    if azure_test_firmware_errors.iot_init_failure in line:
        print("Failed to connect to saved IoT Hub!")
        azure_test_firmware_errors.SDK_ERRORS += 1

    elif azure_test_firmware_errors.wifi_failure in line:
        print("Failed to connect to saved WiFi network.")
        azure_test_firmware_errors.SDK_ERRORS += 1


# ------- interface class -------
class esp_uart_interface(uart_interface):

    message_callbacks = 0
    messages_sent = 5

    def check_sample_errors(self, line):
        if "Confirmation callback" in line:
            self.message_callbacks += 1

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
                time.sleep(wait)
                if (time.time() - timeout > serial_settings.serial_comm_timeout):
                    break

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

        # Special per opt handling:
        if "exit" in message and first_read:
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
            self.check_sample_errors(output)
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
                return None

    def write_read(self, ser, input_file, output_file):
        if 'esp32' in serial_settings.device_type:
            serial_settings.bits_to_cache = 800
            serial_settings.baud_rate = 1000000
        else:
            serial_settings.bits_to_cache = 1600
            serial_settings.baud_rate = 115200

        if input_file:
            # set wait between read/write
            wait = (serial_settings.bits_to_cache/serial_settings.baud_rate)
            with open(input_file) as input_file_obj:
                # initialize input/output_file
                line = input_file_obj.readline()
                output_file_obj = open(output_file, 'w+') # Output file
                while line:
                    # Print only instruction, not secret content
                    if line.split():
                        print("Sending %s" %line.split()[0])

                    # Attempt to write to serial port
                    if not self.serial_write(ser, line, output_file_obj):
                        print("Failed to write to serial port, please diagnose connection.")
                        output_file_obj.close()
                        break
                    time.sleep(1)

                    # Attempt to read serial port
                    output = self.serial_read(ser, line, output_file_obj, first_read=True)

                    while(output):
                        time.sleep(wait)
                        output = self.serial_read(ser, line, output_file_obj)
                    line = input_file_obj.readline()

                # read any trailing output, save to file
                while (ser.in_waiting):
                    time.sleep(.2)
                    output = self.serial_read(ser, line, output_file_obj)

                # forward failed callbacks to SDK_ERRORS
                azure_test_firmware_errors.SDK_ERRORS += self.messages_sent - self.message_callbacks
                with open('exitcode.txt', 'w') as fexit:
                    fexit.write('%d' %azure_test_firmware_errors.SDK_ERRORS)

                output_file_obj.close()