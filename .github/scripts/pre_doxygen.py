#
# Summary:
#
# This script converts Azure IoT C SDK header files into more friendly format for doxygen.

#
# Usage:
#
# replace_mockable.py <input_directory> <output_directory>
# Pre-existing files in <output_directory> will be overwritten.

#
# Motivation and additional details
#
# The Azure IoT C SDK relies on macros to define public APIs and enumerations
# in ways that are not standard C99.  Pre-processors reconcile this during
# early stages of compilation.  Tools that do not have full pre-processors
# such as doxygen do not function well with macros as they are.

# Azure IoT C SDK public APIs are declared via the MOCKABLE_FUNCTION macro
# in  https://github.com/Azure/umock-c/blob/master/inc/umock_c/umock_c_prod.h.
# Public enumerations are declared via the macro utility framework declared in
# https://github.com/Azure/macro-utils-c/blob/master/inc/macro_utils/macro_utils.h.

# This tool does not attempt to build a full AST of the .h file, but relies instead
# on regexes and well defined patterns in order to perform the translation


import os
import sys
import re
import logging

logging.basicConfig(level=logging.INFO)

# Tracks relationship between enum values and when they're defined,
# which is split across arbitrary number of lines with the macro_utils.h framework.
mockable_enum_table = {}

# convert_mockable_function_line converts a MOCKABLE into a standard C99 function
# declaration.  For instance, this will change
#   MOCKABLE_FUNCTION(, IOTHUB_MESSAGE_HANDLE, IoTHubMessage_Clone, IOTHUB_MESSAGE_HANDLE, iotHubMessageHandle);
# into the parseable for doxygen
#   IOTHUB_MESSAGE_HANDLE  IoTHubMessage_Clone(IOTHUB_MESSAGE_HANDLE iotHubMessageHandle);
def convert_mockable_function_line(current_line):
    # Remove the training ");" from function declaration
    current_line = current_line.replace(");\n", "")

    # MOCKABLE_FUNCTIONS use ',' between both the type and arguments of a function.
    # We need to split on this level to get basic information about function
    # as well as combine the param_type and param_name
    tokens = current_line.split(',')

    # Ignore 0th element (the MOCKABLE_FUNCTION part) and then break out inital part of function
    return_type = tokens[1]
    function_name = tokens[2]
    del tokens[0:3]

    # Enumerate parameters to function except for the final one.
    function_arguments = ""
    for param_type, param_name in zip(*[iter(tokens[0:len(tokens)-2])]*2):
        function_arguments += (param_type  + param_name + ",")
    del tokens[0:len(tokens)-2]

    # Special case final parameter (which doesn't need a "," closing)
    for param_type, param_name in zip(*[iter(tokens)]*2):
        function_arguments += (param_type +  param_name)
    
    return return_type + function_name + "(" + function_arguments + ");"

# When we hit a block like....
#    define PROV_DEVICE_RESULT_VALUE            \
#        PROV_DEVICE_RESULT_OK,                  \
#        PROV_DEVICE_RESULT_INVALID_ARG,         \
# Then we read the enums (e.g. PROV_DEVICE_RESULT_OK) and store them for
# later in the file parse where the .c file actually instantiates the enum.
def add_new_values_enum(src_file_handle, current_line):
    # Remove the "define", trailing "/", and whitespace to get just value by itself
    enum_value_name = re.sub(".*#define\s*", "", current_line)
    enum_value_name = enum_value_name.replace("\\","").replace("\n","").strip()
    
    # Read the file - unlike most functions, we actually advance file pointer
    # directly here - and save them in the master hash table for later.
    enum_values = []
    current_line = src_file_handle.readline()
    while current_line:
        if (not current_line.strip()):
            # Reached an empty line.  No more enums to consume
            break
        enum_value = current_line.replace("\\","").replace(",","").strip()
        enum_values.append(enum_value)
        current_line = src_file_handle.readline()

    # Store the results in global hash table which will be referenced when we 
    # see this enum being instantiated via MU_DEFINE_ENUM in the .h
    mockable_enum_table[enum_value_name] = enum_values

