#include "config.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/stat.h>

#include "log.h"

#define FILENAME "crispy-wolf3d.cfg"

#define ARRAY_LEN(x) (sizeof((x)) / sizeof(*(x)))

bool CrispyConfigFullscreen = true;
bool CrispyConfigHighRes = true;
bool CrispyConfigAspectRatioCorrection = true;
unsigned int CrispyConfigWindowScale = 1;

static struct CrispyConfigOpt options[] = {
    {.name = "fullscreen", .type = CRISPY_CONFIG_OPT_TYPE_BOOL, .boolValue = &CrispyConfigFullscreen},
    {.name = "high_res", .type = CRISPY_CONFIG_OPT_TYPE_BOOL, .boolValue = &CrispyConfigHighRes},
    {.name = "aspect_ratio_correction",
     .type = CRISPY_CONFIG_OPT_TYPE_BOOL,
     .boolValue = &CrispyConfigAspectRatioCorrection},
    {.name = "window_scale", .type = CRISPY_CONFIG_OPT_TYPE_UINT, .uintValue = &CrispyConfigWindowScale}};

static struct CrispyConfigOpt *FindConfigOpt(char *name);
static int ReadConfigOpt(struct CrispyConfigOpt *configOpt, char *value);

void CrispyConfigRead(const char *configDir)
{
    char configPath[300];

    if (configDir[0])
        snprintf(configPath, sizeof(configPath), "%s/%s", configDir, FILENAME);
    else
        strcpy(configPath, FILENAME);

    struct stat buffer;
    if (stat(configPath, &buffer) != 0)
    {
        CrispyLogInfo("No config found at %s", configPath);
        CrispyConfigSave(configDir);
        return;
    }

    FILE *file = fopen(configPath, "r");
    if (!file)
    {
        CrispyLogError("Failed to open config at %s", configPath);
        return;
    }

    char line[256];
    while (fgets(line, 256, file))
    {
        char *name = strtok(line, "=");
        if (!name)
        {
            CrispyLogError("Error reading name from option in config");
            continue;
        }

        struct CrispyConfigOpt *configOpt = FindConfigOpt(name);
        if (!configOpt)
        {
            CrispyLogError("Invalid option %s in config", name);
            continue;
        }

        char *value = strtok(NULL, "=");
        if (!value)
        {
            CrispyLogError("Error reading value for option %s in config", name);
            continue;
        }

        // Strip the trailing newline from the value.
        value[strcspn(value, "\n")] = 0;

        if (ReadConfigOpt(configOpt, value) != 0)
        {
            CrispyLogError("Error updating value for option %s from config", name);
            continue;
        }

        CrispyLogInfo("Config option %s loaded with value %s", name, value);
    }
}

void CrispyConfigSave(const char *configDir)
{
    char configPath[300];

    if (configDir[0])
        snprintf(configPath, sizeof(configPath), "%s/%s", configDir, FILENAME);
    else
        strcpy(configPath, FILENAME);

    FILE *file = fopen(configPath, "w");
    if (!file)
    {
        CrispyLogError("Failed to open config at %s", configDir);
        return;
    }

    int len = ARRAY_LEN(options);
    for (int i = 0; i < len; i++)
    {
        struct CrispyConfigOpt option = options[i];
        switch (option.type)
        {
        case CRISPY_CONFIG_OPT_TYPE_UINT:
            fprintf(file, "%s=%u", option.name, *option.uintValue);
            break;
        case CRISPY_CONFIG_OPT_TYPE_BOOL: {
            const char *valueStr = *option.boolValue ? "true" : "false";
            fprintf(file, "%s=%s", option.name, valueStr);
            break;
        }
        }

        fprintf(file, "\n");
    }

    CrispyLogInfo("Config successfully saved to %s", configPath);
}

static struct CrispyConfigOpt *FindConfigOpt(char *name)
{
    int len = ARRAY_LEN(options);
    for (int i = 0; i < len; i++)
    {
        if (strcmp(options[i].name, name) == 0)
            return &options[i];
    }

    return NULL;
}

int StrToUint(const char *s, unsigned int *out)
{
    if (strchr(s, '-') != NULL)
        return -1;

    char *endptr;
    errno = 0;
    unsigned long ul = strtoul(s, &endptr, 10);

    // overflow? no conversion? trailing junk?
    if (errno || endptr == s || *endptr)
        return -1;

    *out = (unsigned int)ul;
    return 0;
}

static int ReadConfigOpt(struct CrispyConfigOpt *configOpt, char *value)
{
    switch (configOpt->type)
    {
    case CRISPY_CONFIG_OPT_TYPE_UINT: {
        unsigned int uintValue;
        if (StrToUint(value, &uintValue) != 0)
        {
            CrispyLogError("Failed to parse uint option value %s", value);
            return -1;
        }

        *configOpt->uintValue = uintValue;
        break;
    }
    case CRISPY_CONFIG_OPT_TYPE_BOOL: {
        bool boolValue;
        if (strcmp(value, "true") == 0)
        {
            boolValue = true;
        }
        else if (strcmp(value, "false") == 0)
        {
            boolValue = false;
        }
        else
        {
            CrispyLogError("Failed to parse bool option value %s", value);
            return -1;
        }

        *configOpt->boolValue = boolValue;
        break;
    }
    }

    return 0;
}
