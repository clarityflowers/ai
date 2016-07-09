#include "alpha_cube.h"

#include <SDL.h>
#include <windows.h>
#include <stdio.h>
#include <malloc.h>

#include "alpha_cube_win.h"

#define SCREEN_WIDTH 1024
#define SCREEN_HEIGHT 960
#define SCREEN_FPS 60
#define SCREEN_TICKS_PER_FRAME 16


//******************************************************************************
//**************************** PROFILER ****************************************
//******************************************************************************

internal uint64
StartProfiler()
{
    return SDL_GetPerformanceCounter();
}

internal double
StopProfiler(uint64 start_time)
{
    uint64 end_time, time_difference, performance_frequency;
    double milliseconds;
    char buffer[256];

    end_time = SDL_GetPerformanceCounter();
    time_difference = end_time - start_time;
    performance_frequency = SDL_GetPerformanceFrequency();
    milliseconds = (time_difference * 1000.0) / performance_frequency;
    snprintf(buffer, sizeof(buffer), "%.02f ms elapsed\n", milliseconds);
    OutputDebugStringA(buffer);

    return milliseconds;
}


//******************************************************************************
//**************************** UTILITIES ***************************************
//******************************************************************************

internal int
StringLength(char * string)
{
	int count = 0;
	while(*string++)
	{
		++count;
	}
	return count;
}

internal void
CatStrings(
 size_t source_a_count, char *source_a,
 size_t source_b_count, char *source_b,
 size_t dest_count, char *dest)
{
	// TODO(casey): Dest bounds checking!
	for(int i = 0; i < source_a_count; ++i)
	{
		*dest++ = *source_a++;
	}
	for(int Index = 0; Index < source_b_count; ++Index)
	{
		*dest++ = *source_b++;
	}
	*dest++ = 0;
}



//******************************************************************************
//**************************** LIBRARIES ***************************************
//******************************************************************************

internal void
Win_GetEXEFileName(WIN_STATE *state)
{
	DWORD size_of_filename = GetModuleFileNameA(0, state->exe_file_name, sizeof(state->exe_file_name));
	state->one_past_last_exe_file_name_slash = state->exe_file_name;
	for(char *scan = state->exe_file_name; *scan; ++scan)
	{
		if(*scan == '\\')
		{
			state->one_past_last_exe_file_name_slash = scan + 1;
		}
	}
}

internal void
Win_BuildEXEPathFileName(
 WIN_STATE *state, char *filename,
 int dest_count, char *dest
){
	CatStrings(state->one_past_last_exe_file_name_slash - state->exe_file_name,
		       state->exe_file_name,
		       StringLength(filename), filename,
		       dest_count, dest);
}

inline FILETIME
Win_GetLastWriteTime(char *filename)
{
	FILETIME last_write_time = {};
	WIN32_FILE_ATTRIBUTE_DATA data;
	if(GetFileAttributesEx(filename, GetFileExInfoStandard, &data))
	{
		last_write_time = data.ftLastWriteTime;
	}
	return last_write_time;
}

internal WIN_GAME_CODE
Win_LoadGameCode(char *source_dll_name, char *temp_dll_name)
{
	WIN_GAME_CODE result = {};

	result.dll_last_write_time = Win_GetLastWriteTime(source_dll_name);
	CopyFile(source_dll_name, temp_dll_name, FALSE);
	result.dll = LoadLibraryA(temp_dll_name);

	if(result.dll)
	{
		result.UpdateAndRender = (game_update_and_render *)GetProcAddress(result.dll, "GameUpdateAndRender" );
		// Result.GetSoundSamples = (game_get_sound_samples *)GetProcAddress(Result.DLL, "GameGetSoundSamples" );
		result.is_valid = 1; //(Result.UpdateAndRender && Result.GetSoundSamples);
	}
	if(!result.is_valid)
	{
		result.UpdateAndRender = 0;
		//Result.GetSoundSamples = 0;
	}
	return result;
}

internal void
Win_UnloadGameCode(WIN_GAME_CODE *game_code)
{
	if(game_code->dll)
	{
			FreeLibrary(game_code->dll);
	}

	game_code->is_valid = false;
	game_code->UpdateAndRender = 0;
	//GameCode->GetSoundSamples = 0;
}



//******************************************************************************
//**************************** MAIN ********************************************
//******************************************************************************

