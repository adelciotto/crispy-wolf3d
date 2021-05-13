#include "config.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <math.h>
#include <sys/stat.h>

#include "log.h"

#define FILENAME "crispy-wolf3d.cfg"

#define ARRAY_LEN(x) (sizeof((x)) / sizeof(*(x)))

bool g_crispyConfigFullscreen = true;
bool g_crispyConfigHighRes = true;
bool g_crispyConfigAspectRatioCorrection = true;
unsigned int g_crispyConfigWindowScale = 1;

bool g_crispyConfigModernKeyboardMouse = true;
unsigned int g_crispyConfigMouseThreshold = 10;
float g_crispyConfigMouseAccel = 1.0f;

static struct CrispyConfigOpt options[] = {
	{ .name = "fullscreen",
	  .type = CRISPY_CONFIG_OPT_TYPE_BOOL,
	  .boolValue = &g_crispyConfigFullscreen },
	{ .name = "high_res",
	  .type = CRISPY_CONFIG_OPT_TYPE_BOOL,
	  .boolValue = &g_crispyConfigHighRes },
	{ .name = "aspect_ratio_correction",
	  .type = CRISPY_CONFIG_OPT_TYPE_BOOL,
	  .boolValue = &g_crispyConfigAspectRatioCorrection },
	{ .name = "window_scale",
	  .type = CRISPY_CONFIG_OPT_TYPE_UINT,
	  .uintValue = &g_crispyConfigWindowScale },

	{ .name = "modern_keyboard_mouse",
	  .type = CRISPY_CONFIG_OPT_TYPE_BOOL,
	  .boolValue = &g_crispyConfigModernKeyboardMouse },
	{ .name = "mouse_threshold",
	  .type = CRISPY_CONFIG_OPT_TYPE_UINT,
	  .uintValue = &g_crispyConfigMouseThreshold },
	{ .name = "mouse_accel",
	  .type = CRISPY_CONFIG_OPT_TYPE_FLOAT,
	  .floatValue = &g_crispyConfigMouseAccel }
};

static struct CrispyConfigOpt *findConfigOpt(char *name);
static int parseConfigOpt(CrispyConfigOpt *configOpt, char *value);

void crispyConfigRead(const char *configDir)
{
	char configPath[300];

	if (configDir[0])
		snprintf(configPath, sizeof(configPath), "%s/%s", configDir, FILENAME);
	else
		strcpy(configPath, FILENAME);

	struct stat buffer;
	if (stat(configPath, &buffer) != 0) {
		crispyLogInfo("No config found at %s", configPath);
		crispyConfigSave(configDir);
		return;
	}

	FILE *file = fopen(configPath, "r");
	if (!file) {
		crispyLogError("Failed to open config at %s", configPath);
		return;
	}

	char line[256];
	while (fgets(line, 256, file)) {
		char *name = strtok(line, "=");
		if (!name) {
			crispyLogError("Error reading name from option in config");
			continue;
		}

		struct CrispyConfigOpt *configOpt = findConfigOpt(name);
		if (!configOpt) {
			crispyLogError("Invalid option %s in config", name);
			continue;
		}

		char *value = strtok(NULL, "=");
		if (!value) {
			crispyLogError("Error reading value for option %s in config", name);
			continue;
		}

		// Strip the trailing newline from the value.
		value[strcspn(value, "\n")] = 0;

		if (parseConfigOpt(configOpt, value) != 0) {
			crispyLogError("Error updating value for option %s from config",
						   name);
			continue;
		}

		crispyLogInfo("Config option %s loaded with value %s", name, value);
	}
}

void crispyConfigSave(const char *configDir)
{
	char configPath[300];

	if (configDir[0])
		snprintf(configPath, sizeof(configPath), "%s/%s", configDir, FILENAME);
	else
		strcpy(configPath, FILENAME);

	FILE *file = fopen(configPath, "w");
	if (!file) {
		crispyLogError("Failed to open config at %s", configDir);
		return;
	}

	int len = ARRAY_LEN(options);
	for (int i = 0; i < len; i++) {
		struct CrispyConfigOpt option = options[i];
		switch (option.type) {
		case CRISPY_CONFIG_OPT_TYPE_UINT:
			fprintf(file, "%s=%u", option.name, *option.uintValue);
			break;
		case CRISPY_CONFIG_OPT_TYPE_BOOL: {
			const char *valueStr = *option.boolValue ? "true" : "false";
			fprintf(file, "%s=%s", option.name, valueStr);
			break;
		case CRISPY_CONFIG_OPT_TYPE_FLOAT:
			fprintf(file, "%s=%.1f", option.name, *option.floatValue);
			break;
		}
		}

		fprintf(file, "\n");
	}

	crispyLogInfo("Config successfully saved to %s", configPath);
}

static CrispyConfigOpt *findConfigOpt(char *name)
{
	int len = ARRAY_LEN(options);
	for (int i = 0; i < len; i++) {
		if (strcmp(options[i].name, name) == 0)
			return &options[i];
	}

	return NULL;
}

int strToUint(const char *s, unsigned int *out)
{
	if (strchr(s, '-') != NULL)
		return -1;

	char *endPtr;
	errno = 0;
	unsigned long ul = strtoul(s, &endPtr, 10);

	// Overflow, no conversion or trailing junk?
	if (errno || endPtr == s || *endPtr)
		return -1;

	*out = (unsigned int)ul;
	return 0;
}

int strToFloat(const char *s, float *out)
{
	char *endPtr;
	errno = 0;
	float val = strtof(s, &endPtr);

	// Overflow, no conversion or trailing junk?
	if (errno || endPtr == s || *endPtr)
		return -1;

	*out = val;
	return 0;
}

static int parseConfigOpt(struct CrispyConfigOpt *configOpt, char *value)
{
	switch (configOpt->type) {
	case CRISPY_CONFIG_OPT_TYPE_UINT: {
		unsigned int uintValue;
		if (strToUint(value, &uintValue) != 0) {
			crispyLogError("Failed to parse uint option value %s", value);
			return -1;
		}

		*configOpt->uintValue = uintValue;
		break;
	}
	case CRISPY_CONFIG_OPT_TYPE_BOOL: {
		bool boolValue;
		if (strcmp(value, "true") == 0) {
			boolValue = true;
		} else if (strcmp(value, "false") == 0) {
			boolValue = false;
		} else {
			crispyLogError("Failed to parse bool option value %s", value);
			return -1;
		}

		*configOpt->boolValue = boolValue;
		break;
	}
	case CRISPY_CONFIG_OPT_TYPE_FLOAT: {
		float floatValue;
		if (strToFloat(value, &floatValue) != 0) {
			crispyLogError("Failed to parse float option value %s", value);
			return -1;
		}

		if (floatValue == 0.0f || isnan(floatValue) || isinf(floatValue)) {
			crispyLogError(
					"Float option must have a valid positive value, got: %s",
					value);
			return -1;
		}

		*configOpt->floatValue = floatValue;
		break;
	}
	}

	return 0;
}
