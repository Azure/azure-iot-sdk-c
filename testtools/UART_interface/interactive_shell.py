import time
import cmd

try:
    import testtools.UART_interface.azure_test_firmware_errors as azure_test_firmware_errors
    import testtools.UART_interface.serial_settings as serial_settings
    import testtools.UART_interface.serial_commands_dict as commands_dict
    import testtools.UART_interface.rpi_uart_interface
    import testtools.UART_interface.mxchip_uart_interface
    import testtools.UART_interface.lin_ipc_interface
except:
    import azure_test_firmware_errors
    import serial_settings
    import serial_commands_dict as commands_dict
    import rpi_uart_interface
    import mxchip_uart_interface
    import lin_ipc_interface

class interactive_shell(cmd.Cmd):
    intro = "Welcome to the interactive version of the Azure C SDK\'s device test cli tool!\nType bye or close to exit."
    interface = None
    # prompt = ''
    connection_handle = None
    output_file = serial_settings.output_file

    # command executed before cmdloop is started
    def preloop(self):
        try:
            self.interface.device_setup()
        except:
            pass

    # command executed when cmdloop is ended
    def postloop(self):
        try:
            self.interface.close()
        except:
            print('Please create a close method for this interface.')
            pass
    def do_help(self, arg):
        self.default('help')

    def emptyline(self):
        # pass
        self.interface.write(self.connection_handle, "\n", self.output_file)

    def setup(self, interface, connection_handle):
        self.interface = interface
        self.connection_handle = connection_handle

    def default(self, line):
        # send command to appropriate device
        if not self.interface.write(self.connection_handle, line, self.output_file):
            print("Failed to write to serial port, please diagnose connection.")

    def postcmd(self, stop, line):
        if stop:
            return True
        output = self.interface.read(self.connection_handle, line, self.output_file)

        cont = not bool(output)
        begin = time.time()
        while cont:
            print('trying')
            self.interface.wait()
            output = self.interface.read(self.connection_handle, line, self.output_file)
            cont = not bool(output)

            if (time.time() - begin) > 2:
                break

        while(output):
            # print('printing')
            self.interface.wait()
            output = self.interface.read(self.connection_handle, line, self.output_file)
        # print("failed to read")


    def do_bye(self, arg):
        # 'Stop recording, close the shell, and exit:  BYE'
        print('Thank you for using shell')
        return True

    def do_close(self, arg):
        # 'Stop recording, close the shell, and exit:  BYE'
        print('Thank you for using shell')
        return True
