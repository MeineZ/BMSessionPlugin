#pragma once

#ifndef SSP_SETTINGS_DEBUG_RENDERER

#define SSP_SETTINGS_DEBUG_RENDERER false

#endif



#ifndef SSP_DEBUG_LOG_ENABLED

//#define SSP_DEBUG_LOG_ENABLED

#endif



#ifdef SSP_DEBUG_LOG_ENABLED

#define SSP_LOG(s) plugin->cvarManager->log(s)

#else

#define SSP_LOG(s)

#endif