# is_current_line_mu_define returns the enum entry if current_line
# maps to a MU_DEFINE_ENUM that we have previously stored the values for.
def is_current_line_mu_define(current_line):
    for key in mockable_enum_table.keys():
        key_search=".*" + key + ".*"
        if (re.match(key_search, current_line)):
            return key
    return None

# The current line is MU_DEFINE_ENUM_WITHOUT_INVALID (as one example).
# We translate this into a standard C style "typedef enum { <valuesWeReadEarlier,...>} typeName;"
def create_friendly_enum_value_string(enum_name):
    # Retrieve the values previously associated with this enum.
    enum_values = mockable_enum_table[enum_name]

    enum_string_start = "typedef enum { "
    enum_string_close = "} " + re.sub("_VALUES?", "", enum_name) + ";"
    enum_string_values = ""

    # Output the actual values of all but the last enum's into the output string.
    for enum_value in enum_values[0:len(enum_values)-1]:
        enum_string_values += enum_value + ", "
    
    # Special case the last output of enum whihc won't need a closing ","
    enum_string_values += enum_values[len(enum_values)-1] + " "

    return enum_string_start + enum_string_values + enum_string_close

# convert_line_if_needed is invoked on each line of a file being
# translated.  It returns the line (unmodified in most cases or
# with MOCKABLE logic cleaned up if needed) for caller to write.
def convert_line_if_needed(src_file_handle, current_line):
    if (re.match(".*MOCKABLE_FUNCTION", current_line)):
        return convert_mockable_function_line(current_line)
    elif (re.match(".*#define.*VALUES? ", current_line)):
        # Do not output anything on this line.  We need to stash the values 
        # for later.  Also note that add_new_values_enum will advance the file handle.
        add_new_values_enum(src_file_handle, current_line)
    elif ((enum_value := is_current_line_mu_define(current_line)) != None):
        # The current line is where enum is actually instantiated AND we have
        # the _VALUES define from earlier that matches the name specifically.
        # E.g <MU_DEFINE_ENUM_WITHOUT_INVALID(PROV_DEVICE_RESULT, PROV_DEVICE_RESULT_VALUE);> 
        # would match this if we had read PROV_DEVICE_RESULT_VALUE.
        return create_friendly_enum_value_string(enum_value)
    else:
        # No translation needed.  Output as-is.
        return current_line

# convert_file_output translates src_file_handle and writes the more
# doxygen friendly results to dst_file_handle
def convert_file_output(src_file_handle, dst_file_handle):
    current_line = src_file_handle.readline()
    while current_line:
        converted_line = convert_line_if_needed(src_file_handle, current_line)
        if (converted_line != None):
            dst_file_handle.write(converted_line)
        current_line = src_file_handle.readline()
        

# convert_all_files enumerates all files in the current directory and
# invokes translation function on them
def convert_all_files(src_path, dest_path):
    for file_iterator in os.scandir(src_path):
        if (not file_iterator.is_file()):
            continue

        # Reset the enum mapping table for each new file visited
        mockable_enum_table = {}

        src_file_name = os.path.join(src_path, file_iterator.name)
        dest_file_name = os.path.join(dest_path, file_iterator.name)

        logging.info("Translating file: " + src_file_name +  ".  Destination file: " + dest_file_name)
        src_file_handle = open(src_file_name, "r")
        dst_file_handle = open(dest_file_name, "w")
        convert_file_output(src_file_handle, dst_file_handle)
        dst_file_handle.close()
        src_file_handle.close()
        logging.info("Completed translation of: " + src_file_name + " into destination file:" + dest_file_name)


def main():
    if (len(sys.argv) != 3):
        raise SystemExit(f"Usage: {sys.argv[0]} <source directory> <output directory>")
    
    src_path = sys.argv[1]
    dest_path = sys.argv[2]

    if (not os.path.exists(dest_path)):
        logging.info("Creating directory " + dest_path)
        os.mkdir(dest_path)

    convert_all_files(src_path, dest_path)

if __name__ == "__main__":
    main()
