#ifndef CRISPY_WOLF3D_LOG_H
#define CRISPY_WOLF3D_LOG_H

#include "SDL_log.h"

#define CrispyLogDebug(fmt, ...)                                                                                       \
    SDL_LogMessage(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_DEBUG, "%s(), %s:%d " fmt, __func__, __FILE__,       \
                   __LINE__, ##__VA_ARGS__)

#define CrispyLogInfo(fmt, ...)                                                                                        \
    SDL_LogMessage(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_INFO, "%s(), %s:%d " fmt, __func__, __FILE__,        \
                   __LINE__, ##__VA_ARGS__)

#define CrispyLogWarn(fmt, ...)                                                                                        \
    SDL_LogMessage(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_WARN, "%s(), %s:%d " fmt, __func__, __FILE__,        \
                   __LINE__, ##__VA_ARGS__)

#define CrispyLogError(fmt, ...)                                                                                       \
    SDL_LogMessage(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_ERROR, "%s(), %s:%d " fmt, __func__, __FILE__,       \
                   __LINE__, ##__VA_ARGS__)

#endif // CRISPY_WOLF3D_LOG_H
