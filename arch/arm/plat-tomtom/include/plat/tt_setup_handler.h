/* 
 * Copyright (C) 2009 TomTom BV <http://www.tomtom.com/>
 * Author: Kees Jongenburger <kees.jongenburger@tomtom.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef __TT_SETUP_HANDLER_H__
#define __TT_SETUP_HANDLER_H__ 1

/* Function prototype for registering callbacks to the parser */
typedef int (*tt_setup_handler)(char *identifier);

/* callback registration of tomtom "device setup" 
 * callback to call when implementation_name is found
 * service the generic name of the service (used for handling in kernel defaults)
 * impl_name the implementation name the specific implementation
 */
int tt_setup_register_handler(tt_setup_handler handler, char *identifier);

/**
 * Utility macro to add a callback for the tt_setup during postcore_init
 **/
#ifdef CONFIG_TOMTOM_TT_SETUP
#define TT_SETUP_CB(callback, identifier) \
	static int __init tt_setup_register_##callback( void ) { \
	        return tt_setup_register_handler(callback, identifier); \
	}\
	postcore_initcall(tt_setup_register_##callback);
#else
/* if CONFIG_TOMTOM_TT_SETUP is not set we skip the registration */
#define  TT_SETUP_CB(callback, identifier) 
#endif /* CONFIG_TOMTOM_TT_SETUP */
#endif
