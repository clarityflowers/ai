#include "platform.h"

#include <SDL.h>
#include <Xaudio2.h>
#include "giffer.h"
#include <windows.h>
#include <stdio.h>
#include <malloc.h>

#include "alpha_cube_win.h"

// #define SCREEN_WIDTH 1024
// #define SCREEN_HEIGHT 960
#define SCREEN_WIDTH 512
#define SCREEN_HEIGHT 480
#define SCREEN_FPS 30
#define GIFFER_FRAME_RATE 1
#define SCREEN_TICKS_PER_FRAME 33
#define AUDIO_BUFFERS 3


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

    end_time = SDL_GetPerformanceCounter();
    time_difference = end_time - start_time;
    performance_frequency = SDL_GetPerformanceFrequency();
    milliseconds = (time_difference * 1000.0) / performance_frequency;
    // {
    //     char buffer[256];
    //     snprintf(buffer, sizeof(buffer), "%.02f ms elapsed\n", milliseconds);
    //     OutputDebugStringA(buffer);
    // }

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
	for(size_t i = 0; i < source_a_count; ++i)
	{
		*dest++ = *source_a++;
	}
	for(size_t Index = 0; Index < source_b_count; ++Index)
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
        result.GetSoundSamples = (game_get_sound_samples *)GetProcAddress(result.dll, "GameGetSoundSamples" );
        result.is_valid = (result.UpdateAndRender && result.GetSoundSamples);
    }
    if(!result.is_valid)
    {
        result.UpdateAndRender = 0;
        result.GetSoundSamples = 0;
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
	game_code->GetSoundSamples = 0;
}



//******************************************************************************
//**************************** MAIN ********************************************
//******************************************************************************

DEBUG_PLATFORM_FREE_FILE_MEMORY(DebugWin_FreeFileMemory)
{
    if(memory)
    {
        VirtualFree(memory, 0, MEM_RELEASE);
    }
}

uint32 FileSize(char *filename)
{
    uint32 result = 0;
    
    HANDLE fileHandle = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
    if(fileHandle != INVALID_HANDLE_VALUE)
    {
        LARGE_INTEGER fileSize;
        if(GetFileSizeEx(fileHandle, &fileSize))
        {
            result = SafeTruncateUInt64(fileSize.QuadPart);
        }
        else
        {
            // TODO(casey): Logging
        }

        CloseHandle(fileHandle);
    }
    else
    {
        // TODO(casey): Logging
    }

    return(result);
}

