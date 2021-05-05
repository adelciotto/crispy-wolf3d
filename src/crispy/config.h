#ifndef CRISPY_WOLF3D_CONFIG_H
#define CRISPY_WOLF3D_CONFIG_H

#include <stdbool.h>

enum CrispyConfigOptType
{
    CRISPY_CONFIG_OPT_TYPE_UINT,
    CRISPY_CONFIG_OPT_TYPE_BOOL
};

struct CrispyConfigOpt
{
    const char *name;
    enum CrispyConfigOptType type;
    union {
        unsigned int *uintValue;
        bool *boolValue;
    };
};

extern bool CrispyConfigFullscreen;
extern bool CrispyConfigHighRes;
extern bool CrispyConfigAspectRatioCorrection;
extern unsigned int CrispyConfigWindowScale;

void CrispyConfigRead(const char *configDir);

void CrispyConfigSave(const char *configDir);

#endif // CRISPY_WOLF3D_CONFIG_H
