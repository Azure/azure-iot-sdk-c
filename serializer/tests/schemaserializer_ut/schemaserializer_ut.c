// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifdef __cplusplus
#include <cstdlib>
#include <cstddef>
#else
#include <stdlib.h>
#include <stddef.h>
#endif

static void* my_gballoc_malloc(size_t t)
{
    return malloc(t);
}

static void* my_gballoc_realloc(void* v, size_t t)
{
    return realloc(v, t);
}

static void my_gballoc_free(void * t)
{
    free(t);
}

#include "umock_c/umock_c.h"
#include "umock_c/umocktypes_charptr.h"
#include "umock_c/umocktypes_bool.h"
#include "umock_c/umocktypes_stdint.h"
#include "umock_c/umock_c_negative_tests.h"

#define ENABLE_MOCKS
#include "schema.h"
#include "azure_c_shared_utility/optimize_size.h"
#include "azure_c_shared_utility/vector.h"
#include "azure_c_shared_utility/strings.h"
#undef ENABLE_MOCKS

#include "schemaserializer.h"
#include "testrunnerswitcher.h"


#define TEST_STRING_HANDLE  (STRING_HANDLE)0x4242
#define TEST_MODEL_HANDLE   (SCHEMA_MODEL_TYPE_HANDLE)0x4243
#define TEST_ACTION_1       (SCHEMA_ACTION_HANDLE)0x44
#define TEST_ACTION_2       (SCHEMA_ACTION_HANDLE)0x45
#define TEST_ARG_1          (SCHEMA_ACTION_ARGUMENT_HANDLE)0x4042
#define TEST_ARG_2          (SCHEMA_ACTION_ARGUMENT_HANDLE)0x4043

TEST_DEFINE_ENUM_TYPE(SCHEMA_SERIALIZER_RESULT, SCHEMA_SERIALIZER_RESULT_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(SCHEMA_SERIALIZER_RESULT, SCHEMA_SERIALIZER_RESULT_VALUES);

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)
static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    char temp_str[256];
    (void)snprintf(temp_str, sizeof(temp_str), "umock_c reported error :%s", MU_ENUM_TO_STRING(UMOCK_C_ERROR_CODE, error_code));
    ASSERT_FAIL(temp_str);
}

static TEST_MUTEX_HANDLE g_testByTest;