DEBUG_PLATFORM_READ_ENTIRE_FILE(DEBUGPlatformReadEntireFile)
{
    DEBUG_READ_FILE_RESULT result = {};
    
    HANDLE fileHandle = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
    if(fileHandle != INVALID_HANDLE_VALUE)
    {
        LARGE_INTEGER fileSize;
        if(GetFileSizeEx(fileHandle, &fileSize))
        {
            uint32 fileSize32 = SafeTruncateUInt64(fileSize.QuadPart);
            result.contents = VirtualAlloc(0, fileSize32, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
            if(result.contents)
            {
                DWORD bytesRead;
                if(ReadFile(fileHandle, result.contents, fileSize32, &bytesRead, 0) &&
                   (fileSize32 == bytesRead))
                {
                    // NOTE(casey): File read successfully
                    result.contents_size = fileSize32;
                }
                else
                {                    
                    // TODO(casey): Logging
                    DebugWin_FreeFileMemory(thread, result.contents);
                    result.contents = 0;
                }
            }
            else
            {
                // TODO(casey): Logging
            }
        }
        else
        {
            // TODO(casey): Logging
        }

        CloseHandle(fileHandle);
    }
    else
    {
        // TODO(casey): Logging
    }

    return(result);
}

DEBUG_PLATFORM_WRITE_ENTIRE_FILE(DEBUGPlatformWriteEntireFile)
{
    bool32 result = false;
    
    HANDLE fileHandle = CreateFileA(filename, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
    if(fileHandle != INVALID_HANDLE_VALUE)
    {
        DWORD bytesWritten;
        bool32 write_result = WriteFile(fileHandle, memory, memorySize, &bytesWritten, 0);
        if(write_result)
        {
            // NOTE(casey): File read successfully
            result = (bytesWritten == memorySize);
        }
        else
        {
            DWORD error = GetLastError();
            {
                char text[256];
                snprintf(text, sizeof(text), "Save sprite failed: %d\n", GetLastError());
                OutputDebugStringA(text);
            }
        }

        CloseHandle(fileHandle);
    }
    else
    {
        // TODO(casey): Logging
    }

    return(result);
}

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
    WIN_RENDER_BUFFER* buffer;

    buffer = (WIN_RENDER_BUFFER*) data;
    SDL_UpdateTexture(buffer->texture, NULL, buffer->pixels, buffer->pitch);
    SDL_RenderClear(buffer->renderer);
    SDL_RenderCopy(buffer->renderer, buffer->texture, NULL, NULL);
    return 0;
}

static int
Win_Giffer(void *data)
{
    WIN_GIFFER* giffer;

    giffer = (WIN_GIFFER*) data;
    Giffer_Frame(giffer->memory, (uint8*) giffer->buffer.pixels);
    return 0;
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
	char game_code_lock_full_path[WIN32_STATE_FILE_NAME_COUNT] = {};
	SDL_Window *window;
	SDL_Renderer *renderer;
	SDL_Texture *texture;
    uint64 timer;
    bool32 recording_gif = false;

	LPVOID base_address;
	GAME_MEMORY game_memory = {};

	int pitch = NULL;
	PIXEL_BACKBUFFER buffer = {};
    GAME_AUDIO game_audio = {};
    WIN_RENDER_BUFFER render_buffer = {};
    WIN_GIFFER giffer = {};
	GAME_INPUT input[2] = {};
	GAME_INPUT *old_input = &(input[0]);
	GAME_INPUT *new_input = &(input[1]);

    SDL_Thread *render_thread = NULL;
    int thread_return_value;
    SDL_Thread *giffer_thread = NULL;

	Win_GetEXEFileName(&state);
	Win_BuildEXEPathFileName(&state, "alpha_cube.dll", sizeof(source_game_code_dll_full_path), source_game_code_dll_full_path);
    Win_BuildEXEPathFileName(&state, "alpha_cube_temp.dll", sizeof(temp_game_code_dll_full_path), temp_game_code_dll_full_path);
	Win_BuildEXEPathFileName(&state, "lock.tmp", sizeof(game_code_lock_full_path), game_code_lock_full_path);
	game = Win_LoadGameCode(source_game_code_dll_full_path, temp_game_code_dll_full_path);

	if( SDL_Init( SDL_INIT_VIDEO | SDL_INIT_AUDIO ) < 0 )
    {
		exit(1);
		// TODO (Cerisa): Logging
    }

    window = SDL_CreateWindow("ALPHA CUBE", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    SDL_ShowCursor(SDL_DISABLE);
	if (window == 0)
	{
		exit(1);
		// TODO (Cerisa): Logging
	}

    // Init audio
    IXAudio2SourceVoice* source_voice = NULL;
    {
        IXAudio2* xaudio2 = NULL;
        IXAudio2MasteringVoice* master_voice = NULL;
        game_audio.size = 1024;
        game_audio.depth = 4;
        game_audio.samples_per_tick = 48;

        WAVEFORMATEX wfx = {};
        wfx.wFormatTag = WAVE_FORMAT_IEEE_FLOAT;
        wfx.nChannels = 1;
        wfx.nSamplesPerSec = 48000;
        wfx.wBitsPerSample = 32;
        wfx.nBlockAlign = (wfx.nChannels * wfx.wBitsPerSample) / 8;
        wfx.nAvgBytesPerSec = (wfx.nChannels * wfx.nSamplesPerSec * wfx.wBitsPerSample) / 8;
        wfx.cbSize = 0;

        if (FAILED(XAudio2Create(&xaudio2, 0, XAUDIO2_DEFAULT_PROCESSOR))) return 1;
        if (FAILED(xaudio2->CreateMasteringVoice(&master_voice))) return 1;
        if (FAILED(xaudio2->CreateSourceVoice(&source_voice, (WAVEFORMATEX*)&wfx))) return 1;

        source_voice->Start(0, 0);
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
	    snprintf(error, sizeof(error), "CreateTextureFromSurface failed: %s\n", SDL_GetError());
		OutputDebugStringA(error);
		exit(1);
	}
    render_buffer.texture = texture;

	buffer.w = SCREEN_WIDTH ;
	buffer.h = SCREEN_HEIGHT;
	buffer.depth = 4;
	buffer.pitch = buffer.depth * buffer.w;
    render_buffer.pitch = buffer.pitch;

#if ALPHA_CUBE_INTERNAL
	base_address = 0;
#else
	base_address = (LPVOID)Terabytes(2);
#endif

	game_memory.permanent_storage_size = Megabytes(6);
    game_memory.transient_storage_size = Megabytes(6);
    game_memory.DEBUGPlatformFreeFileMemory = DebugWin_FreeFileMemory;
    game_memory.DEBUGPlatformReadEntireFile = DEBUGPlatformReadEntireFile;
    game_memory.DEBUGPlatformWriteEntireFile = DEBUGPlatformWriteEntireFile;

	state.total_size = game_memory.permanent_storage_size + game_memory.transient_storage_size;
	state.game_memory_block = VirtualAlloc(base_address, (size_t)state.total_size, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
	game_memory.permanent_storage = state.game_memory_block;
	game_memory.transient_storage = ((uint8 *)game_memory.permanent_storage + game_memory.permanent_storage_size);

	int buffer_memory_size = (buffer.w*buffer.h)*buffer.depth;
	buffer.pixels = VirtualAlloc(0, buffer_memory_size, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
	render_buffer.pixels = VirtualAlloc(0, buffer_memory_size, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
	giffer.buffer.pixels = VirtualAlloc(0, buffer_memory_size, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
    giffer.buffer.w = buffer.w;
    giffer.buffer.h = buffer.h;

    int game_audio_buffer_size = (game_audio.size * game_audio.depth);
    game_audio.stream = VirtualAlloc(0, game_audio_buffer_size, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
    void* audio_output_buffers[AUDIO_BUFFERS];
    for (int i=0; i < AUDIO_BUFFERS; i++)
    {
        audio_output_buffers[i] =  (void*) VirtualAlloc(0, game_audio_buffer_size, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
    }


    if (FileSize("spritemap.sm") == 0)
    {
        void* memory = malloc(4069);
        memset(memory, 0, 4096);
        THREAD_CONTEXT dummy_thread = {};
        DEBUGPlatformWriteEntireFile(&dummy_thread, "spritemap.sm", 4096, memory);
    }
    if (FileSize("debug_spritemap.sm") == 0)
    {
        void* memory = malloc(4069);
        memset(memory, 0, 4096);
        THREAD_CONTEXT dummy_thread = {};
        DEBUGPlatformWriteEntireFile(&dummy_thread, "debug_spritemap.sm", 4096, memory);
    }

	game_is_running = true;
	render_buffer.last_frame_time = SDL_GetTicks();
    timer = StartProfiler();

    double fps_tracker[10] = {};
    int fps_tracker_index = 0;
    int frames = 0;
    uint32 last_sound_time = SDL_GetTicks();
    int current_audio_buffer = 0;
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
        new_input->mouse_x = new_input->mouse_x;
        new_input->mouse_y = new_input->mouse_y;
        new_input->primary_click.transitions = old_input->primary_click.transitions;
        new_input->primary_click.is_down = old_input->primary_click.is_down;
		for (int i=0; i < ArrayCount(new_keyboard->buttons); i++)
		{
			new_keyboard->buttons[i].transitions = old_keyboard->buttons[i].transitions;
			new_keyboard->buttons[i].is_down = old_keyboard->buttons[i].is_down;
        }
        for (int i=old_input->key_buffer_position; i != (old_input->key_buffer_position + old_input->key_buffer_length) % 256; i = (i + 1) % 256)
        {
            new_input->key_buffer[i] = old_input->key_buffer[i];
        }
        new_input->key_buffer_length = old_input->key_buffer_length;
        new_input->key_buffer_position = old_input->key_buffer_position;

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
							Win_LogKeyPress(&(new_keyboard->right), &event);
						} break;
						case SDLK_a:
						{
							Win_LogKeyPress(&(new_keyboard->left), &event);
						} break;
                        case SDLK_j:
                        case SDLK_LSHIFT:
						{
							Win_LogKeyPress(&(new_keyboard->b), &event);
						} break;
						case SDLK_SPACE:
						case SDLK_k:
						{
							Win_LogKeyPress(&(new_keyboard->a), &event);
						} break;
                        case SDLK_F9:
                        {
                            Win_LogKeyPress(&(new_input->record_gif), &event);
                        } break;
                        case SDLK_ESCAPE:
                        {
							Win_LogKeyPress(&(new_keyboard->escape), &event);
                        } break;
                    }
                    if (event.key.type == SDL_KEYDOWN)
                    {
                        if (new_input->key_buffer_length < 256)
                        {
                            int position = new_input->key_buffer_length + new_input->key_buffer_position; 
                            position %= 256;
                            new_input->key_buffer[position] = event.key.keysym.sym;
                            new_input->key_buffer_length++;
                        }
                    }
                } break;
                case SDL_MOUSEBUTTONDOWN:
                case SDL_MOUSEBUTTONUP:
				{
                    switch(event.button.button)
                    {
                        case SDL_BUTTON_LEFT:
                        {
                            GAME_BUTTON_STATE* button = &new_input->primary_click;
                            if (button->is_down != (event.button.state))
                            {
                                button->transitions++;
                            }
                        }
                    }
                } break;
                default:
                {

                }
			}
		}

        {
            GAME_CONTROLLER_INPUT *keyboard = &(new_input->controllers[0]);
            for (int i=0; i < ArrayCount(keyboard->buttons); i++)
            {
                GAME_BUTTON_STATE *button = &(keyboard->buttons[i]);
                button->was_pressed = 0;
                button->was_released = 0;
                if (button->transitions > 0)
                {
                    button->transitions--;
                    button->is_down = !button->is_down;
                    if (button->is_down) button->was_pressed = 1;
                    else button->was_released = 1;
                }
            }
            {
                GAME_BUTTON_STATE *button = &(new_input->primary_click);
                button->was_pressed = 0;
                button->was_released = 0;
                if (button->transitions > 0)
                {
                    button->transitions--;
                    button->is_down = !button->is_down;
                    if (button->is_down) button->was_pressed = 1;
                    else button->was_released = 1;
                }
            }
            {
                GAME_BUTTON_STATE *button = &(new_input->record_gif);
                if (button->transitions >= 1)
                {
                    button->transitions--;
                    button->is_down = !button->is_down;
                    if (button->is_down)
                    {
                        recording_gif = !recording_gif;
                    }
                }
            }
        }

        {
            int x, y;
            uint32 buttons = SDL_GetMouseState(&x, &y);
            new_input->mouse_y = buffer.h - y;
            new_input->mouse_x = x;
        }

		new_dll_write_time = Win_GetLastWriteTime(source_game_code_dll_full_path);
		if (CompareFileTime(&new_dll_write_time, &game.dll_last_write_time) != 0)
		{

            WIN32_FILE_ATTRIBUTE_DATA ignored;
            if (!GetFileAttributesEx(game_code_lock_full_path, GetFileExInfoStandard, &ignored))
            {
                Win_UnloadGameCode(&game);
                game = Win_LoadGameCode(source_game_code_dll_full_path, temp_game_code_dll_full_path);
                Assert(game.is_valid)
            }
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
        // {
        //     fps = (1.0 / (fps / ArrayCount(fps_tracker))) * 1000.0;
        //     char out_buffer[256];
        //     snprintf(out_buffer, sizeof(out_buffer), "           %.02f fps\n", fps);
        //     OutputDebugStringA(out_buffer);
        // }
        THREAD_CONTEXT thread = {};
		game_is_running = game_is_running && game.UpdateAndRender(&thread, &game_memory, &buffer, new_input, SCREEN_TICKS_PER_FRAME);

        // Audio
        {
            XAUDIO2_VOICE_STATE voice_state;
            source_voice->GetState(&voice_state);
            uint32 queued_audio = voice_state.BuffersQueued;
            while (queued_audio < AUDIO_BUFFERS)
            {
                uint32 current_sound_time = SDL_GetTicks();
                uint32 sound_time_elapsed = current_sound_time - last_sound_time;
                if (sound_time_elapsed)
                {
                    sound_time_elapsed = SCREEN_TICKS_PER_FRAME;
                }
                game.GetSoundSamples(&thread, &game_memory, &game_audio, sound_time_elapsed);


                memcpy(audio_output_buffers[current_audio_buffer], game_audio.stream, game_audio.size * game_audio.depth);

                XAUDIO2_BUFFER buffer = {0};
                buffer.AudioBytes = game_audio.size * game_audio.depth;
                buffer.pAudioData = (byte*)audio_output_buffers[current_audio_buffer];
                HRESULT hr = source_voice->SubmitSourceBuffer(&buffer);

                source_voice->GetState(&voice_state);
                queued_audio = voice_state.BuffersQueued;

                last_sound_time = current_sound_time;
                current_audio_buffer = (current_audio_buffer + 1) % AUDIO_BUFFERS;
            }
        }

        uint32 delta_time;

        // Video
        {
            if (render_thread != NULL)
            {
                SDL_WaitThread(render_thread, &thread_return_value);
                SDL_RenderPresent(render_buffer.renderer);
                render_buffer.last_frame_time = SDL_GetTicks();
            }
            memcpy(render_buffer.pixels, buffer.pixels, buffer_memory_size);
            render_thread = SDL_CreateThread(Win_Render, "Render", (void*) (&render_buffer));
        }

        // Giffer
        if (frames % GIFFER_FRAME_RATE == 0)
        {
            if (giffer_thread != NULL)
            {
                SDL_WaitThread(giffer_thread, &thread_return_value);
            }
            if (recording_gif)
            {
                if (giffer.memory == NULL)
                {
                    giffer.memory = Giffer_Init(SCREEN_WIDTH, SCREEN_HEIGHT, 1, (uint16)(SCREEN_TICKS_PER_FRAME * GIFFER_FRAME_RATE / 10));
                }
                else
                {
                    memcpy(giffer.buffer.pixels, buffer.pixels, buffer_memory_size);
                    giffer_thread = SDL_CreateThread(Win_Giffer, "Giffer", (void*) (&giffer));
                }
            }
            else if(giffer.memory != NULL)
            {
                Giffer_End(giffer.memory);
                giffer.memory = NULL;
                giffer_thread = NULL;
            }
        }

        {
            uint32 current_frame_time, time_to_wait;
            
            time_to_wait = 0;
            current_frame_time = SDL_GetTicks();
            delta_time = current_frame_time - render_buffer.last_frame_time;
            // {
            //     char text[256];
            //     snprintf(text, sizeof(text), " delta_time: %d\n", delta_time);
            //     OutputDebugStringA(text);
            // }
            if (delta_time < SCREEN_TICKS_PER_FRAME)
            {
                time_to_wait = SCREEN_TICKS_PER_FRAME - delta_time;
                SDL_Delay(time_to_wait);
                current_frame_time = SDL_GetTicks();
                delta_time = current_frame_time - render_buffer.last_frame_time;
            }
            // {
            //     char text[256];
            //     snprintf(text, sizeof(text), "time_waited: %d\n",time_to_wait);
            //     OutputDebugStringA(text);
            // }
        }

        frames++;
		{
			GAME_INPUT* tmp = old_input;
			old_input = new_input;
			new_input = tmp;
		}
		OutputDebugStringA("\n\n");
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
    for (int i=0; i < AUDIO_BUFFERS; i++)
    {
        VirtualFree(audio_output_buffers[i], 0, MEM_RELEASE);
    }
	return 0;
}
