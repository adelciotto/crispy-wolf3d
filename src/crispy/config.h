#ifndef CRISPY_WOLF3D_CONFIG_H
#define CRISPY_WOLF3D_CONFIG_H

#include <stdbool.h>

typedef enum CrispyConfigOptType {
	CRISPY_CONFIG_OPT_TYPE_UINT,
	CRISPY_CONFIG_OPT_TYPE_BOOL,
	CRISPY_CONFIG_OPT_TYPE_FLOAT
} CrispyConfigOptType;

typedef struct CrispyConfigOpt {
	const char *name;
	enum CrispyConfigOptType type;

	union {
		unsigned int *uintValue;
		bool *boolValue;
		float *floatValue;
	};
} CrispyConfigOpt;

extern bool g_crispyConfigFullscreen;
extern bool g_crispyConfigHighRes;
extern bool g_crispyConfigAspectRatioCorrection;
extern unsigned int g_crispyConfigWindowScale;

extern bool g_crispyConfigModernKeyboardMouse;
extern unsigned int g_crispyConfigMouseThreshold;
extern float g_crispyConfigMouseAccel;

void crispyConfigRead(const char *configDir);
void crispyConfigSave(const char *configDir);

#endif // CRISPY_WOLF3D_CONFIG_H