internal void
Win_LogKeyPress(GAME_BUTTON_STATE* button, SDL_Event* event)
{
	if (button->is_down != (event->key.type == SDL_KEYDOWN))
	{
		button->transitions++;
	}
}

static int
Win_Render(void *data)
{
    uint32 current_frame_time, delta_time;
    WIN_RENDER_BUFFER* buffer;

    buffer = (WIN_RENDER_BUFFER*) data;
    SDL_UpdateTexture(buffer->texture, NULL, buffer->pixels, buffer->pitch);
    SDL_RenderClear(buffer->renderer);
    SDL_RenderCopy(buffer->renderer, buffer->texture, NULL, NULL);

    current_frame_time = SDL_GetTicks();
    delta_time = current_frame_time - buffer->last_frame_time;
    if (delta_time < SCREEN_TICKS_PER_FRAME)
    {
        SDL_Delay(SCREEN_TICKS_PER_FRAME - delta_time);
        delta_time = SCREEN_TICKS_PER_FRAME;
    }
    SDL_RenderPresent(buffer->renderer);
    buffer->last_frame_time = current_frame_time;
    return 1;
}

int CALLBACK
WinMain(
 HINSTANCE Instance,
 HINSTANCE PrevInstance,
 LPSTR CmdLine,
 int ShowCode)
{
	WIN_STATE state = {};
	WIN_GAME_CODE game = {};
	bool32 game_is_running = 0;
	char source_game_code_dll_full_path[WIN32_STATE_FILE_NAME_COUNT] = {};
	char temp_game_code_dll_full_path[WIN32_STATE_FILE_NAME_COUNT] = {};
	SDL_Window *window;
	SDL_Renderer *renderer;
	SDL_Texture *texture;
    uint64 timer;

	LPVOID base_address;
	GAME_MEMORY game_memory = {};

	// uint32 last_frame_time = 0;
	// uint32 delta_time = 0;
	int pitch = NULL;
	PIXEL_BACKBUFFER buffer = {};
    WIN_RENDER_BUFFER render_buffer = {};
	GAME_INPUT input[2] = {};
	GAME_INPUT *old_input = &(input[0]);
	GAME_INPUT *new_input = &(input[1]);

    SDL_Thread *render_thread = NULL;
    int thread_return_value;

	Win_GetEXEFileName(&state);
	Win_BuildEXEPathFileName(&state, "alpha_cube.dll", sizeof(source_game_code_dll_full_path), source_game_code_dll_full_path);
	Win_BuildEXEPathFileName(&state, "alpha_cube_temp.dll", sizeof(temp_game_code_dll_full_path), temp_game_code_dll_full_path);
	game = Win_LoadGameCode(source_game_code_dll_full_path, temp_game_code_dll_full_path);

	if( SDL_Init( SDL_INIT_VIDEO ) < 0 )
    {
		exit(1);
		// TODO (Cerisa): Logging
    }

	window = SDL_CreateWindow("ALPHA CUBE", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
	if (window == 0)
	{
		exit(1);
		// TODO (Cerisa): Logging
	}

	renderer = SDL_CreateRenderer(window, -1, 0);
	if (renderer == 0)
	{
		exit(1);
		// TODO (Cerisa): Logging
	}
    render_buffer.renderer = renderer;

	texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STREAMING, SCREEN_WIDTH, SCREEN_HEIGHT);
	if (texture == 0)
	{
		char error[256];
	    snprintf(error, sizeof(error), "CreateTextureFromSruface failed: %s\n", SDL_GetError());
		OutputDebugStringA(error);
		exit(1);
	}
    render_buffer.texture = texture;

	buffer.w = SCREEN_WIDTH;
	buffer.h = SCREEN_HEIGHT;
	buffer.depth = 4;
	buffer.pitch = buffer.depth * buffer.w;
    render_buffer.pitch = buffer.pitch;
	// SDL_LockTexture(texture, 0, &(buffer.pixels), &(buffer.pitch));

#if ALPHA_CUBE_INTERNAL
	base_address = 0;
#else
	base_address = Terabytes(2);
#endif

	game_memory.permanent_storage_size = Megabytes(6);
	game_memory.transient_storage_size = Megabytes(6);

	state.total_size = game_memory.permanent_storage_size + game_memory.transient_storage_size;
	state.game_memory_block = VirtualAlloc(base_address, (size_t)state.total_size, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
	game_memory.permanent_storage = state.game_memory_block;
	game_memory.transient_storage = ((uint8 *)game_memory.permanent_storage + game_memory.permanent_storage_size);

	int buffer_memory_size = (buffer.w*buffer.h)*buffer.depth;
	buffer.pixels = VirtualAlloc(0, buffer_memory_size, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
	render_buffer.pixels = VirtualAlloc(0, buffer_memory_size, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);

	game_is_running = true;
	render_buffer.last_frame_time = SDL_GetTicks();
    timer = StartProfiler();

    double fps_tracker[10] = {};
    int fps_tracker_index = 0;
	while (game_is_running)
	{
		FILETIME new_dll_write_time;
		uint32 current_frame_time = 0;
		SDL_Event event;
		GAME_CONTROLLER_INPUT* old_keyboard;
		GAME_CONTROLLER_INPUT* new_keyboard;

		old_keyboard = &(old_input->controllers[0]);
		new_keyboard = &(new_input->controllers[0]);

		*new_keyboard = {};

		for (int i=0; i < ArrayCount(new_keyboard->buttons); i++)
		{
			new_keyboard->buttons[i].transitions = old_keyboard->buttons[i].transitions;
			new_keyboard->buttons[i].is_down = old_keyboard->buttons[i].is_down;
		}

		while( SDL_PollEvent(&event) != 0)
		{
			switch (event.type)
			{
				case SDL_QUIT:
				{
					game_is_running = false;
				} break;
				case SDL_KEYUP:
				case SDL_KEYDOWN:
				{
					switch(event.key.keysym.sym)
					{
						case SDLK_d:
						{
							Win_LogKeyPress(&(new_keyboard->move_right), &event);
						} break;
						case SDLK_a:
						{
							Win_LogKeyPress(&(new_keyboard->move_left), &event);
						} break;
						case SDLK_SPACE:
						{
							Win_LogKeyPress(&(new_keyboard->drop), &event);
						} break;
                        case SDLK_k:
                        {
							Win_LogKeyPress(&(new_keyboard->clear_board), &event);
                        } break;
                        case SDLK_ESCAPE:
                        {
							Win_LogKeyPress(&(new_keyboard->escape), &event);
                        } break;
					}
				}
				default:
				{

				}
			}
		}

		new_dll_write_time = Win_GetLastWriteTime(source_game_code_dll_full_path);
		if (CompareFileTime(&new_dll_write_time, &game.dll_last_write_time) != 0)
		{
			Win_UnloadGameCode(&game);
			game = Win_LoadGameCode(source_game_code_dll_full_path, temp_game_code_dll_full_path);
		}

        if (render_thread != NULL)
        {
            SDL_WaitThread(render_thread, &thread_return_value);
        }

        OutputDebugStringA("Game loop: ");
        fps_tracker[fps_tracker_index] = StopProfiler(timer);
        timer = StartProfiler();
        fps_tracker_index = (fps_tracker_index + 1) % 10;
        double fps = 0.0;
        for (int i=0; i<ArrayCount(fps_tracker); i++)
        {
            fps += fps_tracker[i];
        }
        fps = (1.0 / (fps / ArrayCount(fps_tracker))) * 1000.0;
        char out_buffer[256];
        snprintf(out_buffer, sizeof(out_buffer), "           %.02f fps\n", fps);
        OutputDebugStringA(out_buffer);

		game_is_running = game.UpdateAndRender(&game_memory, &buffer, new_input, 0);
        memcpy(render_buffer.pixels, buffer.pixels, buffer_memory_size);
        render_thread = SDL_CreateThread(Win_Render, "Render", (void*) (&render_buffer));

		{
			GAME_INPUT* tmp = old_input;
			old_input = new_input;
			new_input = tmp;
		}
		OutputDebugStringA("\n");
	}
    SDL_WaitThread(render_thread, &thread_return_value);
	SDL_DestroyTexture(texture);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
	Win_UnloadGameCode(&game);
	VirtualFree(game_memory.permanent_storage, 0, MEM_RELEASE);
	VirtualFree(game_memory.transient_storage, 0, MEM_RELEASE);
	VirtualFree(buffer.pixels, 0, MEM_RELEASE);
	return 0;
}
