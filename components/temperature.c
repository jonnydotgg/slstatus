/* See LICENSE file for copyright and license details. */
#include <stddef.h>

#include "../slstatus.h"
#include "../util.h"


#if defined(__linux__)
	#include <dirent.h>
	#include <limits.h>
	#include <stdint.h>
	#include <stdlib.h>
	#include <string.h>

	const char *
	temp(const char *file)
	{
		uintmax_t temp;

		if (pscanf(file, "%ju", &temp) != 1) {
			return NULL;
		}

		return bprintf("%ju", temp / 1000);
	}

	/* prefix with a dwm colour scheme byte when over the thresholds */
	static const char *
	hwmon_colour(const char *t)
	{
		static char out[32];
		unsigned int c;

		if (!t)
			return NULL;
		c = strtoul(t, NULL, 10);
		if (c < temp_warn)
			return t;
		/* t points into bprintf's buffer, so don't reuse it here */
		if (esnprintf(out, sizeof(out), "%s%s",
		              c >= temp_crit ? STATUS_CRIT : STATUS_WARN, t) < 0)
			return t;
		return out;
	}

	/* hwmonN numbering isn't stable across boots, so look it up by name */
	const char *
	hwmon_temp(const char *name)
	{
		static char path[PATH_MAX];
		char file[PATH_MAX], buf[64];
		const char *s;
		DIR *d;
		struct dirent *e;

		if (path[0] && (s = temp(path)))
			return hwmon_colour(s);

		path[0] = '\0';
		if (!(d = opendir("/sys/class/hwmon"))) {
			warn("opendir '/sys/class/hwmon':");
			return NULL;
		}
		while ((e = readdir(d))) {
			if (e->d_name[0] == '.')
				continue;
			if (esnprintf(file, sizeof(file), "/sys/class/hwmon/%s/name",
			              e->d_name) < 0)
				continue;
			if (pscanf(file, "%63s", buf) == 1 && !strcmp(buf, name)) {
				esnprintf(path, sizeof(path),
				          "/sys/class/hwmon/%s/temp1_input", e->d_name);
				break;
			}
		}
		closedir(d);

		return path[0] ? hwmon_colour(temp(path)) : NULL;
	}
#elif defined(__OpenBSD__)
	#include <stdio.h>
	#include <sys/time.h> /* before <sys/sensors.h> for struct timeval */
	#include <sys/sensors.h>
	#include <sys/sysctl.h>

	const char *
	temp(const char *unused)
	{
		int mib[5];
		size_t size;
		struct sensor temp;

		mib[0] = CTL_HW;
		mib[1] = HW_SENSORS;
		mib[2] = 0; /* cpu0 */
		mib[3] = SENSOR_TEMP;
		mib[4] = 0; /* temp0 */

		size = sizeof(temp);

		if (sysctl(mib, 5, &temp, &size, NULL, 0) < 0) {
			warn("sysctl 'SENSOR_TEMP':");
			return NULL;
		}

		/* kelvin to celsius */
		return bprintf("%d", (int)((float)(temp.value-273150000) / 1E6));
	}
#elif defined(__FreeBSD__)
	#include <stdio.h>
	#include <stdlib.h>
	#include <sys/sysctl.h>

	const char *
	temp(const char *zone)
	{
		char buf[256];
		int temp;
		size_t len;

		len = sizeof(temp);
		snprintf(buf, sizeof(buf), "hw.acpi.thermal.%s.temperature", zone);
		if (sysctlbyname(buf, &temp, &len, NULL, 0) == -1
				|| !len)
			return NULL;

		/* kelvin to decimal celcius */
		return bprintf("%d.%d", (temp - 2731) / 10, abs((temp - 2731) % 10));
	}
#endif
