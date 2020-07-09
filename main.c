
#include <chiaki/base64.h>
#include <chiaki/session.h>

#include <stdio.h>
#include <string.h>

ChiakiLog *logger = NULL;
ChiakiSession session;

bool read_line(char *buf, size_t bufsz, FILE *f)
{
	size_t sz = 0;
	while(sz < bufsz) {
		char c;
		if(!fread(&c, 1, 1, f))
			return false;
		if(c == '\n') {
			buf[sz] = '\0';
			return true;
		}
		buf[sz++] = c;
	}
	return false;
}

/*
 * Settings format:
 *
 * <host>\n
 * <regist_key>\n
 * <morning in base64>\n
 * <resolution as int>\n
 * <fps as int>\n
 */

bool read_settings(ChiakiConnectInfo *connect_info, const char *file)
{
	FILE *f = fopen(file, "r");
	if(!f)
		return false;
	bool ret = false;

	char line[512];
#define READ_LINE do { if(!read_line(line, sizeof(line), f)) return false; } while(0)

	READ_LINE;
	connect_info->host = strdup(line);

	READ_LINE;
	memset(connect_info->regist_key, 0, sizeof(connect_info->regist_key));
	strncpy(connect_info->regist_key, line, sizeof(connect_info->regist_key));

	READ_LINE;
	size_t sz = sizeof(connect_info->morning);
	if(chiaki_base64_decode(line, 0, connect_info->morning, &sz) != CHIAKI_ERR_SUCCESS
			|| sz != sizeof(connect_info->morning))
		goto beach;

	READ_LINE;
	int resolution_preset = (int)strtol(line, NULL, 0);
	if(resolution_preset != CHIAKI_VIDEO_RESOLUTION_PRESET_360p
			&& resolution_preset != CHIAKI_VIDEO_RESOLUTION_PRESET_540p
			&& resolution_preset != CHIAKI_VIDEO_RESOLUTION_PRESET_720p
			&& resolution_preset != CHIAKI_VIDEO_RESOLUTION_PRESET_1080p)
		goto beach;

	READ_LINE;
	int fps_preset = (int)strtol(line, NULL, 0);
	if(fps_preset != CHIAKI_VIDEO_FPS_PRESET_30 && fps_preset != CHIAKI_VIDEO_FPS_PRESET_60)
		goto beach;

	chiaki_connect_video_profile_preset(&connect_info->video_profile, resolution_preset, fps_preset);

#undef READ_LINE
	ret = true;
beach:
	fclose(f);
	return true;
}

int main(int argc, const char *argv[])
{
	if(argc != 2)
	{
		printf("usage: %s [settings-file]\n", argv[0]);
		return 1;
	}
	const char *settings_file = argv[1];

	ChiakiErrorCode err;

	ChiakiConnectInfo connect_info;
	if(!read_settings(&connect_info, settings_file))
	{
		printf("reading settings from %s failed\n", settings_file);
		return 1;
	}

	err = chiaki_session_init(&session, &connect_info, NULL);

	printf("TODO: do stuff now\n");
	return 0;
}
