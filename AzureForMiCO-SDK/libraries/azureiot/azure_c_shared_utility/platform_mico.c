// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

/* This is a template file used for porting */

/* Please go through all the TODO sections below and implement the needed code */

#include <stdlib.h>
#ifdef _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif
#include "platform_mico.h"
#include "xio.h"
#include "tlsio_mico.h"

int platform_init(void)
{
	/* TODO: Insert here any platform specific one time initialization code like:
	- starting TCP stack
	- starting timer
	- etc.
	*/
	
	return 0;
}

const IO_INTERFACE_DESCRIPTION* platform_get_default_tlsio(void)
{
	/* TODO: Insert here a call to the tlsio adapter that is available on your platform.
	return tlsio_mytls_get_interface_description(); */
    return tlsio_mico_get_interface_description();
}

void platform_deinit(void)
{
	/* TODO: Insert here any platform specific one time de-initialization code like.
	This has to undo what has been done in platform_init. */
}
