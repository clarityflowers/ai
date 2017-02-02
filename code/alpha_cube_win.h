#if !defined (ALPHA_CUBE_WIN32_H)

#define WIN32_STATE_FILE_NAME_COUNT MAX_PATH

struct WIN_GIFFER
{
    GIFFER_MEMORY* memory;
    PIXEL_BACKBUFFER buffer;
};

struct WIN_RENDER_BUFFER
{
    void* pixels;
    SDL_Renderer* renderer;
    SDL_Texture* texture;
    int pitch;
    bool32 is_rendering;
    uint32 last_frame_time;
};

struct WIN_PIXEL
{
	uint8 r;
	uint8 g;
	uint8 b;
	uint8 a;
};

struct WIN_GAME_CODE
{
	HMODULE dll;
	FILETIME dll_last_write_time;

	game_update_and_render *UpdateAndRender;
	game_get_sound_samples *GetSoundSamples;

	bool32 is_valid;
};

struct WIN_STATE
{
	uint64_t total_size;
	void *game_memory_block;

	char exe_file_name[WIN32_STATE_FILE_NAME_COUNT];
    char *one_past_last_exe_file_name_slash;
};

#define ALPHA_CUBE_WIN32_H
#endif
