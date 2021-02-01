#pragma once

#ifndef SSP_SETTINGS_DEBUG_RENDERER

#define SSP_SETTINGS_DEBUG_RENDERER false

#endif



#ifndef SSP_DEBUG_LOG_ENABLED

#define SSP_DEBUG_LOG_ENABLED

#endif



#ifdef SSP_DEBUG_LOG_ENABLED

#define SSP_LOG(s) plugin->cvarManager->log(s)
#define SSP_NO_PLUGIN_LOG(s) cvarManager->log(s)

#else

#define SSP_LOG(s)
#define SSP_NO_PLUGIN_LOG(s)

#endif


#define MMR_INVALID_VALUE -1.0f
#define ID_INVALID_VALUE 0
#define TEAM_INVALID_VALUE 255