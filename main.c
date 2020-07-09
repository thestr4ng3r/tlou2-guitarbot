
#include <chiaki/base64.h>
#include <chiaki/session.h>

#include <stdio.h>
#include <string.h>
#include <unistd.h>

ChiakiLog _logger;
ChiakiLog *logger = &_logger;
ChiakiSession session;
int video_fd;
ChiakiThread play_th;

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
	if(chiaki_base64_decode(line, strlen(line), connect_info->morning, &sz) != CHIAKI_ERR_SUCCESS
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
	return ret;
}

bool video_cb(uint8_t *buf, size_t buf_size, void *user)
{
	write(video_fd, buf, buf_size);
	return true;
}

void switch_chordset(ChiakiControllerState *state, bool right)
{
	uint32_t button = right ? CHIAKI_CONTROLLER_BUTTON_R1 : CHIAKI_CONTROLLER_BUTTON_L1;
	state->buttons |= button;
	chiaki_session_set_controller_state(&session, state);
	usleep(32000);
	state->buttons &= ~button;
	chiaki_session_set_controller_state(&session, state);
}

typedef enum {
	WHEEL_TOP,
	WHEEL_TOPR,
	WHEEL_BOTR,
	WHEEL_BOT,
	WHEEL_BOTL,
	WHEEL_TOPL
} WheelPos;

void set_wheel(ChiakiControllerState *state, WheelPos pos)
{
	switch(pos)
	{
		case WHEEL_TOP:
			state->left_x = 0;
			state->left_y = INT16_MIN;
			break;
		case WHEEL_TOPR:
			state->left_x = INT16_MAX;
			state->left_y = -21342;
			break;
		case WHEEL_BOTR:
			state->left_x = INT16_MAX;
			state->left_y = 21342;
			break;
		case WHEEL_BOT:
			state->left_x = 0;
			state->left_y = INT16_MAX;
			break;
		case WHEEL_BOTL:
			state->left_x = INT16_MIN;
			state->left_y = 21342;
			break;
		case WHEEL_TOPL:
			state->left_x = INT16_MIN;
			state->left_y = -21342;
			break;
	}
}

typedef enum {
	STRING_NONE,
	STRING_E,
	STRING_a,
	STRING_d,
	STRING_g,
	STRING_b,
	STRING_e
} String;

void set_string(ChiakiControllerState *state, String string)
{
	state->touches[0].id = state->touches[1].id = -1;

	uint16_t y = 0;
	switch(string)
	{
		case STRING_NONE:
			return;
		case STRING_E:
			y = 850;
			break;
		case STRING_a:
			y = 750;
			break;
		case STRING_d:
			y = 600;
			break;
		case STRING_g:
			y = 400;
			break;
		case STRING_b:
			y = 250;
			break;
		case STRING_e:
			y = 50;
			break;
	}
	chiaki_controller_state_start_touch(state, 1920/2, y);
}

typedef struct {
	uint64_t duration;
	WheelPos wheel_pos;
	String string;
} Note;

#define DUR (300    *60/120)

Note song[] = {
	{ DUR, WHEEL_BOT,  STRING_a },
	{ DUR, WHEEL_BOT,  STRING_d },
	{ DUR, WHEEL_BOT,  STRING_g },
	{ DUR, WHEEL_TOPR, STRING_g },

	{ DUR, WHEEL_TOPR, STRING_E },
	{ DUR, WHEEL_TOPR, STRING_a },
	{ DUR, WHEEL_TOPR, STRING_d },
	{ DUR, WHEEL_TOPR, STRING_g },

	{ DUR, WHEEL_TOPL, STRING_E },
	{ DUR, WHEEL_TOPL, STRING_a },
	{ DUR, WHEEL_TOPR, STRING_d },
	{ DUR, WHEEL_TOPR, STRING_g },

	{ DUR, WHEEL_TOPR, STRING_E },
	{ DUR, WHEEL_TOPR, STRING_a },
	{ DUR, WHEEL_TOPR, STRING_d },
	{ DUR, WHEEL_TOPR, STRING_g }
};

#define NOTES_COUNT (sizeof(song) / sizeof(Note))

void play_note(ChiakiControllerState *state, Note *note)
{
	int16_t lx = state->left_x;
	int16_t ly = state->left_y;
	set_wheel(state, note->wheel_pos);
	if(state->left_x != lx || state->left_y != ly)
	{
		printf("wheel\n");
		chiaki_session_set_controller_state(&session, state);
		usleep(32000);
	}

	printf("string\n");
	set_string(state, note->string);
	chiaki_session_set_controller_state(&session, state);
	usleep(32000);
	set_string(state, STRING_NONE);
	chiaki_session_set_controller_state(&session, state);
	usleep(note->duration * 1000);
}

void *play(void *user)
{
	(void)user;

	ChiakiControllerState state;
	chiaki_controller_state_set_idle(&state);

	size_t i=0;
	while(1)
	{
		play_note(&state, &song[i++]);
		i = i % NOTES_COUNT;
	}

	return NULL;
}

void event_cb(ChiakiEvent *event, void *user)
{
	if(event->type == CHIAKI_EVENT_CONNECTED)
		chiaki_thread_create(&play_th, play, NULL);
}

int main(int argc, const char *argv[])
{
	if(argc < 2 || argc > 3)
	{
		printf("usage: %s [settings-file] ([video-fd])\n", argv[0]);
		return 1;
	}
	const char *settings_file = argv[1];
	if(argc > 2)
		video_fd = (int)strtol(argv[2], NULL, 0);
	else
		video_fd = -1;

	ChiakiErrorCode err;

	chiaki_log_init(&_logger, CHIAKI_LOG_ALL & (~CHIAKI_LOG_VERBOSE), NULL, NULL);

	ChiakiConnectInfo connect_info;
	if(!read_settings(&connect_info, settings_file))
	{
		printf("reading settings from %s failed\n", settings_file);
		return 1;
	}

	err = chiaki_session_init(&session, &connect_info, logger);
	free((void *)connect_info.host);
	if(err != CHIAKI_ERR_SUCCESS)
	{
		printf("failed to init session\n");
		return 1;
	}

	chiaki_session_set_video_sample_cb(&session, video_cb, NULL);
	chiaki_session_set_event_cb(&session, event_cb, NULL);

	err = chiaki_session_start(&session);
	if(err != CHIAKI_ERR_SUCCESS)
	{
		printf("failed to start session\n");
		return 1;
	}

	chiaki_session_join(&session);
	chiaki_session_fini(&session);

	return 0;
}
