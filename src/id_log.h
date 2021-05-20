#ifndef CRISPY_WOLF3D_ID_LOG_H
#define CRISPY_WOLF3D_ID_LOG_H

#include "SDL_log.h"

#define LOG_Debug(fmt, ...)                                                    \
	SDL_LogMessage(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_DEBUG,       \
				   "%s(), %s:%d " fmt, __func__, __FILE__, __LINE__,           \
				   ##__VA_ARGS__)

#define LOG_Info(fmt, ...)                                                     \
	SDL_LogMessage(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_INFO,        \
				   "%s(), %s:%d " fmt, __func__, __FILE__, __LINE__,           \
				   ##__VA_ARGS__)

#define LOG_Warn(fmt, ...)                                                     \
	SDL_LogMessage(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_WARN,        \
				   "%s(), %s:%d " fmt, __func__, __FILE__, __LINE__,           \
				   ##__VA_ARGS__)

#define LOG_Error(fmt, ...)                                                    \
	SDL_LogMessage(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_ERROR,       \
				   "%s(), %s:%d " fmt, __func__, __FILE__, __LINE__,           \
				   ##__VA_ARGS__)

#endif //CRISPY_WOLF3D_ID_LOG_H
