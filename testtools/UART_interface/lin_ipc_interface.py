import os
import sys
import time
import re
from subprocess import PIPE, Popen

try:
    import testtools.UART_interface.azure_test_firmware_errors as azure_test_firmware_errors
    import testtools.UART_interface.serial_settings as serial_settings

except:
    import azure_test_firmware_errors
    import serial_settings


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
        serial_settings.tests_run = True
        return False
    return True

# def check_firmware_errors(line):
#     if azure_test_firmware_errors.iot_init_failure in line:
#         print("Failed to connect to saved IoT Hub!")
#         azure_test_firmware_errors.SDK_ERRORS += 1
#
#     elif azure_test_firmware_errors.sensor_init_failure in line:
#         print("Failed to init mxchip sensor")
#         azure_test_firmware_errors.SDK_ERRORS += 1
#
#     elif azure_test_firmware_errors.wifi_failure in line:
#         print("Failed to connect to saved WiFi network.")
#         azure_test_firmware_errors.SDK_ERRORS += 1

def make_int(s):
    s = s.strip()
    j = re.search(r'\d+', s)
    if j:
        return int(j.group())
    return 0

# ------- interface class -------
class lin_ipc_interface():
    ipc_process = None

    # so here's the problem,
    def wait(self):
        pass

    def write(self, ser, message, file=None):
        os.system(message)
        return True
        # return self.subprocess_write(ser, message, file)
    # method to write to subprocess with connection monitoring
    def subprocess_write(self, ser, message, file=None):

        try:
            if not ser.poll(): # previous process has yet to return
                output = ser.communicate(timeout=(2*serial_settings.wait_for_flash))
                self.read_all_helper(output, message, file)
        except:
            pass

        if message.split():
            # handling for piping
            if "|" in message:
                message_parts = message.split('|')
            else:
                message_parts = []
                message_parts.append(message)
            i = 0
            p = {}
            for message_part in message_parts:
                message_part = message_part.strip()
                if i == 0:
                    p[i] = Popen(message_part.split(), stdin=None, stdout=PIPE, stderr=PIPE)
                else:
                    p[i] = Popen(message_part.split(), stdin=p[i - 1].stdout, stdout=PIPE, stderr=PIPE)
                i = i + 1
            # end handling for piping
            ser = p[i - 1]
            output = ser.communicate(timeout=(2*serial_settings.wait_for_flash))

            return output
        else:
            return (b'', b'')

    def read(self, ser, message, file):
        return None
        # return self.subprocess_read(ser, message, file)
    # Read from saved subprocess output
    def subprocess_read(self, ser, message, file=None):

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

        # if not output:
        output = ser.readline()
        output = output.decode(encoding='utf-8', errors='ignore')
        check_sdk_errors(output)
        check_test_failures(output)
        print(output)
        try:
            # File must exist to write to it
            file.writelines(output)
        except:
            pass
        # else:
        #     printo = output[0].decode(encoding='utf-8', errors='ignore').split('\n')
        #     for line in printo:
        #         check_sdk_errors(line)
        #         check_test_failures(line)
        #         print(line)
        #         try:
        #             # File must exist to write to it
        #             file.writelines(line)
        #         except:
        #             pass

        return output

    def read_all_helper(self, output, message, file):
        err_string = output[1].decode(encoding='utf-8', errors='ignore')
        print(err_string)
        azure_test_firmware_errors.SDK_ERRORS += make_int(err_string)
        printo = output[0].decode(encoding='utf-8', errors='ignore').split('\n')
        for line in printo:
            check_sdk_errors(line)
            check_test_failures(line)
            print(line)
            try:
                # File must exist to write to it
                file.writelines(line)
            except:
                pass


    def write_read(self, input_file, output_file):
        session_start = time.time()
        if input_file:
            # # set wait between read/write
            # wait = (serial_settings.bits_to_cache/serial_settings.baud_rate)
            with open(input_file) as fp:
                # initialize input/output_file
                line = fp.readline()
                f = open(output_file, 'w+') # Output file
                while line:
                    if line.split():
                        print("Sending %s" %line.split()[0])

                    # Attempt to write to subprocess
                    output = self.subprocess_write(self.ipc_process, line, f)
                    if not output:
                        print("Failed to write to subprocess, please diagnose memory space.")
                        f.close()
                        break

                    # Attempt to read stdout
                    azure_test_firmware_errors.SDK_ERRORS += make_int(output[1].decode(encoding='utf-8', errors='ignore'))
                    # try:
                    #     if not self.ipc_process.poll():
                    #         output = self.ipc_process.communicate(timeout=(2 * serial_settings.wait_for_flash))
                    # except:
                    #     print("ipc_process not initialized.")
                    #     raise ValueError
                    self.read_all_helper(output, line, f)
                    # while output[0]:
                    #     self.subprocess_read(output[0], line, f)

                    line = fp.readline()

                # # read any trailing output, save to file
                # if serial_settings.test_timeout:
                #     while((time.time() - session_start) < serial_settings.test_timeout):
                #         time.sleep(.2)
                #         output = self.subprocess_read(ser, line, f)
                #         output = output.decode(encoding='utf-8', errors='ignore')
                #         check_test_failures(output)
                #
                #         if len(output) > 4:
                #             print(output)
                #
                #         #for now we can assume one test suite is run
                #         if " tests run" in output or serial_settings.tests_run:
                #             break

                # else:
                #     while (ser.in_waiting):
                #         time.sleep(.2)
                #         output = self.subprocess_read(ser, line, f)
                #         check_sdk_errors(output)

                f.close()