BEGIN_TEST_SUITE(SchemaSerializer_ut)

    TEST_SUITE_INITIALIZE(TestClassInitialize)
    {
        g_testByTest = TEST_MUTEX_CREATE();
        ASSERT_IS_NOT_NULL(g_testByTest);

        (void)umock_c_init(on_umock_c_error);
        (void)umocktypes_bool_register_types();
        (void)umocktypes_charptr_register_types();
        (void)umocktypes_stdint_register_types();

        REGISTER_UMOCK_ALIAS_TYPE(STRING_HANDLE, void*);
        REGISTER_UMOCK_ALIAS_TYPE(SCHEMA_MODEL_TYPE_HANDLE, void*);
        REGISTER_UMOCK_ALIAS_TYPE(SCHEMA_ACTION_HANDLE, void*);
        REGISTER_UMOCK_ALIAS_TYPE(SCHEMA_ACTION_ARGUMENT_HANDLE, void*);


        REGISTER_GLOBAL_MOCK_RETURN(STRING_concat, 0);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(STRING_concat, MU_FAILURE);
        REGISTER_GLOBAL_MOCK_RETURN(Schema_GetModelActionCount, SCHEMA_OK);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(Schema_GetModelActionCount, SCHEMA_ERROR);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(Schema_GetModelActionByIndex, NULL);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(Schema_GetModelActionName, NULL);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(Schema_GetModelActionArgumentCount, SCHEMA_ERROR);
        REGISTER_GLOBAL_MOCK_RETURN(Schema_GetModelActionArgumentCount, SCHEMA_OK);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(Schema_GetModelActionArgumentByIndex, NULL);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(Schema_GetActionArgumentName, NULL);
        REGISTER_GLOBAL_MOCK_FAIL_RETURN(Schema_GetActionArgumentType, NULL);

    }

    TEST_SUITE_CLEANUP(TestClassCleanup)
    {
        umock_c_deinit();

        TEST_MUTEX_DESTROY(g_testByTest);
    }

    TEST_FUNCTION_INITIALIZE(TestMethodInitialize)
    {
        if (TEST_MUTEX_ACQUIRE(g_testByTest))
        {
            ASSERT_FAIL("our mutex is ABANDONED. Failure in test framework");
        }

        umock_c_reset_all_calls();
    }

    TEST_FUNCTION_CLEANUP(TestMethodCleanup)
    {
        TEST_MUTEX_RELEASE(g_testByTest);
    }


    /* SchemaSerializer_SerializeCommandMetadata */

    /* Tests_SRS_SCHEMA_SERIALIZER_01_013: [If the modelHandle argument is NULL, SchemaSerializer_SerializeCommandMetadata shall return SCHEMA_SERIALIZER_INVALID_ARG.] */
    TEST_FUNCTION(SchemaSerializer_SerializeCommandMetadata_With_NULL_model_handle_fails)
    {
        // arrange

        // act
        SCHEMA_SERIALIZER_RESULT result = SchemaSerializer_SerializeCommandMetadata(NULL, TEST_STRING_HANDLE);

        // assert
        ASSERT_ARE_EQUAL(SCHEMA_SERIALIZER_RESULT, SCHEMA_SERIALIZER_INVALID_ARG, result);
    }

    /* Tests_SRS_SCHEMA_SERIALIZER_01_014: [If the schemaText argument is NULL, SchemaSerializer_SerializeCommandMetadata shall return SCHEMA_SERIALIZER_INVALID_ARG.] */
    TEST_FUNCTION(SchemaSerializer_SerializeCommandMetadata_With_NULL_string_handle_fails)
    {
        // arrange

        // act
        SCHEMA_SERIALIZER_RESULT result = SchemaSerializer_SerializeCommandMetadata(TEST_MODEL_HANDLE, NULL);

        // assert
        ASSERT_ARE_EQUAL(SCHEMA_SERIALIZER_RESULT, SCHEMA_SERIALIZER_INVALID_ARG, result);
    }

    static void SchemaSerializer_SerializeCommandMetadata_When_Command_Count_Is_0_Should_Yield_An_Empty_Commands_Array_inert_path(const size_t* commandCount)
    {
        STRICT_EXPECTED_CALL(STRING_concat(TEST_STRING_HANDLE, "["));
        STRICT_EXPECTED_CALL(Schema_GetModelActionCount(TEST_MODEL_HANDLE, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, commandCount, sizeof(*commandCount));
        STRICT_EXPECTED_CALL(STRING_concat(TEST_STRING_HANDLE, "]"));
    }

    /* Tests_SRS_SCHEMA_SERIALIZER_01_001: [SchemaSerializer_SerializeCommandMetadata shall serialize a specific model to a string using JSON as format.] */
    /* Tests_SRS_SCHEMA_SERIALIZER_01_002: [Only commands shall be serialized, the properties of a model shall be ignored.] */
    /* Tests_SRS_SCHEMA_SERIALIZER_01_003: [The output JSON shall have an array, where each array element shall represent a command.] */
    /* Tests_SRS_SCHEMA_SERIALIZER_01_011: [The JSON text shall be built into the string indicated by the schemaText argument by using String APIs.] */
    /* Tests_SRS_SCHEMA_SERIALIZER_01_012: [On success SchemaSerializer_SerializeCommandMetadata shall return SCHEMA_SERIALIZER_OK.] */
    /* Tests_SRS_SCHEMA_SERIALIZER_01_006: [The object for a command shall have a member named Name, whose value shall be the command name as obtained by using Schema APIs.] */
    TEST_FUNCTION(SchemaSerializer_SerializeCommandMetadata_When_Command_Count_Is_0_Should_Yield_An_Empty_Commands_Array_happy_path)
    {
        // arrange
        size_t commandCount = 0;

        SchemaSerializer_SerializeCommandMetadata_When_Command_Count_Is_0_Should_Yield_An_Empty_Commands_Array_inert_path(&commandCount);

        // act
        SCHEMA_SERIALIZER_RESULT result = SchemaSerializer_SerializeCommandMetadata(TEST_MODEL_HANDLE, TEST_STRING_HANDLE);

        // assert
        ASSERT_ARE_EQUAL(SCHEMA_SERIALIZER_RESULT, SCHEMA_SERIALIZER_OK, result);
    }

    /*negative testing*/
    /*Tests_SRS_SCHEMA_SERIALIZER_01_015: [ If any of the Schema or String APIs fail then SchemaSerializer_SerializeCommandMetadata shall return SCHEMA_SERIALIZER_ERROR. ]*/
    TEST_FUNCTION(SchemaSerializer_SerializeCommandMetadata_When_Command_Count_Is_0_Should_Yield_An_Empty_Commands_Array_unhappy_paths)
    {
        // arrange
        umock_c_negative_tests_init();

        size_t commandCount = 0;
        SchemaSerializer_SerializeCommandMetadata_When_Command_Count_Is_0_Should_Yield_An_Empty_Commands_Array_inert_path(&commandCount);

        umock_c_negative_tests_snapshot();

        for (size_t i = 0; i < umock_c_negative_tests_call_count(); i++)
        {
            umock_c_negative_tests_reset();

            umock_c_negative_tests_fail_call(i);

            // act
            SCHEMA_SERIALIZER_RESULT result = SchemaSerializer_SerializeCommandMetadata(TEST_MODEL_HANDLE, TEST_STRING_HANDLE);

            // assert
            ASSERT_ARE_NOT_EQUAL(SCHEMA_SERIALIZER_RESULT, SCHEMA_SERIALIZER_OK, result);
        }

        ///cleanup
        umock_c_negative_tests_deinit();
    }

    static void SchemaSerializer_SerializeCommandMetadata_1_Command_With_No_Arguments_Yields_The_Proper_JSON_inert_path(
        const size_t* commandCount,
        const size_t* argCount
    )
    {
        STRICT_EXPECTED_CALL(STRING_concat(TEST_STRING_HANDLE, "["));
        STRICT_EXPECTED_CALL(Schema_GetModelActionCount(TEST_MODEL_HANDLE, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, commandCount, sizeof(*commandCount));
        STRICT_EXPECTED_CALL(Schema_GetModelActionByIndex(TEST_MODEL_HANDLE, 0))
            .SetReturn(TEST_ACTION_1);
        STRICT_EXPECTED_CALL(STRING_concat(TEST_STRING_HANDLE, "{ \"Name\":\""));
        STRICT_EXPECTED_CALL(Schema_GetModelActionName(TEST_ACTION_1))
            .SetReturn("Action1");
        STRICT_EXPECTED_CALL(STRING_concat(TEST_STRING_HANDLE, "Action1"));
        STRICT_EXPECTED_CALL(STRING_concat(TEST_STRING_HANDLE, "\", \"Parameters\":["));
        STRICT_EXPECTED_CALL(Schema_GetModelActionArgumentCount(TEST_ACTION_1, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, argCount, sizeof(*argCount));
        STRICT_EXPECTED_CALL(STRING_concat(TEST_STRING_HANDLE, "]}"));
        STRICT_EXPECTED_CALL(STRING_concat(TEST_STRING_HANDLE, "]"));
    }
    /* Tests_SRS_SCHEMA_SERIALIZER_01_001: [SchemaSerializer_SerializeCommandMetadata shall serialize a specific model to a string using JSON as format.] */
    /* Tests_SRS_SCHEMA_SERIALIZER_01_002: [Only commands shall be serialized, the properties of a model shall be ignored.] */
    /* Tests_SRS_SCHEMA_SERIALIZER_01_003: [The output JSON shall have an array, where each array element shall represent a command.] */
    /* Tests_SRS_SCHEMA_SERIALIZER_01_011: [The JSON text shall be built into the string indicated by the schemaText argument by using String APIs.] */
    /* Tests_SRS_SCHEMA_SERIALIZER_01_012: [On success SchemaSerializer_SerializeCommandMetadata shall return SCHEMA_SERIALIZER_OK.] */
    /* Tests_SRS_SCHEMA_SERIALIZER_01_006: [The object for a command shall have a member named Name, whose value shall be the command name as obtained by using Schema APIs.] */
    /* Tests_SRS_SCHEMA_SERIALIZER_01_005: [Each array element shall be a JSON object.] */
    /* Tests_SRS_SCHEMA_SERIALIZER_01_007: [The object for a command shall also have a "parameters" member.] */
    TEST_FUNCTION(SchemaSerializer_SerializeCommandMetadata_1_Command_With_No_Arguments_Yields_The_Proper_JSON_happy_path)
    {
        // arrange
        size_t commandCount = 1;
        size_t argCount = 0;
        SchemaSerializer_SerializeCommandMetadata_1_Command_With_No_Arguments_Yields_The_Proper_JSON_inert_path(&commandCount, &argCount);

        // act
        SCHEMA_SERIALIZER_RESULT result = SchemaSerializer_SerializeCommandMetadata(TEST_MODEL_HANDLE, TEST_STRING_HANDLE);

        // assert
        ASSERT_ARE_EQUAL(SCHEMA_SERIALIZER_RESULT, SCHEMA_SERIALIZER_OK, result);
    }

    /*negative tests*/
    /*Tests_SRS_SCHEMA_SERIALIZER_01_015: [ If any of the Schema or String APIs fail then SchemaSerializer_SerializeCommandMetadata shall return SCHEMA_SERIALIZER_ERROR. ]*/
    TEST_FUNCTION(SchemaSerializer_SerializeCommandMetadata_1_Command_With_No_Arguments_Yields_The_Proper_JSON_unhappy_paths)
    {
        // arrange
        umock_c_negative_tests_init();

        size_t commandCount = 1;
        size_t argCount = 0;
        SchemaSerializer_SerializeCommandMetadata_1_Command_With_No_Arguments_Yields_The_Proper_JSON_inert_path(&commandCount, &argCount);

        umock_c_negative_tests_snapshot();
        for (size_t i = 0; i < umock_c_negative_tests_call_count(); i++)
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(i);
            char temp_str[128];
            sprintf(temp_str, "On failed call %lu", (unsigned long)i);

            // act
            SCHEMA_SERIALIZER_RESULT result = SchemaSerializer_SerializeCommandMetadata(TEST_MODEL_HANDLE, TEST_STRING_HANDLE);

            // assert
            ASSERT_ARE_NOT_EQUAL(SCHEMA_SERIALIZER_RESULT, SCHEMA_SERIALIZER_OK, result, temp_str);
        }

        ///cleanup
        umock_c_negative_tests_deinit();
    }

    static void SchemaSerializer_SerializeCommandMetadata_1_Command_With_1_Argument_Yields_The_Proper_JSON_inert_path(
        const size_t* commandCount,
        const size_t* argCount
    )
    {
        STRICT_EXPECTED_CALL(STRING_concat(TEST_STRING_HANDLE, "["));
        STRICT_EXPECTED_CALL(Schema_GetModelActionCount(TEST_MODEL_HANDLE, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, commandCount, sizeof(*commandCount));
        STRICT_EXPECTED_CALL(Schema_GetModelActionByIndex(TEST_MODEL_HANDLE, 0))
            .SetReturn(TEST_ACTION_1);
        STRICT_EXPECTED_CALL(STRING_concat(TEST_STRING_HANDLE, "{ \"Name\":\""));
        STRICT_EXPECTED_CALL(Schema_GetModelActionName(TEST_ACTION_1))
            .SetReturn("Action1");
        STRICT_EXPECTED_CALL(STRING_concat(TEST_STRING_HANDLE, "Action1"));
        STRICT_EXPECTED_CALL(STRING_concat(TEST_STRING_HANDLE, "\", \"Parameters\":["));
        STRICT_EXPECTED_CALL(Schema_GetModelActionArgumentCount(TEST_ACTION_1, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, argCount, sizeof(*argCount));
        STRICT_EXPECTED_CALL(Schema_GetModelActionArgumentByIndex(TEST_ACTION_1, 0))
            .SetReturn(TEST_ARG_1);
        STRICT_EXPECTED_CALL(STRING_concat(TEST_STRING_HANDLE, "{\"Name\":\""));
        STRICT_EXPECTED_CALL(Schema_GetActionArgumentName(TEST_ARG_1))
            .SetReturn("Argument1");
        STRICT_EXPECTED_CALL(STRING_concat(TEST_STRING_HANDLE, "Argument1"));
        STRICT_EXPECTED_CALL(STRING_concat(TEST_STRING_HANDLE, "\",\"Type\":\""));
        STRICT_EXPECTED_CALL(Schema_GetActionArgumentType(TEST_ARG_1))
            .SetReturn("ascii_char_ptr");
        STRICT_EXPECTED_CALL(STRING_concat(TEST_STRING_HANDLE, "string"));
        STRICT_EXPECTED_CALL(STRING_concat(TEST_STRING_HANDLE, "\"}"));
        STRICT_EXPECTED_CALL(STRING_concat(TEST_STRING_HANDLE, "]}"));
        STRICT_EXPECTED_CALL(STRING_concat(TEST_STRING_HANDLE, "]"));
    }

    /* Tests_SRS_SCHEMA_SERIALIZER_01_001: [SchemaSerializer_SerializeCommandMetadata shall serialize a specific model to a string using JSON as format.] */
    /* Tests_SRS_SCHEMA_SERIALIZER_01_002: [Only commands shall be serialized, the properties of a model shall be ignored.] */
    /* Tests_SRS_SCHEMA_SERIALIZER_01_003: [The output JSON shall have an array, where each array element shall represent a command.] */
    /* Tests_SRS_SCHEMA_SERIALIZER_01_011: [The JSON text shall be built into the string indicated by the schemaText argument by using String APIs.] */
    /* Tests_SRS_SCHEMA_SERIALIZER_01_012: [On success SchemaSerializer_SerializeCommandMetadata shall return SCHEMA_SERIALIZER_OK.] */
    /* Tests_SRS_SCHEMA_SERIALIZER_01_006: [The object for a command shall have a member named Name, whose value shall be the command name as obtained by using Schema APIs.] */
    /* Tests_SRS_SCHEMA_SERIALIZER_01_005: [Each array element shall be a JSON object.] */
    /* Tests_SRS_SCHEMA_SERIALIZER_01_007: [The object for a command shall also have a "Parameters" member.] */
    /* Tests_SRS_SCHEMA_SERIALIZER_01_016: ["ascii_char_ptr" shall be translated to "string".] */
    /* Tests_SRS_SCHEMA_SERIALIZER_01_008: [The parameters member shall be an array, where each entry is a command parameter.]*/
    /* Tests_SRS_SCHEMA_SERIALIZER_01_009: [Each command parameter shall have a member named "Name", that should have as value the command argument name as obtained by using Schema APIs.] */
    /* Tests_SRS_SCHEMA_SERIALIZER_01_010: [Each command parameter shall have a member named "Type", that should have as value the command argument type as obtained by using Schema APIs.] */
    TEST_FUNCTION(SchemaSerializer_SerializeCommandMetadata_1_Command_With_1_Argument_Yields_The_Proper_JSON_happy_path)
    {
        // arrange
        size_t commandCount = 1;
        size_t argCount = 1;
        SchemaSerializer_SerializeCommandMetadata_1_Command_With_1_Argument_Yields_The_Proper_JSON_inert_path(&commandCount, &argCount);
        // act
        SCHEMA_SERIALIZER_RESULT result = SchemaSerializer_SerializeCommandMetadata(TEST_MODEL_HANDLE, TEST_STRING_HANDLE);

        // assert
        ASSERT_ARE_EQUAL(SCHEMA_SERIALIZER_RESULT, SCHEMA_SERIALIZER_OK, result);
    }

    /*negative testing*/
    /*Tests_SRS_SCHEMA_SERIALIZER_01_015: [ If any of the Schema or String APIs fail then SchemaSerializer_SerializeCommandMetadata shall return SCHEMA_SERIALIZER_ERROR. ]*/
    TEST_FUNCTION(SchemaSerializer_SerializeCommandMetadata_1_Command_With_1_Argument_Yields_The_Proper_JSON_unhappy_paths)
    {
        /// arrange
        size_t commandCount = 1;
        size_t argCount = 1;
        umock_c_negative_tests_init();
        SchemaSerializer_SerializeCommandMetadata_1_Command_With_1_Argument_Yields_The_Proper_JSON_inert_path(&commandCount, &argCount);
        umock_c_negative_tests_snapshot();

        for (size_t i = 0; i < umock_c_negative_tests_call_count(); i++)
        {

            umock_c_negative_tests_reset();

            umock_c_negative_tests_fail_call(i);
            char temp_str[128];
            sprintf(temp_str, "On failed call %lu", (unsigned long)i);

            ///act
            SCHEMA_SERIALIZER_RESULT result = SchemaSerializer_SerializeCommandMetadata(TEST_MODEL_HANDLE, TEST_STRING_HANDLE);

            /// assert
            ASSERT_ARE_NOT_EQUAL(SCHEMA_SERIALIZER_RESULT, SCHEMA_SERIALIZER_OK, result);
        }

        /// cleanup
        umock_c_negative_tests_deinit();
    }

    static void SchemaSerializer_SerializeCommandMetadata_2_Commanda_With_1_Argument_Each_Yields_The_Proper_JSON_inert_path(const size_t* commandCount, const size_t* argCount)
    {
        STRICT_EXPECTED_CALL(STRING_concat(TEST_STRING_HANDLE, "["));
        STRICT_EXPECTED_CALL(Schema_GetModelActionCount(TEST_MODEL_HANDLE, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, commandCount, sizeof(*commandCount));
        STRICT_EXPECTED_CALL(Schema_GetModelActionByIndex(TEST_MODEL_HANDLE, 0))
            .SetReturn(TEST_ACTION_1);
        STRICT_EXPECTED_CALL(STRING_concat(TEST_STRING_HANDLE, "{ \"Name\":\""));
        STRICT_EXPECTED_CALL(Schema_GetModelActionName(TEST_ACTION_1))
            .SetReturn("Action1");
        STRICT_EXPECTED_CALL(STRING_concat(TEST_STRING_HANDLE, "Action1"));
        STRICT_EXPECTED_CALL(STRING_concat(TEST_STRING_HANDLE, "\", \"Parameters\":["));

        STRICT_EXPECTED_CALL(Schema_GetModelActionArgumentCount(TEST_ACTION_1, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, argCount, sizeof(*argCount));
        STRICT_EXPECTED_CALL(Schema_GetModelActionArgumentByIndex(TEST_ACTION_1, 0))
            .SetReturn(TEST_ARG_1);
        STRICT_EXPECTED_CALL(STRING_concat(TEST_STRING_HANDLE, "{\"Name\":\""));
        STRICT_EXPECTED_CALL(Schema_GetActionArgumentName(TEST_ARG_1))
            .SetReturn("Argument1");
        STRICT_EXPECTED_CALL(STRING_concat(TEST_STRING_HANDLE, "Argument1"));
        STRICT_EXPECTED_CALL(STRING_concat(TEST_STRING_HANDLE, "\",\"Type\":\""));
        STRICT_EXPECTED_CALL(Schema_GetActionArgumentType(TEST_ARG_1))
            .SetReturn("ascii_char_ptr");
        STRICT_EXPECTED_CALL(STRING_concat(TEST_STRING_HANDLE, "string"));
        STRICT_EXPECTED_CALL(STRING_concat(TEST_STRING_HANDLE, "\"}"));
        STRICT_EXPECTED_CALL(STRING_concat(TEST_STRING_HANDLE, "]},"));
        STRICT_EXPECTED_CALL(Schema_GetModelActionByIndex(TEST_MODEL_HANDLE, 1))
            .SetReturn(TEST_ACTION_2);
        STRICT_EXPECTED_CALL(STRING_concat(TEST_STRING_HANDLE, "{ \"Name\":\""));
        STRICT_EXPECTED_CALL(Schema_GetModelActionName(TEST_ACTION_2))
            .SetReturn("Action2");
        STRICT_EXPECTED_CALL(STRING_concat(TEST_STRING_HANDLE, "Action2"));
        STRICT_EXPECTED_CALL(STRING_concat(TEST_STRING_HANDLE, "\", \"Parameters\":["));
        STRICT_EXPECTED_CALL(Schema_GetModelActionArgumentCount(TEST_ACTION_2, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, argCount, sizeof(*argCount));
        STRICT_EXPECTED_CALL(Schema_GetModelActionArgumentByIndex(TEST_ACTION_2, 0))
            .SetReturn(TEST_ARG_1);
        STRICT_EXPECTED_CALL(STRING_concat(TEST_STRING_HANDLE, "{\"Name\":\""));
        STRICT_EXPECTED_CALL(Schema_GetActionArgumentName(TEST_ARG_1))
            .SetReturn("Argument1");
        STRICT_EXPECTED_CALL(STRING_concat(TEST_STRING_HANDLE, "Argument1"));
        STRICT_EXPECTED_CALL(STRING_concat(TEST_STRING_HANDLE, "\",\"Type\":\""));
        STRICT_EXPECTED_CALL(Schema_GetActionArgumentType(TEST_ARG_1))
            .SetReturn("ascii_char_ptr");
        STRICT_EXPECTED_CALL(STRING_concat(TEST_STRING_HANDLE, "string"));
        STRICT_EXPECTED_CALL(STRING_concat(TEST_STRING_HANDLE, "\"}"));
        STRICT_EXPECTED_CALL(STRING_concat(TEST_STRING_HANDLE, "]}"));
        STRICT_EXPECTED_CALL(STRING_concat(TEST_STRING_HANDLE, "]"));
    }

    /* Tests_SRS_SCHEMA_SERIALIZER_01_001: [SchemaSerializer_SerializeCommandMetadata shall serialize a specific model to a string using JSON as format.] */
    /* Tests_SRS_SCHEMA_SERIALIZER_01_002: [Only commands shall be serialized, the properties of a model shall be ignored.] */
    /* Tests_SRS_SCHEMA_SERIALIZER_01_003: [The output JSON shall have an array, where each array element shall represent a command.] */
    /* Tests_SRS_SCHEMA_SERIALIZER_01_011: [The JSON text shall be built into the string indicated by the schemaText argument by using String APIs.] */
    /* Tests_SRS_SCHEMA_SERIALIZER_01_012: [On success SchemaSerializer_SerializeCommandMetadata shall return SCHEMA_SERIALIZER_OK.] */
    /* Tests_SRS_SCHEMA_SERIALIZER_01_006: [The object for a command shall have a member named Name, whose value shall be the command name as obtained by using Schema APIs.] */
    /* Tests_SRS_SCHEMA_SERIALIZER_01_005: [Each array element shall be a JSON object.] */
    /* Tests_SRS_SCHEMA_SERIALIZER_01_007: [The object for a command shall also have a "Parameters" member.] */
    /* Tests_SRS_SCHEMA_SERIALIZER_01_016: ["ascii_char_ptr" shall be translated to "string".] */
    /* Tests_SRS_SCHEMA_SERIALIZER_01_008: [The parameters member shall be an array, where each entry is a command parameter.]*/
    /* Tests_SRS_SCHEMA_SERIALIZER_01_009: [Each command parameter shall have a member named "Name", that should have as value the command argument name as obtained by using Schema APIs.] */
    /* Tests_SRS_SCHEMA_SERIALIZER_01_010: [Each command parameter shall have a member named "Type", that should have as value the command argument type as obtained by using Schema APIs.] */
    TEST_FUNCTION(SchemaSerializer_SerializeCommandMetadata_2_Commanda_With_1_Argument_Each_Yields_The_Proper_JSON_happy_path)
    {
        // arrange
        size_t commandCount = 2;
        size_t argCount = 1;
        SchemaSerializer_SerializeCommandMetadata_2_Commanda_With_1_Argument_Each_Yields_The_Proper_JSON_inert_path(&commandCount, &argCount);

        // act
        SCHEMA_SERIALIZER_RESULT result = SchemaSerializer_SerializeCommandMetadata(TEST_MODEL_HANDLE, TEST_STRING_HANDLE);

        // assert
        ASSERT_ARE_EQUAL(SCHEMA_SERIALIZER_RESULT, SCHEMA_SERIALIZER_OK, result);
    }

    /*negative tests*/
    /*Tests_SRS_SCHEMA_SERIALIZER_01_015: [ If any of the Schema or String APIs fail then SchemaSerializer_SerializeCommandMetadata shall return SCHEMA_SERIALIZER_ERROR. ]*/
    TEST_FUNCTION(SchemaSerializer_SerializeCommandMetadata_2_Commanda_With_1_Argument_Each_Yields_The_Proper_JSON_unhappy_paths)
    {
        // arrange
        size_t commandCount = 2;
        size_t argCount = 1;
        umock_c_negative_tests_init();
        SchemaSerializer_SerializeCommandMetadata_2_Commanda_With_1_Argument_Each_Yields_The_Proper_JSON_inert_path(&commandCount, &argCount);
        umock_c_negative_tests_snapshot();

        for (size_t i = 0; i < umock_c_negative_tests_call_count(); i++)
        {

            umock_c_negative_tests_reset();

            umock_c_negative_tests_fail_call(i);
            char temp_str[128];
            sprintf(temp_str, "On failed call %lu", (unsigned long)i);

            /// act
            SCHEMA_SERIALIZER_RESULT result = SchemaSerializer_SerializeCommandMetadata(TEST_MODEL_HANDLE, TEST_STRING_HANDLE);

            /// assert
            ASSERT_ARE_NOT_EQUAL(SCHEMA_SERIALIZER_RESULT, SCHEMA_SERIALIZER_OK, result, temp_str);
        }

        ///cleanup
        umock_c_negative_tests_deinit();
    }


    void static SchemaSerializer_SerializeCommandMetadata_1_Command_With_1_Argument_Different_Than_String_Keeps_The_Same_Type_inert_path(const size_t* commandCount, const size_t* argCount)
    {
        STRICT_EXPECTED_CALL(STRING_concat(TEST_STRING_HANDLE, "["));
        STRICT_EXPECTED_CALL(Schema_GetModelActionCount(TEST_MODEL_HANDLE, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, commandCount, sizeof(*commandCount));
        STRICT_EXPECTED_CALL(Schema_GetModelActionByIndex(TEST_MODEL_HANDLE, 0))
            .SetReturn(TEST_ACTION_1);
        STRICT_EXPECTED_CALL(STRING_concat(TEST_STRING_HANDLE, "{ \"Name\":\""));
        STRICT_EXPECTED_CALL(Schema_GetModelActionName(TEST_ACTION_1))
            .SetReturn("Action1");
        STRICT_EXPECTED_CALL(STRING_concat(TEST_STRING_HANDLE, "Action1"));
        STRICT_EXPECTED_CALL(STRING_concat(TEST_STRING_HANDLE, "\", \"Parameters\":["));
        STRICT_EXPECTED_CALL(Schema_GetModelActionArgumentCount(TEST_ACTION_1, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, argCount, sizeof(*argCount));
        STRICT_EXPECTED_CALL(Schema_GetModelActionArgumentByIndex(TEST_ACTION_1, 0))
            .SetReturn(TEST_ARG_1);
        STRICT_EXPECTED_CALL(STRING_concat(TEST_STRING_HANDLE, "{\"Name\":\""));
        STRICT_EXPECTED_CALL(Schema_GetActionArgumentName(TEST_ARG_1))
            .SetReturn("Argument1");
        STRICT_EXPECTED_CALL(STRING_concat(TEST_STRING_HANDLE, "Argument1"));
        STRICT_EXPECTED_CALL(STRING_concat(TEST_STRING_HANDLE, "\",\"Type\":\""));
        STRICT_EXPECTED_CALL(Schema_GetActionArgumentType(TEST_ARG_1))
            .SetReturn("pupu");
        STRICT_EXPECTED_CALL(STRING_concat(TEST_STRING_HANDLE, "pupu"));
        STRICT_EXPECTED_CALL(STRING_concat(TEST_STRING_HANDLE, "\"}"));
        STRICT_EXPECTED_CALL(STRING_concat(TEST_STRING_HANDLE, "]}"));
        STRICT_EXPECTED_CALL(STRING_concat(TEST_STRING_HANDLE, "]"));
    }

    /* Tests_SRS_SCHEMA_SERIALIZER_01_017: [All other types shall be kept as they are.]  */
    TEST_FUNCTION(SchemaSerializer_SerializeCommandMetadata_1_Command_With_1_Argument_Different_Than_String_Keeps_The_Same_Type_happy_path)
    {
        // arrange
        size_t commandCount = 1;
        size_t argCount = 1;

        SchemaSerializer_SerializeCommandMetadata_1_Command_With_1_Argument_Different_Than_String_Keeps_The_Same_Type_inert_path(&commandCount, &argCount);

        // act
        SCHEMA_SERIALIZER_RESULT result = SchemaSerializer_SerializeCommandMetadata(TEST_MODEL_HANDLE, TEST_STRING_HANDLE);

        // assert
        ASSERT_ARE_EQUAL(SCHEMA_SERIALIZER_RESULT, SCHEMA_SERIALIZER_OK, result);
    }

    /*negative tests*/
    /*Tests_SRS_SCHEMA_SERIALIZER_01_015: [ If any of the Schema or String APIs fail then SchemaSerializer_SerializeCommandMetadata shall return SCHEMA_SERIALIZER_ERROR. ]*/
    TEST_FUNCTION(SchemaSerializer_SerializeCommandMetadata_1_Command_With_1_Argument_Different_Than_String_Keeps_The_Same_Type_unhappy_paths)
    {
        // arrange
        size_t commandCount = 1;
        size_t argCount = 1;
        umock_c_negative_tests_init();
        SchemaSerializer_SerializeCommandMetadata_1_Command_With_1_Argument_Different_Than_String_Keeps_The_Same_Type_inert_path(&commandCount, &argCount);
        umock_c_negative_tests_snapshot();

        for (size_t i = 0; i < umock_c_negative_tests_call_count(); i++)
        {

            umock_c_negative_tests_reset();

            umock_c_negative_tests_fail_call(i);
            char temp_str[128];
            sprintf(temp_str, "On failed call %lu", (unsigned long)i);

            ///act
            SCHEMA_SERIALIZER_RESULT result = SchemaSerializer_SerializeCommandMetadata(TEST_MODEL_HANDLE, TEST_STRING_HANDLE);

            /// assert
            ASSERT_ARE_NOT_EQUAL(SCHEMA_SERIALIZER_RESULT, SCHEMA_SERIALIZER_OK, result, temp_str);
        }

        /// cleanup
        umock_c_negative_tests_deinit();
    }

    static void SchemaSerializer_SerializeCommandMetadata_1_Command_With_2_Arguments_Yields_The_Proper_JSON_inert_path(const size_t* commandCount, const size_t* argCount)
    {
        STRICT_EXPECTED_CALL(STRING_concat(TEST_STRING_HANDLE, "["));
        STRICT_EXPECTED_CALL(Schema_GetModelActionCount(TEST_MODEL_HANDLE, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, commandCount, sizeof(*commandCount));
        STRICT_EXPECTED_CALL(Schema_GetModelActionByIndex(TEST_MODEL_HANDLE, 0))
            .SetReturn(TEST_ACTION_1);
        STRICT_EXPECTED_CALL(STRING_concat(TEST_STRING_HANDLE, "{ \"Name\":\""));
        STRICT_EXPECTED_CALL(Schema_GetModelActionName(TEST_ACTION_1))
            .SetReturn("Action1");
        STRICT_EXPECTED_CALL(STRING_concat(TEST_STRING_HANDLE, "Action1"));
        STRICT_EXPECTED_CALL(STRING_concat(TEST_STRING_HANDLE, "\", \"Parameters\":["));
        STRICT_EXPECTED_CALL(Schema_GetModelActionArgumentCount(TEST_ACTION_1, IGNORED_PTR_ARG))
            .CopyOutArgumentBuffer(2, argCount, sizeof(*argCount));
        STRICT_EXPECTED_CALL(Schema_GetModelActionArgumentByIndex(TEST_ACTION_1, 0))
            .SetReturn(TEST_ARG_1);
        STRICT_EXPECTED_CALL(STRING_concat(TEST_STRING_HANDLE, "{\"Name\":\""));
        STRICT_EXPECTED_CALL(Schema_GetActionArgumentName(TEST_ARG_1))
            .SetReturn("Argument1");
        STRICT_EXPECTED_CALL(STRING_concat(TEST_STRING_HANDLE, "Argument1"));
        STRICT_EXPECTED_CALL(STRING_concat(TEST_STRING_HANDLE, "\",\"Type\":\""));
        STRICT_EXPECTED_CALL(Schema_GetActionArgumentType(TEST_ARG_1))
            .SetReturn("ascii_char_ptr");
        STRICT_EXPECTED_CALL(STRING_concat(TEST_STRING_HANDLE, "string"));
        STRICT_EXPECTED_CALL(STRING_concat(TEST_STRING_HANDLE, "\"},"));
        STRICT_EXPECTED_CALL(Schema_GetModelActionArgumentByIndex(TEST_ACTION_1, 1))
            .SetReturn(TEST_ARG_2);
        STRICT_EXPECTED_CALL(STRING_concat(TEST_STRING_HANDLE, "{\"Name\":\""));
        STRICT_EXPECTED_CALL(Schema_GetActionArgumentName(TEST_ARG_2))
            .SetReturn("Argument2");
        STRICT_EXPECTED_CALL(STRING_concat(TEST_STRING_HANDLE, "Argument2"));
        STRICT_EXPECTED_CALL(STRING_concat(TEST_STRING_HANDLE, "\",\"Type\":\""));
        STRICT_EXPECTED_CALL(Schema_GetActionArgumentType(TEST_ARG_2))
            .SetReturn("ascii_char_ptr");
        STRICT_EXPECTED_CALL(STRING_concat(TEST_STRING_HANDLE, "string"));
        STRICT_EXPECTED_CALL(STRING_concat(TEST_STRING_HANDLE, "\"}"));
        STRICT_EXPECTED_CALL(STRING_concat(TEST_STRING_HANDLE, "]}"));
        STRICT_EXPECTED_CALL(STRING_concat(TEST_STRING_HANDLE, "]"));
    }

    /* Tests_SRS_SCHEMA_SERIALIZER_01_001: [SchemaSerializer_SerializeCommandMetadata shall serialize a specific model to a string using JSON as format.] */
    /* Tests_SRS_SCHEMA_SERIALIZER_01_002: [Only commands shall be serialized, the properties of a model shall be ignored.] */
    /* Tests_SRS_SCHEMA_SERIALIZER_01_003: [The output JSON shall have an array, where each array element shall represent a command.] */
    /* Tests_SRS_SCHEMA_SERIALIZER_01_011: [The JSON text shall be built into the string indicated by the schemaText argument by using String APIs.] */
    /* Tests_SRS_SCHEMA_SERIALIZER_01_012: [On success SchemaSerializer_SerializeCommandMetadata shall return SCHEMA_SERIALIZER_OK.] */
    /* Tests_SRS_SCHEMA_SERIALIZER_01_006: [The object for a command shall have a member named Name, whose value shall be the command name as obtained by using Schema APIs.] */
    /* Tests_SRS_SCHEMA_SERIALIZER_01_005: [Each array element shall be a JSON object.] */
    /* Tests_SRS_SCHEMA_SERIALIZER_01_007: [The object for a command shall also have a "Parameters" member.] */
    /* Tests_SRS_SCHEMA_SERIALIZER_01_016: ["ascii_char_ptr" shall be translated to "string".] */
    /* Tests_SRS_SCHEMA_SERIALIZER_01_008: [The parameters member shall be an array, where each entry is a command parameter.]*/
    /* Tests_SRS_SCHEMA_SERIALIZER_01_009: [Each command parameter shall have a member named "Name", that should have as value the command argument name as obtained by using Schema APIs.] */
    /* Tests_SRS_SCHEMA_SERIALIZER_01_010: [Each command parameter shall have a member named "Type", that should have as value the command argument type as obtained by using Schema APIs.] */
    TEST_FUNCTION(SchemaSerializer_SerializeCommandMetadata_1_Command_With_2_Arguments_Yields_The_Proper_JSON_happy_path)
    {
        // arrange
        size_t commandCount = 1;
        size_t argCount = 2;

        SchemaSerializer_SerializeCommandMetadata_1_Command_With_2_Arguments_Yields_The_Proper_JSON_inert_path(&commandCount, &argCount);

        // act
        SCHEMA_SERIALIZER_RESULT result = SchemaSerializer_SerializeCommandMetadata(TEST_MODEL_HANDLE, TEST_STRING_HANDLE);

        // assert
        ASSERT_ARE_EQUAL(SCHEMA_SERIALIZER_RESULT, SCHEMA_SERIALIZER_OK, result);
    }


    /*negative tests*/
    /*Tests_SRS_SCHEMA_SERIALIZER_01_015: [ If any of the Schema or String APIs fail then SchemaSerializer_SerializeCommandMetadata shall return SCHEMA_SERIALIZER_ERROR. ]*/
    TEST_FUNCTION(SchemaSerializer_SerializeCommandMetadata_1_Command_With_2_Arguments_Yields_The_Proper_JSON_unhappy_paths)
    {
        // arrange
        size_t commandCount = 1;
        size_t argCount = 2;
        umock_c_negative_tests_init();
        SchemaSerializer_SerializeCommandMetadata_1_Command_With_2_Arguments_Yields_The_Proper_JSON_inert_path(&commandCount, &argCount);
        umock_c_negative_tests_snapshot();

        for (size_t i = 0; i < umock_c_negative_tests_call_count(); i++)
        {

            umock_c_negative_tests_reset();

            umock_c_negative_tests_fail_call(i);
            char temp_str[128];
            sprintf(temp_str, "On failed call %lu", (unsigned long)i);

            ///act
            SCHEMA_SERIALIZER_RESULT result = SchemaSerializer_SerializeCommandMetadata(TEST_MODEL_HANDLE, TEST_STRING_HANDLE);

            /// assert
            ASSERT_ARE_NOT_EQUAL(SCHEMA_SERIALIZER_RESULT, SCHEMA_SERIALIZER_OK, result, temp_str);
        }

        /// cleanup
        umock_c_negative_tests_deinit();
    }
END_TEST_SUITE(SchemaSerializer_ut)

