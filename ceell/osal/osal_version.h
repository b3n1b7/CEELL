/*
 * CEELL OSAL — Application Version
 *
 * Provides build-time version macros from the VERSION file.
 */

#ifndef CEELL_OSAL_VERSION_H
#define CEELL_OSAL_VERSION_H

#if defined(CONFIG_ZEPHYR)
#include <zephyr/app_version.h>

#define CEELL_APP_VERSION_MAJOR APP_VERSION_MAJOR
#define CEELL_APP_VERSION_MINOR APP_VERSION_MINOR
#define CEELL_APP_PATCHLEVEL    APP_PATCHLEVEL

#else
/* Non-Zephyr backends: define via build system -D flags */
#ifndef CEELL_APP_VERSION_MAJOR
#define CEELL_APP_VERSION_MAJOR 0
#endif
#ifndef CEELL_APP_VERSION_MINOR
#define CEELL_APP_VERSION_MINOR 0
#endif
#ifndef CEELL_APP_PATCHLEVEL
#define CEELL_APP_PATCHLEVEL 0
#endif
#endif

#endif /* CEELL_OSAL_VERSION_H */
