#define _WIN32_WINNT	0x600

#include <windows.h>
#include <winbase.h>
#include <stdio.h>
#include <io.h>
#include <stdint.h>

//#define DEBUG

// It conflicts with my names in the XxxFile classes...
#undef CreateFile

#include "steam_api.h"
#include "sssspatcher.h"
#include "patch.h"
#include "debug.h"
#include "symbols.h"
#include "Utils.h"
#include "IniFile.h"
#include "CpkFile.h"

#include "Dimps/EmbFile.h"
#include "SSSS/CmsFile.h"
#include "SSSS/CdcFile.h"
#include "SSSS/CspFile.h"
#include "SSSS/RpdFile.h"
#include "SSSS/SlotsFile.h"
#include "SSSS/GpdFile.h"
#include "SSSS/GwdFile.h"
#include "SSSS/BgrFile.h"
#include "SSSS/BscFile.h"
#include "SSSS/TdbFile.h"
#include "SSSS/SstFile.h"
#include "SSSS/CncFile.h"
#include "SSSS/ChcFile.h"
#include "SSSS/GrcFile.h"
#include "SSSS/GtcFile.h"
#include "SSSS/VscFile.h"

#include "blobs.h"

#define EXPORT WINAPI __declspec(dllexport)

#define NUM_EXPORTED_FUNCTIONS	18

const char *gSteamExportsNames[NUM_EXPORTED_FUNCTIONS] =
{
	"SteamAPI_SetMiniDumpComment",
	"SteamAPI_WriteMiniDump",
	"SteamUserStats",
	"SteamAPI_UnregisterCallResult",
	"SteamAPI_RegisterCallResult",
	"SteamAPI_RunCallbacks",
	"SteamAPI_RegisterCallback",
	"SteamAPI_UnregisterCallback",
	"SteamMatchmaking",
	"SteamUtils",
	"SteamUser",
	"SteamFriends",
	"SteamRemoteStorage",
	"SteamClient",
	"SteamAPI_Init",
	"SteamAPI_Shutdown",
	"SteamNetworking",
	"SteamApps"
};

DummyFunction gMyExports[NUM_EXPORTED_FUNCTIONS] =
{
	SteamAPI_SetMiniDumpComment,
	SteamAPI_WriteMiniDump,
	SteamUserStats,
	SteamAPI_UnregisterCallResult,
	SteamAPI_RegisterCallResult,
	SteamAPI_RunCallbacks,
	SteamAPI_RegisterCallback,
	SteamAPI_UnregisterCallback,
	SteamMatchmaking,
	SteamUtils,
	SteamUser,
	SteamFriends,
	SteamRemoteStorage,
	SteamClient,
	SteamAPI_Init,
	SteamAPI_Shutdown,
	SteamNetworking,
	SteamApps
};

static HANDLE gOrigSteam;

static void **readfile_import;
static void *original_readfile;

static uint8_t *models_data;
static unsigned int num_models = 0x9A;

// Config
bool do_controller_patch = false;
bool do_unlock_patch = false;

uint32_t controller_index;

// End config

uint8_t (* __thiscall cpk_file_exists)(void *, char *);
bool (* is_character_unlocked)(uint32_t cms_entry, uint32_t model_spec_idx);

#if defined(SSSS) 

DWORD (WINAPI *XInputGetState)(DWORD dwUserIndex, void *pState);

#endif

static BOOL InGameProcess(VOID)
{
	char szPath[MAX_PATH];
	
	GetModuleFileName(NULL, szPath, MAX_PATH);
	_strlwr(szPath);
	
	// A very poor aproach, I know
	return (strstr(szPath, PROCESS_NAME) != NULL);
}

static BOOL LoadDllAndResolveExports(VOID)
{
	char szDll[32];
	
	strcpy(szDll, "steam_api_real.dll");
	
	gOrigSteam = LoadLibrary(szDll);		
	if (!gOrigSteam)
	{
		UPRINTF("I told you to rename original steam_api.dll as steam_api_real.dll!");
		return FALSE;
	}
		
	for (int i = 0; i < NUM_EXPORTED_FUNCTIONS; i++)
	{
		PVOID orig_func = (PVOID)GetProcAddress((HMODULE)gOrigSteam, gSteamExportsNames[i]);
		if (!orig_func)
		{
			DPRINTF("Failed to get original function: %s\n", gSteamExportsNames[i]);
			return FALSE;
		}
		
		uint8_t *my_func = (uint8_t *)gMyExports[i];	
		if (*my_func != 0xB8) // Not what we expected
		{
			DPRINTF("Eeer hmmm, seems like this is not the real address of the function.\n");
		}
		
		write_mem32(my_func+1, (DWORD)orig_func);	// my func+1 = the operand part in mov eax, 0x12345678 
	}
	
	return TRUE;
}

static VOID UnloadDll(VOID)
{
	if (gOrigSteam)
	{
		FreeLibrary((HMODULE)gOrigSteam);
		gOrigSteam = NULL;
	}
}

static uint8_t *read_file_from(const char *file,  uint64_t offset, unsigned int *psize)
{
	HANDLE hFile;
	LONG high;
	uint8_t *buf;
	
	// Microsoft and their 5000 parameter functions...
	hFile = CreateFileA(file, 
						GENERIC_READ, 
						FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE, 
						NULL,
						OPEN_EXISTING,
						0,
						NULL);
						
	if (hFile == INVALID_HANDLE_VALUE)
		return NULL;
	
	high = (offset>>32);
	if (SetFilePointer(hFile, offset&0xFFFFFFFF, &high, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
	{
		CloseHandle(hFile);
		return NULL;		
	}
		
	buf = new uint8_t[*psize];
	if (!buf)
	{
		CloseHandle(hFile);
		return NULL;
	}
	
	if (!ReadFile(hFile, buf, *psize, (LPDWORD)psize, NULL))
	{
		delete[] buf;
		CloseHandle(hFile);
		return NULL;
	}
	
	CloseHandle(hFile);						
	return buf;
}

static CpkFile *get_cpk_toc(const char *file, uint64_t *toc_offset, uint64_t *toc_size, uint8_t **hdr_buf, uint8_t **toc_buf)
{
	unsigned rsize;
	bool success = false;
	
	*hdr_buf = NULL;
	*toc_buf = NULL;
	
	rsize = 0x800;
	*hdr_buf = read_file_from(file, 0, &rsize);
	if (!(*hdr_buf))
		return NULL;
	
	CpkFile *cpk = new CpkFile();
	
	if (!cpk->ParseHeaderData(*hdr_buf))
	{
		goto clean;
	}
	
	*toc_offset = cpk->GetTocOffset();
	*toc_size = cpk->GetTocSize();
	
	if (*toc_offset == (uint64_t)-1 || *toc_size == 0)
	{
		goto clean;
	}
	
	rsize = *toc_size;
	*toc_buf = read_file_from(file, *toc_offset, &rsize);
	
	if (!(*toc_buf))
	{
		DPRINTF("read_file_from failed (2)\n");
		delete[] *toc_buf;
		goto clean;
	}
	
	if (rsize != *toc_size)
	{
		DPRINTF("Warning: read size doesn't match requested size.\n");
	}
	
	if (!cpk->ParseTocData(*toc_buf))
	{
		goto clean;
	}	
	
	DPRINTF("This .cpk has %d files.\n", cpk->GetNumFiles());
	success = true;
	
clean:

	if (!success)
	{
		if (*hdr_buf)
		{
			delete[] *hdr_buf;
			*hdr_buf = NULL;
		}
		
		if (*toc_buf)
		{
			delete[] *toc_buf;
			*toc_buf = NULL;
		}
		
		delete cpk;
		cpk = NULL;
	}
	
	return cpk;
}

static bool get_cpk_tocs(CpkFile **cpks)
{
	for (int i = 0; i < NUM_CPK; i++)
	{
		cpks[i] = get_cpk_toc(cpk_defs[i].name, &cpk_defs[i].toc_offset, &cpk_defs[i].toc_size, &cpk_defs[i].hdr_buf, &cpk_defs[i].toc_buf);

		if (!cpks[i])
		{
			for (int j = 0; j < i; j++)
			{
				if (cpk_defs[j].hdr_buf)
				{
					delete[] cpk_defs[j].hdr_buf;
					cpk_defs[j].hdr_buf = NULL;
				}
				
				if (cpk_defs[j].toc_buf)
				{
					delete[] cpk_defs[j].toc_buf;
					cpk_defs[j].toc_buf = NULL;
				}
				
				delete cpks[j];
				cpks[j] = NULL;
			}			
			
			return false;
		}
		
		cpk_defs[i].patched = false;
		
		DPRINTF("%s.toc = %I64x, size = %I64x\n", cpk_defs[i].name, cpk_defs[i].toc_offset, cpk_defs[i].toc_size);
	}
	
	return true;
}

static bool local_file_exists( char *path)
{
	HANDLE hFind;
	WIN32_FIND_DATA wfd;
	
	hFind = FindFirstFile(path, &wfd);
	if (hFind == INVALID_HANDLE_VALUE)
		return false;
	
	FindClose(hFind);	
	return true;
}

static bool local_file_exists(FileEntry *entry)
{
	char *path;
	int len;
	
	len = strlen(entry->file_name) + 1;
	
	if (entry->dir_name)
	{
		len += (strlen(entry->dir_name) + 1); // also make room for /
	}
	else
	{
		len++;
	}
		
	path = new char[len];
	if (!path)
		return false;
	
	if (entry->dir_name)
		sprintf(path, "%s/%s", entry->dir_name, entry->file_name); // Should be safe... should...
	else
		strcpy(path, entry->file_name);
	
	bool ret = local_file_exists(path);
	delete[] path;
	
	return ret;
}

static void patch_toc(CpkFile *cpk)
{
	int count = 0;
	size_t num_files = cpk->GetNumFiles();
	
	for (size_t i = 0; i < num_files; i++)
	{
		FileEntry *file = cpk->GetFileAt(i);
		
		if (local_file_exists(file))
		{
			if (file->dir_name)
				cpk->UnlinkFileFromDirectory(i);
			else
				cpk->UnlinkFilename(i);
			
			count++;
		}
	}
	
	DPRINTF("%d files deleted in RAM.\n", count);
}

static bool IsThisFile(HANDLE hFile, const char *name)
{
	static char path[MAX_PATH+1];
	
	memset(path, 0, sizeof(path));
	
	if (GetFinalPathNameByHandle(hFile, path, sizeof(path)-1, FILE_NAME_NORMALIZED) != 0)
	{
		_strlwr(path);
		return (strstr(path, name) != NULL);
	}
	
	return false;
}

static uint64_t GetFilePointer(HANDLE hFile)
{
	LONG high = 0;	
	DWORD ret = SetFilePointer(hFile, 0, &high, FILE_CURRENT);
	
	if (ret == INVALID_SET_FILE_POINTER)
		return (uint64_t)-1;
	
	return (((uint64_t)high << 32) | (uint64_t)ret);
}

static inline bool everything_patched()
{
	for (int i = 0; i < NUM_CPK; i++)
	{
		if (!cpk_defs[i].patched)
			return false;
	}
	
	return true;
}

static BOOL WINAPI ReadFile_patched(HANDLE hFile, LPVOID lpBuffer, DWORD nNumberOfBytesToRead, LPDWORD lpNumberOfBytesRead, LPOVERLAPPED lpOverlapped)
{
	if (everything_patched())
	{
		DPRINTF("Main patch is finished. Unhooking function.\n");
		write_mem32((void *)readfile_import, (uint32_t)original_readfile);
		
		for (int i = 0; i < NUM_CPK; i++)
		{
			delete[] cpk_defs[i].hdr_buf;
			delete[] cpk_defs[i].toc_buf;
			
			cpk_defs[i].hdr_buf = NULL;
			cpk_defs[i].toc_buf = NULL;
		}		
	}
	else
	{		
		
		for (int i = 0; i < NUM_CPK; i++)
		{			
			if (IsThisFile(hFile, cpk_defs[i].name))
			{
				if (nNumberOfBytesToRead == 0x800)
				{
					if (GetFilePointer(hFile) == 0)
					{
						memcpy(lpBuffer, cpk_defs[i].hdr_buf, nNumberOfBytesToRead);
						SetFilePointer(hFile, nNumberOfBytesToRead, NULL, FILE_CURRENT);
						
						if (lpNumberOfBytesRead)
						{
							*lpNumberOfBytesRead = nNumberOfBytesToRead;
						}
					
						DPRINTF("%s HDR patched.\n", cpk_defs[i].name);						
						return TRUE;
					}
				}
				
				if (nNumberOfBytesToRead == cpk_defs[i].toc_size)
				{
					if (GetFilePointer(hFile) == cpk_defs[i].toc_offset)
					{
						memcpy(lpBuffer, cpk_defs[i].toc_buf, nNumberOfBytesToRead);
						SetFilePointer(hFile, nNumberOfBytesToRead, NULL, FILE_CURRENT);
					
						if (lpNumberOfBytesRead)
						{
							*lpNumberOfBytesRead = nNumberOfBytesToRead;
						}
					
						DPRINTF("%s TOC patched.\n", cpk_defs[i].name);	
						cpk_defs[i].patched = true;
						return TRUE;
					}
				}
			}			
		}		
	}
	
	return ReadFile(hFile, lpBuffer, nNumberOfBytesToRead, lpNumberOfBytesRead, lpOverlapped);
}

#ifndef XENOVERSE

static uint8_t __thiscall cpk_file_exists_patched(void *object, char *file)
{
	uint8_t ret = cpk_file_exists(object, file);
	
	if (ret == 0)
	{
		//DPRINTF("File exists originally returned 0 (%s)", file);
		return local_file_exists(file);
	}
	
	return ret;
}

#endif

#ifdef DEBUG

void *(* open_cpk_file)(int, const char *, int );

void *my_open_cpk_file(int a1, const char *s, int a3)
{
	void *ret = open_cpk_file(a1, s, a3);
	
	//DPRINTF("open_cpk_file = %s; a1=%x, a3=%x; ret = %p\n", s, a1, a3, ret);	
	
	return ret;
}

#endif

#if defined(SSSS) 

DWORD WINAPI XInputGetState_Patched(DWORD dwUserIndex, void *pState)
{
	if (dwUserIndex == 0)
		dwUserIndex = controller_index;
		
	return XInputGetState(dwUserIndex, pState);	
}

#endif

static int get_model_id(uint32_t cms_entry, uint32_t model_spec_idx)
{
	ModelStruct *data = (ModelStruct *)models_data;
	
	for (uint32_t i = 0; i < num_models; i++)
	{
		//CDCEntry *cdc_entry = (CDCEntry *)data[i].cdc_entry;
		CMSEntry *entry = (CMSEntry *)data[i].cms_entry;
		CMSModelSpec *spec = (CMSModelSpec *)data[i].model_spec;
		
		if (entry->id == cms_entry && spec->idx == model_spec_idx)
		{
			//DPRINTF("%x  %d -> %x.\n", entry->id, spec->idx, spec->model_id);
			return spec->model_id;
		}
	}
	
	return -1;
}

bool is_character_unlocked_patched(uint32_t cms_entry, uint32_t model_spec_idx)
{
	if (do_unlock_patch)
	{
		if (cms_entry != 0x6F)
			return true; // all unlocked

		return is_character_unlocked(cms_entry, model_spec_idx);
	}
	
	int id = get_model_id(cms_entry, model_spec_idx);
	
	if (id < ORIGINAL_NUMBER_MODELS)
		return is_character_unlocked(cms_entry, model_spec_idx);
	
	return true; // All of our new characters are unlocked!
}

void build_bs_cache(SlotsFile *slf, BscFile *bsc)
{
	for (uint32_t i = 0; i < slf->GetNumSlots(); i++)
	{
		for (uint32_t j = 0; j < MAX_SLOTS; j++)
		{
			if (!slf->IsCurrentRandomSlot(i) && !slf->IsFreeSlot(i, j))
			{
				uint32_t cms_entry;
				uint32_t cms_model_spec_idx;
				
				if (slf->GetCmsEntry(i, j, &cms_entry, &cms_model_spec_idx))
				{
					std::string name = Utils::SSSSModelFileName(cms_entry, cms_model_spec_idx, ".dds");
					
					if (Utils::FileExists(std::string(AVATARS_DIRECTORY) + name))
					{
						bsc->AppendAvatarEntry(cms_entry, cms_model_spec_idx);
					}
					
					if (Utils::FileExists(std::string(BATTLE_NAMES_DIRECTORY_LEFT) + languages[1] + "/" + name))
					{
						if (Utils::FileExists(std::string(BATTLE_NAMES_DIRECTORY_RIGHT) + languages[1] + "/" + name))
							bsc->AppendBattleNameEntry(cms_entry, cms_model_spec_idx);
					}
				}
			}
		}
	}	
}

bool build_bs_emb(BscFile *bsc, uint32_t *avatar_first_new_entry, uint32_t *bn_left_first_new_entry, uint32_t *bn_right_first_new_entry)
{
	*avatar_first_new_entry = 0;
	*bn_left_first_new_entry = 0;
	*bn_right_first_new_entry = 0;
	
	// EMB time
	for (size_t i = 0; i < battle_steam_base.size(); i++)
	{
		EmbFile emb;
		
		if (!emb.LoadFromFile(battle_steam_base[i]))
			return false;
		
		// Avatars
		for (size_t j = 0; j < bsc->GetNumAvatars(); j++)
		{
			uint32_t cms_entry, cms_model_spec_idx;
			
			if (!bsc->GetAvatarEntry(j, &cms_entry, &cms_model_spec_idx))
				return false;
		
			uint32_t entry;
			uint8_t *buf;
			size_t size;	
			std::string name;
			
			name = Utils::SSSSModelFileName(cms_entry, cms_model_spec_idx, ".dds");
			
			buf = Utils::ReadFile(std::string(AVATARS_DIRECTORY) + name, &size, false);
			if (!buf)
				return false;
			
			entry = emb.AppendFile(buf, size, name);
			delete[] buf;
			
			if (j == 0)
			{
				if (i == 0)
				{
					*avatar_first_new_entry = entry;
				}
				else if (entry != *avatar_first_new_entry)
				{
					DPRINTF("There is a mismatch between the battle_STEAM bases. You shouldn't have added entries to these ones!!!!\n");
					return false;
				}
			}			
		}
		
		// Battle name left
		for (size_t j = 0; j < bsc->GetNumBattleNames(); j++)
		{
			uint32_t cms_entry, cms_model_spec_idx;
			
			if (!bsc->GetBattleNameEntry(j, &cms_entry, &cms_model_spec_idx))
				return false;
			
			uint32_t entry;
			uint8_t *buf;
			size_t size;	
			std::string name;
			
			name = Utils::SSSSModelFileName(cms_entry, cms_model_spec_idx, ".dds");
			
			buf = Utils::ReadFile(std::string(BATTLE_NAMES_DIRECTORY_LEFT) + languages[i] + "/" + name, &size, false);
			if (!buf)
			{
				// If this is english version, return false, this one must exist.
				if (i == 1)
					return false;
				
				// Try to fallback to english version
				buf = Utils::ReadFile(std::string(BATTLE_NAMES_DIRECTORY_LEFT) + languages[1] + "/" + name, &size, false);
				if (!buf)
					return false;
			}
			
			entry = emb.AppendFile(buf, size, name);
			
			if (j == 0)
			{
				if (i == 0)
				{
					*bn_left_first_new_entry = entry;
				}
				else if (entry != *bn_left_first_new_entry)
				{
					DPRINTF("There is a mismatch between the battle_STEAM bases. You shouldn't have added entries to these ones!!!!\n");
					return false;
				}
			}
		}
		
		// Battle name right
		for (size_t j = 0; j < bsc->GetNumBattleNames(); j++)
		{
			uint32_t cms_entry, cms_model_spec_idx;
			
			if (!bsc->GetBattleNameEntry(j, &cms_entry, &cms_model_spec_idx))
				return false;
			
			uint32_t entry;
			uint8_t *buf;
			size_t size;	
			std::string name;
			
			name = Utils::SSSSModelFileName(cms_entry, cms_model_spec_idx, ".dds");
			
			buf = Utils::ReadFile(std::string(BATTLE_NAMES_DIRECTORY_RIGHT) + languages[i] + "/" + name, &size, false);
			if (!buf)
			{
				// If this is english version, return false, this one must exist.
				if (i == 1)
					return false;
				
				// Try to fallback to english version
				buf = Utils::ReadFile(std::string(BATTLE_NAMES_DIRECTORY_RIGHT) + languages[1] + "/" + name, &size, false);
				if (!buf)
					return false;
			}
			
			entry = emb.AppendFile(buf, size, name);
			
			if (j == 0)
			{
				if (i == 0)
				{
					*bn_right_first_new_entry = entry;
				}
				else if (entry != *bn_right_first_new_entry)
				{
					DPRINTF("There is a mismatch between the battle_STEAM bases. You shouldn't have added entries to these ones!!!!\n");
					return false;
				}
			}
		}
		
		
		if (!emb.SaveToFile(battle_steam_out[i], true, true))
			return false;
	}
	
	return true;
}

void build_bs_ems_avatars(uint8_t *buf, size_t new_section_start, size_t num_new_avatars, uint32_t avatar_first_new_entry)
{
	uint32_t left_new_start, right_new_start;
	
	// Left		
	left_new_start = new_section_start;
	uint32_t *id_table = (uint32_t *)(buf + left_new_start);
	uint8_t *mapping = ((uint8_t *)id_table) + (NUM_ORIGINAL_AVATARS+num_new_avatars)*sizeof(uint32_t);
	
	*(uint32_t *)(buf+BS_AVATAR_LEFT_NUM_OFFSET) = NUM_ORIGINAL_AVATARS+num_new_avatars;
	*(uint16_t *)(buf+BS_AVATAR_LEFT_NUM_OFFSET2) = NUM_ORIGINAL_AVATARS+num_new_avatars;
	*(uint16_t *)(buf+BS_AVATAR_LEFT_NUM_OFFSET3) = NUM_ORIGINAL_AVATARS+num_new_avatars;
	
	memcpy(id_table, buf+BS_AVATAR_LEFT_ID_TABLE, NUM_ORIGINAL_AVATARS*sizeof(uint32_t));
	memcpy(mapping, buf+BS_AVATAR_LEFT_MAPPING, NUM_ORIGINAL_AVATARS*0x20);
	
	*(uint32_t *)(buf+BS_AVATAR_LEFT_ID_TABLE_FPTR) = left_new_start;
	*(uint32_t *)(buf+BS_AVATAR_LEFT_MAPPING_FPTR) = left_new_start + (NUM_ORIGINAL_AVATARS+num_new_avatars)*sizeof(uint32_t);
	
	for (unsigned int i = NUM_ORIGINAL_AVATARS; i < (NUM_ORIGINAL_AVATARS+num_new_avatars); i++)
	{
		uint32_t *map = (uint32_t *)(mapping + i*0x20);
		
		id_table[i] = i;
		map[0] = avatar_first_new_entry + (i-NUM_ORIGINAL_AVATARS);
		map[1] = 3;
		map[2] = 0x3F800000; // 1.0
		map[3] = 8;
		map[4] = map[5] = 0;
		map[6] = map[7] = 0x43000000; // 128.0
	}
	
	// Right
	right_new_start = left_new_start + (NUM_ORIGINAL_AVATARS+num_new_avatars)*sizeof(uint32_t);
	right_new_start += (NUM_ORIGINAL_AVATARS+num_new_avatars)*0x20;
	id_table = (uint32_t *)(buf + right_new_start);
	mapping = ((uint8_t *)id_table) + (NUM_ORIGINAL_AVATARS+num_new_avatars)*sizeof(uint32_t);
	
	*(uint32_t *)(buf+BS_AVATAR_RIGHT_NUM_OFFSET) = NUM_ORIGINAL_AVATARS+num_new_avatars;
	*(uint16_t *)(buf+BS_AVATAR_RIGHT_NUM_OFFSET2) = NUM_ORIGINAL_AVATARS+num_new_avatars;
	*(uint16_t *)(buf+BS_AVATAR_RIGHT_NUM_OFFSET3) = NUM_ORIGINAL_AVATARS+num_new_avatars;
	
	memcpy(id_table, buf+BS_AVATAR_RIGHT_ID_TABLE, NUM_ORIGINAL_AVATARS*sizeof(uint32_t));
	memcpy(mapping, buf+BS_AVATAR_RIGHT_MAPPING, NUM_ORIGINAL_AVATARS*0x20);
	
	*(uint32_t *)(buf+BS_AVATAR_RIGHT_ID_TABLE_FPTR) = right_new_start;
	*(uint32_t *)(buf+BS_AVATAR_RIGHT_MAPPING_FPTR) = right_new_start + (NUM_ORIGINAL_AVATARS+num_new_avatars)*sizeof(uint32_t);
	
	for (unsigned int i = NUM_ORIGINAL_AVATARS; i < (NUM_ORIGINAL_AVATARS+num_new_avatars); i++)
	{
		uint32_t *map = (uint32_t *)(mapping + i*0x20);
		
		id_table[i] = i;
		map[0] = avatar_first_new_entry + (i-NUM_ORIGINAL_AVATARS);
		map[1] = 3;
		map[2] = 0x3F800000; // 1.0
		map[3] = 8;
		map[4] = map[5] = 0;
		map[6] = map[7] = 0x43000000; // 128.0
	}
}

void build_bs_ems_battle_names(uint8_t *buf, size_t new_section_start, size_t num_new_battle_names, uint32_t bn_left_first_new_entry, uint32_t bn_right_first_new_entry)
{
	uint32_t left_new_start, right_new_start;
	
	// Left		
	left_new_start = new_section_start;
	uint32_t *id_table = (uint32_t *)(buf + left_new_start);
	uint8_t *mapping = ((uint8_t *)id_table) + (NUM_ORIGINAL_BATTLE_NAMES+num_new_battle_names)*sizeof(uint32_t);
	
	*(uint32_t *)(buf+BS_BN_LEFT_NUM_OFFSET) = NUM_ORIGINAL_BATTLE_NAMES+num_new_battle_names;
	*(uint16_t *)(buf+BS_BN_LEFT_NUM_OFFSET2) = NUM_ORIGINAL_BATTLE_NAMES+num_new_battle_names;
	*(uint16_t *)(buf+BS_BN_LEFT_NUM_OFFSET3) = NUM_ORIGINAL_BATTLE_NAMES+num_new_battle_names;
	
	memcpy(id_table, buf+BS_BN_LEFT_ID_TABLE, NUM_ORIGINAL_BATTLE_NAMES*sizeof(uint32_t));
	memcpy(mapping, buf+BS_BN_LEFT_MAPPING, NUM_ORIGINAL_BATTLE_NAMES*0x20);
	
	*(uint32_t *)(buf+BS_BN_LEFT_ID_TABLE_FPTR) = left_new_start;
	*(uint32_t *)(buf+BS_BN_LEFT_MAPPING_FPTR) = left_new_start + (NUM_ORIGINAL_BATTLE_NAMES+num_new_battle_names)*sizeof(uint32_t);
	
	for (unsigned int i = NUM_ORIGINAL_BATTLE_NAMES; i < (NUM_ORIGINAL_BATTLE_NAMES+num_new_battle_names); i++)
	{
		uint32_t *map = (uint32_t *)(mapping + i*0x20);
		
		id_table[i] = i;
		map[0] = bn_left_first_new_entry + (i-NUM_ORIGINAL_BATTLE_NAMES);
		map[1] = 3;
		map[2] = 0x3F800000; // 1.0
		map[3] = 0xB;
		map[4] = map[5] = 0;
		map[6] = 0x43800000; // 256.0   2/3 of width
		map[7] = 0x42200000; // 40.0	2/3 of height
	}
	
	// Right
	right_new_start = left_new_start + (NUM_ORIGINAL_BATTLE_NAMES+num_new_battle_names)*sizeof(uint32_t);
	right_new_start += (NUM_ORIGINAL_BATTLE_NAMES+num_new_battle_names)*0x20;
	id_table = (uint32_t *)(buf + right_new_start);
	mapping = ((uint8_t *)id_table) + (NUM_ORIGINAL_BATTLE_NAMES+num_new_battle_names)*sizeof(uint32_t);
	
	*(uint32_t *)(buf+BS_BN_RIGHT_NUM_OFFSET) = NUM_ORIGINAL_BATTLE_NAMES+num_new_battle_names;
	*(uint16_t *)(buf+BS_BN_RIGHT_NUM_OFFSET2) = NUM_ORIGINAL_BATTLE_NAMES+num_new_battle_names;
	*(uint16_t *)(buf+BS_BN_RIGHT_NUM_OFFSET3) = NUM_ORIGINAL_BATTLE_NAMES+num_new_battle_names;
	
	memcpy(id_table, buf+BS_BN_RIGHT_ID_TABLE, NUM_ORIGINAL_BATTLE_NAMES*sizeof(uint32_t));
	memcpy(mapping, buf+BS_BN_RIGHT_MAPPING, NUM_ORIGINAL_BATTLE_NAMES*0x20);
	
	*(uint32_t *)(buf+BS_BN_RIGHT_ID_TABLE_FPTR) = right_new_start;
	*(uint32_t *)(buf+BS_BN_RIGHT_MAPPING_FPTR) = right_new_start + (NUM_ORIGINAL_BATTLE_NAMES+num_new_battle_names)*sizeof(uint32_t);
	
	for (unsigned int i = NUM_ORIGINAL_BATTLE_NAMES; i < (NUM_ORIGINAL_BATTLE_NAMES+num_new_battle_names); i++)
	{
		uint32_t *map = (uint32_t *)(mapping + i*0x20);
		
		id_table[i] = i;
		map[0] = bn_right_first_new_entry + (i-NUM_ORIGINAL_BATTLE_NAMES);
		map[1] = 3;
		map[2] = 0x3F800000; // 1.0
		map[3] = 0xB;
		map[4] = map[5] = 0;
		map[6] = 0x43800000; // 256.0    2/3 of width
		map[7] = 0x42200000; // 40.0	 2/3 of height
	}
}

bool build_bs_ems(BscFile *bsc, uint32_t avatar_first_new_entry, uint32_t bn_left_first_new_entry, uint32_t bn_right_first_new_entry)
{
	uint8_t *buf, *out;
	size_t size;
	unsigned int extended_size = 0;
	unsigned int bn_start = 0;
	
	buf = Utils::ReadFile(BATTLE_STEAM_EMS_BASE, &size);
	if (!buf)
		return false;
	
	if (size != BATTLE_STEAM_EMS_SIZE)
	{
		DPRINTF("Don't modify battle_STEAM.ems, retard!\n");
		delete[] buf;
		return false;
	}
	
	size_t num_new_avatars = bsc->GetNumAvatars();
	
	if (num_new_avatars > 0)
	{
		unsigned int extended_size_avatar;
		
		extended_size_avatar = (NUM_ORIGINAL_AVATARS+num_new_avatars)*sizeof(uint32_t);
		extended_size_avatar += (NUM_ORIGINAL_AVATARS+num_new_avatars)*0x20;
		extended_size_avatar *= 2; // Left and right
		
		extended_size += extended_size_avatar;
	}
	
	size_t num_new_battle_names = bsc->GetNumBattleNames();
	
	if (num_new_battle_names > 0)
	{
		unsigned int extended_size_bn;
		
		extended_size_bn = (NUM_ORIGINAL_BATTLE_NAMES+num_new_battle_names)*sizeof(uint32_t);
		extended_size_bn += (NUM_ORIGINAL_BATTLE_NAMES+num_new_battle_names)*0x20;
		extended_size_bn *= 2; // Left and right
		
		bn_start = size + extended_size;
		extended_size += extended_size_bn;
	}
	
	out = new uint8_t[size+extended_size];
	if (!out)
	{
		DPRINTF("%s: Memory allocation error.\n", __PRETTY_FUNCTION__);
		return false;
	}
	
	memcpy(out, buf, size);
	delete[] buf;
	buf = nullptr;	
	
	if (num_new_avatars > 0)
		build_bs_ems_avatars(out, size, num_new_avatars, avatar_first_new_entry);
	
	if (num_new_battle_names > 0)
		build_bs_ems_battle_names(out, bn_start, num_new_battle_names, bn_left_first_new_entry, bn_right_first_new_entry);
	
	bool ret = Utils::WriteFileBool(BATTLE_STEAM_EMS_OUT, out, size+extended_size, true, true);
	delete[] out;
	
	return ret;	
}

bool build_bs_files(BscFile *bsc)
{
	uint32_t avatar_first_new_entry, bn_left_first_new_entry, bn_right_first_new_entry;
	
	if (!build_bs_emb(bsc, &avatar_first_new_entry, &bn_left_first_new_entry, &bn_right_first_new_entry))
		return false;
	
	if (!build_bs_ems(bsc, avatar_first_new_entry, bn_left_first_new_entry, bn_right_first_new_entry))
		return false;	
	
	return true;
}

void process_battle_steam(SlotsFile *slf)
{
	BscFile bsc;
	
	if (!bsc.LoadFromFile(BATTLE_STEAM_CACHE, false))
	{
		build_bs_cache(slf, &bsc);
		
		if (bsc.IsEmpty())
			return;
		
		if (!build_bs_files(&bsc))
			return;
		
		bsc.SaveToFile(BATTLE_STEAM_CACHE, true, true);
	}
	
	for (size_t i = 0; i < bsc.GetNumAvatars(); i++)
	{
		uint32_t cms_entry, cms_model_spec_idx;
		
		if (!bsc.GetAvatarEntry(i, &cms_entry, &cms_model_spec_idx))
		{
			DPRINTF("%s: buuuuuuuuuuuuuuu\n", __PRETTY_FUNCTION__);
			return;
		}
		
		slf->SetAvatarId(cms_entry, cms_model_spec_idx, NUM_ORIGINAL_AVATARS+i);
	}
	
	for (size_t i = 0; i < bsc.GetNumBattleNames(); i++)
	{
		uint32_t cms_entry, cms_model_spec_idx;
		
		if (!bsc.GetBattleNameEntry(i, &cms_entry, &cms_model_spec_idx))
		{
			DPRINTF("%s: buuuuuuuuuuuuuuu\n", __PRETTY_FUNCTION__);
			return;
		}
		
		slf->SetBattleNameId(cms_entry, cms_model_spec_idx, NUM_ORIGINAL_BATTLE_NAMES+i);
	}
}

void build_cn_cache(SlotsFile *slf, CncFile *cnc)
{
	for (uint32_t i = 0; i < slf->GetNumSlots(); i++)
	{
		for (uint32_t j = 0; j < MAX_SLOTS; j++)
		{
			if (!slf->IsCurrentRandomSlot(i) && !slf->IsFreeSlot(i, j))
			{
				uint32_t cms_entry;
				uint32_t cms_model_spec_idx;
				
				if (slf->GetCmsEntry(i, j, &cms_entry, &cms_model_spec_idx))
				{
					SstFile sst;
					std::string name;
					
					name = Utils::SSSSModelFileName(cms_entry, cms_model_spec_idx, ".xml");
					
					if (!sst.CompileFromFile(std::string(CHR_NAME_DIRECTORY) + name, false))
						continue;
					
					if (sst.HasSignName())
					{
						cnc->AppendSignNameEntry(cms_entry, cms_model_spec_idx);
					}
					
					if (sst.HasRobesName())
					{
						cnc->AppendRobesNameEntry(cms_entry, cms_model_spec_idx);
					}					
				}
			}
		}
	}	
}

bool build_cn_files(CncFile *cnc)
{
	TdbFile tdb;
	
	if (!tdb.LoadFromFile(TDB_CHR_NAME_BASE))
		return false;

	for (size_t i = 0; i < cnc->GetNumSignNames(); i++)
	{
		uint32_t cms_entry, cms_model_spec_idx;
		std::string name;
		SstFile sst;
		
		if (!cnc->GetSignNameEntry(i, &cms_entry, &cms_model_spec_idx))
			return false;
		
		name = Utils::SSSSModelFileName(cms_entry, cms_model_spec_idx, ".xml");
					
		if (!sst.CompileFromFile(std::string(CHR_NAME_DIRECTORY) + name, false))
			return false;
		
		if (!sst.HasSignName()) // This shouldnt happen because it was checked when building the cache
			return false; 
			
		std::vector<std::string> sign_names;
		
		sign_names.resize(NUM_LANGUAGES);
		
		for (int j = 0; j < NUM_LANGUAGES; j++)
		{
			sign_names[j] = sst.GetSignName(j);
		}
		
		tdb.AppendString(sign_names);
	}
	
	for (size_t i = 0; i < cnc->GetNumRobesNames(); i++)
	{
		uint32_t cms_entry, cms_model_spec_idx;
		std::string name;
		SstFile sst;
		
		if (!cnc->GetRobesNameEntry(i, &cms_entry, &cms_model_spec_idx))
			return false;
		
		name = Utils::SSSSModelFileName(cms_entry, cms_model_spec_idx, ".xml");
					
		if (!sst.CompileFromFile(std::string(CHR_NAME_DIRECTORY) + name, false))
			return false;
		
		if (!sst.HasRobesName()) // This shouldnt happen because it was checked when building the cache
			return false; 
			
		std::vector<std::string> robes_names;
		
		robes_names.resize(NUM_LANGUAGES);
		
		for (int j = 0; j < NUM_LANGUAGES; j++)
		{
			robes_names[j] = sst.GetRobesName(j);
		}
		
		tdb.AppendString(robes_names);
	}
	
	return tdb.SaveToFile(TDB_CHR_NAME_OUT, true, true);
}

void process_chr_name(SlotsFile *slf)
{
	CncFile cnc;
	
	if (!cnc.LoadFromFile(CHR_NAME_CACHE, false))
	{
		build_cn_cache(slf, &cnc);
				
		if (cnc.IsEmpty())
			return;		
		
		if (!build_cn_files(&cnc))
			return;		
		
		cnc.SaveToFile(CHR_NAME_CACHE, true, true);
	}	
	
	uint32_t current_tdb_entry = NUM_ORIGINAL_CN;
	
	for (size_t i = 0; i < cnc.GetNumSignNames(); i++)
	{
		uint32_t cms_entry, cms_model_spec_idx;
		
		if (!cnc.GetSignNameEntry(i, &cms_entry, &cms_model_spec_idx))
		{
			DPRINTF("%s: buuuuuuuuuuuuuuu\n", __PRETTY_FUNCTION__);
			return;
		}
		
		slf->SetSignNameId(cms_entry, cms_model_spec_idx, current_tdb_entry);
		current_tdb_entry++;
	}
	
	for (size_t i = 0; i < cnc.GetNumRobesNames(); i++)
	{
		uint32_t cms_entry, cms_model_spec_idx;
		
		if (!cnc.GetRobesNameEntry(i, &cms_entry, &cms_model_spec_idx))
		{
			DPRINTF("%s: buuuuuuuuuuuuuuu\n", __PRETTY_FUNCTION__);
			return;
		}
		
		slf->SetRobesNameId(cms_entry, cms_model_spec_idx, current_tdb_entry);
		current_tdb_entry++;
	}
}

void build_cs_cache(SlotsFile *slf, ChcFile *chc)
{
	for (uint32_t i = 0; i < slf->GetNumSlots(); i++)
	{
		for (uint32_t j = 0; j < MAX_SLOTS; j++)
		{
			if (!slf->IsCurrentRandomSlot(i) && !slf->IsFreeSlot(i, j))
			{
				uint32_t cms_entry;
				uint32_t cms_model_spec_idx;
				
				if (slf->GetCmsEntry(i, j, &cms_entry, &cms_model_spec_idx))
				{
					std::string name = Utils::SSSSModelFileName(cms_entry, cms_model_spec_idx, ".dds");
					
					if (Utils::FileExists(std::string(ICONS_DIRECTORY_BIG) + name))
					{
						if (Utils::FileExists(std::string(ICONS_DIRECTORY_SMALL) + name))
							chc->AppendIconEntry(cms_entry, cms_model_spec_idx);
					}

					if (Utils::FileExists(std::string(SN_DIRECTORY) + languages[1] + "/" + name))
					{
						chc->AppendSelectNameEntry(cms_entry, cms_model_spec_idx);
					}
					
					if (Utils::FileExists(std::string(SN2_DIRECTORY) + languages[1] + "/" + name))
					{
						chc->AppendSelect2NameEntry(cms_entry, cms_model_spec_idx);
					}
				}
			}
		}
	}	
}

bool build_cs_emb(ChcFile *chc, uint32_t *icon_big_first_new_entry, uint32_t *icon_small_first_new_entry, uint32_t *sn_first_new_entry, uint32_t *sn2_first_new_entry)
{
	*icon_big_first_new_entry = 0;
	*icon_small_first_new_entry = 0;
	*sn_first_new_entry = 0;
	*sn2_first_new_entry = 0;
	
	for (size_t i = 0; i < cha_sel_base.size(); i++)
	{
		EmbFile emb;
		
		if (!emb.LoadFromFile(cha_sel_base[i]))
			return false;
		
		// Icons big
		for (size_t j = 0; j < chc->GetNumIcons(); j++)
		{
			uint32_t cms_entry, cms_model_spec_idx;
			
			if (!chc->GetIconEntry(j, &cms_entry, &cms_model_spec_idx))
				return false;
		
			uint32_t entry;
			uint8_t *buf;
			size_t size;	
			std::string name;
			
			name = Utils::SSSSModelFileName(cms_entry, cms_model_spec_idx, ".dds");
			
			buf = Utils::ReadFile(std::string(ICONS_DIRECTORY_BIG) + name, &size, false);
			if (!buf)
				return false;
			
			entry = emb.AppendFile(buf, size, name);
			delete[] buf;
			
			if (j == 0)
			{
				if (i == 0)
				{
					*icon_big_first_new_entry = entry;
				}
				else if (entry != *icon_big_first_new_entry)
				{
					DPRINTF("There is a mismatch between the cha_sel bases. You shouldn't have added entries to these ones!!!!\n");
					return false;
				}
			}			
		}

		// Icons small
		for (size_t j = 0; j < chc->GetNumIcons(); j++)
		{
			uint32_t cms_entry, cms_model_spec_idx;
			
			if (!chc->GetIconEntry(j, &cms_entry, &cms_model_spec_idx))
				return false;
		
			uint32_t entry;
			uint8_t *buf;
			size_t size;	
			std::string name;
			
			name = Utils::SSSSModelFileName(cms_entry, cms_model_spec_idx, ".dds");
			
			buf = Utils::ReadFile(std::string(ICONS_DIRECTORY_SMALL) + name, &size, false);
			if (!buf)
				return false;
			
			entry = emb.AppendFile(buf, size, name);
			delete[] buf;
			
			if (j == 0)
			{
				if (i == 0)
				{
					*icon_small_first_new_entry = entry;
				}
				else if (entry != *icon_small_first_new_entry)
				{
					DPRINTF("There is a mismatch between the cha_sel bases. You shouldn't have added entries to these ones!!!!\n");
					return false;
				}
			}			
		}

		// Select names
		for (size_t j = 0; j < chc->GetNumSelectNames(); j++)
		{
			uint32_t cms_entry, cms_model_spec_idx;
			
			if (!chc->GetSelectNameEntry(j, &cms_entry, &cms_model_spec_idx))
				return false;
			
			uint32_t entry;
			uint8_t *buf;
			size_t size;	
			std::string name;
			
			name = Utils::SSSSModelFileName(cms_entry, cms_model_spec_idx, ".dds");
			
			buf = Utils::ReadFile(std::string(SN_DIRECTORY) + languages[i] + "/" + name, &size, false);
			if (!buf)
			{
				// If this is english version, return false, this one must exist.
				if (i == 1)
					return false;
				
				// Try to fallback to english version
				buf = Utils::ReadFile(std::string(SN_DIRECTORY) + languages[1] + "/" + name, &size, false);
				if (!buf)
					return false;
			}
			
			entry = emb.AppendFile(buf, size, name);
			
			if (j == 0)
			{
				if (i == 0)
				{
					*sn_first_new_entry = entry;
				}
				else if (entry != *sn_first_new_entry)
				{
					DPRINTF("There is a mismatch between the cha_sel bases. You shouldn't have added entries to these ones!!!!\n");
					return false;
				}
			}
		}
		
		// Select2 names
		for (size_t j = 0; j < chc->GetNumSelect2Names(); j++)
		{
			uint32_t cms_entry, cms_model_spec_idx;
			
			if (!chc->GetSelect2NameEntry(j, &cms_entry, &cms_model_spec_idx))
				return false;
			
			uint32_t entry;
			uint8_t *buf;
			size_t size;	
			std::string name;
			
			name = Utils::SSSSModelFileName(cms_entry, cms_model_spec_idx, ".dds");
			
			buf = Utils::ReadFile(std::string(SN2_DIRECTORY) + languages[i] + "/" + name, &size, false);
			if (!buf)
			{
				// If this is english version, return false, this one must exist.
				if (i == 1)
					return false;
				
				// Try to fallback to english version
				buf = Utils::ReadFile(std::string(SN2_DIRECTORY) + languages[1] + "/" + name, &size, false);
				if (!buf)
					return false;
			}
			
			entry = emb.AppendFile(buf, size, name);
			
			if (j == 0)
			{
				if (i == 0)
				{
					*sn2_first_new_entry = entry;
				}
				else if (entry != *sn2_first_new_entry)
				{
					DPRINTF("There is a mismatch between the cha_sel bases. You shouldn't have added entries to these ones!!!!\n");
					return false;
				}
			}
		}
		
		if (!emb.SaveToFile(cha_sel_out[i], true, true))
			return false;
	}
	
	return true;	
}

void build_cs_ems_icons(uint8_t *buf, size_t new_section_start, size_t num_new_icons, uint32_t icon_big_first_new_entry, uint32_t icon_small_first_new_entry)
{
	uint32_t big_new_start, small_new_start;
	uint32_t total_big_icons = CS_NUM_ORIGINAL_ICONS_BIG+num_new_icons;
	//uint32_t total_small_icons = NUM_ORIGINAL_ICONS_SMALL+num_new_icons;
	
	// Big
	big_new_start = new_section_start;
	uint32_t *id_table = (uint32_t *)(buf + big_new_start);
	uint8_t *mapping = ((uint8_t *)id_table) + total_big_icons*sizeof(uint32_t);	
	
	*(uint32_t *)(buf+CS_ICON_BIG_NUM_OFFSET) = total_big_icons;
	*(uint16_t *)(buf+CS_ICON_BIG_NUM_OFFSET2) = total_big_icons;
	*(uint16_t *)(buf+CS_ICON_BIG_NUM_OFFSET3) = total_big_icons;
	
	memcpy(id_table, buf+CS_ICON_BIG_ID_TABLE, CS_NUM_ORIGINAL_ICONS_BIG*sizeof(uint32_t));
	memcpy(mapping, buf+CS_ICON_BIG_MAPPING, CS_NUM_ORIGINAL_ICONS_BIG*0x20);
	
	*(uint32_t *)(buf+CS_ICON_BIG_ID_TABLE_FPTR) = big_new_start;
	*(uint32_t *)(buf+CS_ICON_BIG_MAPPING_FPTR) = big_new_start + total_big_icons*sizeof(uint32_t);
	
	for (unsigned int i = CS_NUM_ORIGINAL_ICONS_BIG; i < total_big_icons; i++)
	{
		uint32_t *map = (uint32_t *)(mapping + i*0x20);
		
		id_table[i] = i;
		map[0] = icon_big_first_new_entry + (i-CS_NUM_ORIGINAL_ICONS_BIG);
		map[1] = 3;
		map[2] = 0x3F800000; // 1.0
		map[3] = 8;
		map[4] = map[5] = 0;
		map[6] = map[7] = 0x42280000; // 42.0		2/3 of width&height
	}
	
	// Small 
	small_new_start = big_new_start + total_big_icons*sizeof(uint32_t);
	small_new_start += total_big_icons*0x20;
	id_table = (uint32_t *)(buf + small_new_start);
	mapping = ((uint8_t *)id_table) + total_big_icons*sizeof(uint32_t);	
	
	*(uint32_t *)(buf+CS_ICON_SMALL_NUM_OFFSET) = total_big_icons;
	*(uint16_t *)(buf+CS_ICON_SMALL_NUM_OFFSET2) = total_big_icons;
	*(uint16_t *)(buf+CS_ICON_SMALL_NUM_OFFSET3) = total_big_icons;
	
	memcpy(id_table, buf+CS_ICON_SMALL_ID_TABLE, CS_NUM_ORIGINAL_ICONS_SMALL*sizeof(uint32_t));
	memcpy(mapping, buf+CS_ICON_SMALL_MAPPING, CS_NUM_ORIGINAL_ICONS_SMALL*0x20);
	
	*(uint32_t *)(buf+CS_ICON_SMALL_ID_TABLE_FPTR) = small_new_start;
	*(uint32_t *)(buf+CS_ICON_SMALL_MAPPING_FPTR) = small_new_start + total_big_icons*sizeof(uint32_t);
	
	for (unsigned int i = CS_NUM_ORIGINAL_ICONS_SMALL; i < total_big_icons; i++)
	{
		uint32_t *map = (uint32_t *)(mapping + i*0x20);
		
		id_table[i] = i;
		
		if (i == CS_NUM_ORIGINAL_ICONS_SMALL)
		{
			map[0] = 3;
		}
		else
		{
			map[0] = icon_small_first_new_entry + (i-1-CS_NUM_ORIGINAL_ICONS_SMALL);			
		}
		map[1] = 3;
		map[2] = 0x3F800000; // 1.0
		map[3] = 8;
		map[4] = map[5] = 0;
		map[6] = map[7] = 0x41E00000; // 28.0    2/3 of width&height
	}
}

void build_cs_ems_sn(uint8_t *buf, size_t new_section_start, size_t num_new_select_names, uint32_t sn_first_new_entry)
{
	uint32_t total_select_names = NUM_ORIGINAL_SELECT_NAMES+num_new_select_names;
		
	uint32_t *id_table = (uint32_t *)(buf + new_section_start);
	uint8_t *mapping = ((uint8_t *)id_table) + total_select_names*sizeof(uint32_t);	
	
	*(uint32_t *)(buf+CS_SN_NUM_OFFSET) = total_select_names;
	*(uint16_t *)(buf+CS_SN_NUM_OFFSET2) = total_select_names;
	*(uint16_t *)(buf+CS_SN_NUM_OFFSET3) = total_select_names;
	
	memcpy(id_table, buf+CS_SN_ID_TABLE, NUM_ORIGINAL_SELECT_NAMES*sizeof(uint32_t));
	memcpy(mapping, buf+CS_SN_MAPPING, NUM_ORIGINAL_SELECT_NAMES*0x20);
	
	*(uint32_t *)(buf+CS_SN_ID_TABLE_FPTR) = new_section_start;
	*(uint32_t *)(buf+CS_SN_MAPPING_FPTR) = new_section_start + total_select_names*sizeof(uint32_t);
	
	for (unsigned int i = NUM_ORIGINAL_SELECT_NAMES; i < total_select_names; i++)
	{
		uint32_t *map = (uint32_t *)(mapping + i*0x20);
		
		id_table[i] = i;
		map[0] = sn_first_new_entry + (i-NUM_ORIGINAL_SELECT_NAMES);
		map[1] = 3;
		map[2] = 0x3F800000; // 1.0
		map[3] = 8;
		map[4] = map[5] = 0;
		map[6] = 0x43AA0000; // 340.0        2/3 of width
		map[7] = 0x42600000; // 56.0		 2/3 of height
	}	
}

void build_cs_ems_sn2(uint8_t *buf, size_t new_section_start, size_t num_new_select2_names, uint32_t sn2_first_new_entry)
{
	uint32_t total_select2_names = NUM_ORIGINAL_SELECT2_NAMES+num_new_select2_names;
		
	uint32_t *id_table = (uint32_t *)(buf + new_section_start);
	uint8_t *mapping = ((uint8_t *)id_table) + total_select2_names*sizeof(uint32_t);	
	
	*(uint32_t *)(buf+CS_SN2_NUM_OFFSET) = total_select2_names;
	*(uint16_t *)(buf+CS_SN2_NUM_OFFSET2) = total_select2_names;
	*(uint16_t *)(buf+CS_SN2_NUM_OFFSET3) = total_select2_names;
	
	memcpy(id_table, buf+CS_SN2_ID_TABLE, NUM_ORIGINAL_SELECT2_NAMES*sizeof(uint32_t));
	memcpy(mapping, buf+CS_SN2_MAPPING, NUM_ORIGINAL_SELECT2_NAMES*0x20);
	
	*(uint32_t *)(buf+CS_SN2_ID_TABLE_FPTR) = new_section_start;
	*(uint32_t *)(buf+CS_SN2_MAPPING_FPTR) = new_section_start + total_select2_names*sizeof(uint32_t);
	
	for (unsigned int i = NUM_ORIGINAL_SELECT2_NAMES; i < total_select2_names; i++)
	{
		uint32_t *map = (uint32_t *)(mapping + i*0x20);
		
		id_table[i] = i;
		map[0] = sn2_first_new_entry + (i-NUM_ORIGINAL_SELECT2_NAMES);
		map[1] = 3;
		map[2] = 0x3F800000; // 1.0
		map[3] = 8;
		map[4] = map[5] = 0;
		map[6] = 0x43240000; // 164.0	2/3 of width
		map[7] = 0x41D00000; // 26.0	2/3 of height
	}	
}

bool build_cs_ems(ChcFile *chc, uint32_t icon_big_first_new_entry, uint32_t icon_small_first_new_entry, uint32_t sn_first_new_entry, uint32_t sn2_first_new_entry)
{
	uint8_t *buf, *out;
	size_t size;
	unsigned int extended_size = 0;
	unsigned int sn_start = 0, sn2_start = 0;
	
	buf = Utils::ReadFile(CHA_SEL_EMS_BASE, &size);
	if (!buf)
		return false;
	
	if (size != CHA_SEL_EMS_SIZE)
	{
		DPRINTF("Don't modify cha_sel_JP.ems, retard!\n");
		delete[] buf;
		return false;
	}
	
	size_t num_new_icons = chc->GetNumIcons();
	
	if (num_new_icons > 0)
	{
		unsigned int extended_size_icons;
		
		extended_size_icons = (CS_NUM_ORIGINAL_ICONS_BIG+num_new_icons)*sizeof(uint32_t);
		extended_size_icons += (CS_NUM_ORIGINAL_ICONS_BIG+num_new_icons)*0x20;
		extended_size_icons *= 2; // Big and small
		
		extended_size += extended_size_icons;
	}

	size_t num_new_select_names = chc->GetNumSelectNames();
	
	if (num_new_select_names > 0)
	{
		unsigned int extended_size_sn;
		
		extended_size_sn = (NUM_ORIGINAL_SELECT_NAMES+num_new_select_names)*sizeof(uint32_t);
		extended_size_sn += (NUM_ORIGINAL_SELECT_NAMES+num_new_select_names)*0x20;
			
		sn_start = size + extended_size;
		extended_size += extended_size_sn;
	}

	size_t num_new_select2_names = chc->GetNumSelect2Names();
	
	if (num_new_select2_names > 0)
	{
		unsigned int extended_size_sn2;
		
		extended_size_sn2 = (NUM_ORIGINAL_SELECT2_NAMES+num_new_select2_names)*sizeof(uint32_t);
		extended_size_sn2 += (NUM_ORIGINAL_SELECT2_NAMES+num_new_select2_names)*0x20;
		
		sn2_start = size + extended_size;
		extended_size += extended_size_sn2;
	}
	
	out = new uint8_t[size+extended_size];
	if (!out)
	{
		DPRINTF("%s: Memory allocation error.\n", __PRETTY_FUNCTION__);
		return false;
	}
	
	memcpy(out, buf, size);
	delete[] buf;
	buf = nullptr;	
	
	if (num_new_icons > 0)
		build_cs_ems_icons(out, size, num_new_icons, icon_big_first_new_entry, icon_small_first_new_entry);
	
	if (num_new_select_names > 0)
		build_cs_ems_sn(out, sn_start, num_new_select_names, sn_first_new_entry);
	
	if (num_new_select2_names > 0)
		build_cs_ems_sn2(out, sn2_start, num_new_select2_names, sn2_first_new_entry);
	
	bool ret = Utils::WriteFileBool(CHA_SEL_EMS_OUT, out, size+extended_size, true, true);
	delete[] out;
	
	return ret;	
}

bool build_cs_files(ChcFile *chc)
{
	uint32_t icon_big_first_new_entry, icon_small_first_new_entry, sn_first_new_entry, sn2_first_new_entry;
	
	if (!build_cs_emb(chc, &icon_big_first_new_entry, &icon_small_first_new_entry, &sn_first_new_entry, &sn2_first_new_entry))
		return false;
	
	if (!build_cs_ems(chc, icon_big_first_new_entry, icon_small_first_new_entry, sn_first_new_entry, sn2_first_new_entry))
		return false;	
	
	return true;
}

void process_cha_sel(SlotsFile *slf)
{
	ChcFile chc;
	
	if (!chc.LoadFromFile(CHA_SEL_CACHE, false))
	{
		build_cs_cache(slf, &chc);
		
		if (chc.IsEmpty())
			return;
		
		if (!build_cs_files(&chc))
			return;
		
		chc.SaveToFile(CHA_SEL_CACHE, true, true);
	}
	
	for (size_t i = 0; i < chc.GetNumIcons(); i++)
	{
		uint32_t cms_entry, cms_model_spec_idx;
		
		if (!chc.GetIconEntry(i, &cms_entry, &cms_model_spec_idx))
		{
			DPRINTF("%s: buuuuuuuuuuuuuuu\n", __PRETTY_FUNCTION__);
			return;
		}
		
		slf->SetIconId(cms_entry, cms_model_spec_idx, (float)(CS_NUM_ORIGINAL_ICONS_BIG+i));
	}

	for (size_t i = 0; i < chc.GetNumSelectNames(); i++)
	{
		uint32_t cms_entry, cms_model_spec_idx;
		
		if (!chc.GetSelectNameEntry(i, &cms_entry, &cms_model_spec_idx))
		{
			DPRINTF("%s: buuuuuuuuuuuuuuu\n", __PRETTY_FUNCTION__);
			return;
		}
		
		slf->SetSelectNameId(cms_entry, cms_model_spec_idx, NUM_ORIGINAL_SELECT_NAMES+i);
	}

	for (size_t i = 0; i < chc.GetNumSelect2Names(); i++)
	{
		uint32_t cms_entry, cms_model_spec_idx;
		
		if (!chc.GetSelect2NameEntry(i, &cms_entry, &cms_model_spec_idx))
		{
			DPRINTF("%s: buuuuuuuuuuuuuuu\n", __PRETTY_FUNCTION__);
			return;
		}
		
		slf->SetSelect2NameId(cms_entry, cms_model_spec_idx, NUM_ORIGINAL_SELECT2_NAMES+i);
	}			
}

void build_gwr_cache(SlotsFile *slf, GrcFile *grc)
{
	for (uint32_t i = 0; i < slf->GetNumSlots(); i++)
	{
		for (uint32_t j = 0; j < MAX_SLOTS; j++)
		{
			if (slf->SlotHasCI3(i, j))
			{
				uint32_t cms_entry;
				uint32_t cms_model_spec_idx;
				
				if (slf->GetCmsEntry(i, j, &cms_entry, &cms_model_spec_idx))
				{
					std::string name = Utils::SSSSModelFileName(cms_entry, cms_model_spec_idx, ".dds");
					
					if (Utils::FileExists(std::string(ICONS_DIRECTORY_BIG) + name))
					{
						if (Utils::FileExists(std::string(ICONS_DIRECTORY_SMALL) + name))
							grc->AppendIconEntry(cms_entry, cms_model_spec_idx);
					}

					if (Utils::FileExists(std::string(GWR_NAMES_DIRECTORY) + languages[1] + "/" + name))
					{
						grc->AppendNameEntry(cms_entry, cms_model_spec_idx);
					}
				}
			}
		}
	}	
}

bool build_gwr_emb(GrcFile *grc, uint32_t *icon_big_first_new_entry, uint32_t *name_first_new_entry)
{
	*icon_big_first_new_entry = 0;
	*name_first_new_entry = 0;
		
	for (size_t i = 0; i < gwr_base.size(); i++)
	{
		EmbFile emb;
		
		if (!emb.LoadFromFile(gwr_base[i]))
			return false;
		
		// Icons big
		for (size_t j = 0; j < grc->GetNumIcons(); j++)
		{
			uint32_t cms_entry, cms_model_spec_idx;
			
			if (!grc->GetIconEntry(j, &cms_entry, &cms_model_spec_idx))
				return false;
		
			uint32_t entry;
			uint8_t *buf;
			size_t size;	
			std::string name;
			
			name = Utils::SSSSModelFileName(cms_entry, cms_model_spec_idx, ".dds");
			
			buf = Utils::ReadFile(std::string(ICONS_DIRECTORY_BIG) + name, &size, false);
			if (!buf)
				return false;
			
			entry = emb.AppendFile(buf, size, name);
			delete[] buf;
			
			if (j == 0)
			{
				if (i == 0)
				{
					*icon_big_first_new_entry = entry;
				}
				else if (entry != *icon_big_first_new_entry)
				{
					DPRINTF("There is a mismatch between the gw_result bases. You shouldn't have added entries to these ones!!!!\n");
					return false;
				}
			}			
		}

		// Names
		for (size_t j = 0; j < grc->GetNumNames(); j++)
		{
			uint32_t cms_entry, cms_model_spec_idx;
			
			if (!grc->GetNameEntry(j, &cms_entry, &cms_model_spec_idx))
				return false;
			
			uint32_t entry;
			uint8_t *buf;
			size_t size;	
			std::string name;
			
			name = Utils::SSSSModelFileName(cms_entry, cms_model_spec_idx, ".dds");
			
			buf = Utils::ReadFile(std::string(GWR_NAMES_DIRECTORY) + languages[i] + "/" + name, &size, false);
			if (!buf)
			{
				// If this is english version, return false, this one must exist.
				if (i == 1)
					return false;
				
				// Try to fallback to english version
				buf = Utils::ReadFile(std::string(GWR_NAMES_DIRECTORY) + languages[1] + "/" + name, &size, false);
				if (!buf)
					return false;
			}
			
			entry = emb.AppendFile(buf, size, name);
			
			if (j == 0)
			{
				if (i == 0)
				{
					*name_first_new_entry = entry;
				}
				else if (entry != *name_first_new_entry)
				{
					DPRINTF("There is a mismatch between the gw_result bases. You shouldn't have added entries to these ones!!!!\n");
					return false;
				}
			}
		}
		
		if (!emb.SaveToFile(gwr_out[i], true, true))
			return false;
	}	
	
	return true;
}

void build_gwr_ems_icons(uint8_t *buf, size_t new_section_start, size_t num_new_icons, uint32_t icon_big_first_new_entry)
{
	uint32_t big_new_start;
	uint32_t total_big_icons = GWR_NUM_ORIGINAL_ICONS_BIG+num_new_icons;
		
	big_new_start = new_section_start;
	uint32_t *id_table = (uint32_t *)(buf + big_new_start);
	uint8_t *mapping = ((uint8_t *)id_table) + total_big_icons*sizeof(uint32_t);	
	
	*(uint32_t *)(buf+GWR_ICON_BIG_NUM_OFFSET) = total_big_icons;
	*(uint16_t *)(buf+GWR_ICON_BIG_NUM_OFFSET2) = total_big_icons;
	*(uint16_t *)(buf+GWR_ICON_BIG_NUM_OFFSET3) = total_big_icons;
	
	memcpy(id_table, buf+GWR_ICON_BIG_ID_TABLE, GWR_NUM_ORIGINAL_ICONS_BIG*sizeof(uint32_t));
	memcpy(mapping, buf+GWR_ICON_BIG_MAPPING, GWR_NUM_ORIGINAL_ICONS_BIG*0x20);
	
	*(uint32_t *)(buf+GWR_ICON_BIG_ID_TABLE_FPTR) = big_new_start;
	*(uint32_t *)(buf+GWR_ICON_BIG_MAPPING_FPTR) = big_new_start + total_big_icons*sizeof(uint32_t);
	
	for (unsigned int i = GWR_NUM_ORIGINAL_ICONS_BIG; i < total_big_icons; i++)
	{
		uint32_t *map = (uint32_t *)(mapping + i*0x20);
		
		id_table[i] = i;
		map[0] = icon_big_first_new_entry + (i-GWR_NUM_ORIGINAL_ICONS_BIG);
		map[1] = 3;
		map[2] = 0x3F800000; // 1.0
		map[3] = 8;
		map[4] = map[5] = 0;
		map[6] = map[7] = 0x42280000; // 42.0		2/3 of width&height
	}
}

void build_gwr_ems_names(uint8_t *buf, size_t new_section_start, size_t num_new_names, uint32_t name_first_new_entry)
{
	uint32_t total_names = NUM_ORIGINAL_GWR_NAMES+num_new_names;
		
	uint32_t *id_table = (uint32_t *)(buf + new_section_start);
	uint8_t *mapping = ((uint8_t *)id_table) + total_names*sizeof(uint32_t);	
	
	*(uint32_t *)(buf+GWR_NAME_NUM_OFFSET) = total_names;
	*(uint16_t *)(buf+GWR_NAME_NUM_OFFSET2) = total_names;
	*(uint16_t *)(buf+GWR_NAME_NUM_OFFSET3) = total_names;
	
	memcpy(id_table, buf+GWR_NAME_ID_TABLE, NUM_ORIGINAL_GWR_NAMES*sizeof(uint32_t));
	memcpy(mapping, buf+GWR_NAME_MAPPING, NUM_ORIGINAL_GWR_NAMES*0x20);
	
	*(uint32_t *)(buf+GWR_NAME_ID_TABLE_FPTR) = new_section_start;
	*(uint32_t *)(buf+GWR_NAME_MAPPING_FPTR) = new_section_start + total_names*sizeof(uint32_t);
	
	for (unsigned int i = NUM_ORIGINAL_GWR_NAMES; i < total_names; i++)
	{
		uint32_t *map = (uint32_t *)(mapping + i*0x20);
		
		id_table[i] = i;
		map[0] = name_first_new_entry + (i-NUM_ORIGINAL_GWR_NAMES);
		map[1] = 3;
		map[2] = 0x3F800000; // 1.0
		map[3] = 0xB;
		map[4] = map[5] = 0;
		map[6] = 0x43800000; // 256.0	2/3 of width
		map[7] = 0x42200000; // 40.0	2/3 of height
	}	
}

bool build_gwr_ems(GrcFile *grc, uint32_t icon_big_first_new_entry, uint32_t name_first_new_entry)
{
	uint8_t *buf, *out;
	size_t size;
	unsigned int extended_size = 0;
	unsigned int n_start = 0;
	
	buf = Utils::ReadFile(GWR_EMS_BASE, &size);
	if (!buf)
		return false;
	
	if (size != GWR_EMS_SIZE)
	{
		DPRINTF("Don't modify gw_result_JP.ems, retard!\n");
		delete[] buf;
		return false;
	}
	
	size_t num_new_icons = grc->GetNumIcons();
	
	if (num_new_icons > 0)
	{
		unsigned int extended_size_icons;
		
		extended_size_icons = (GWR_NUM_ORIGINAL_ICONS_BIG+num_new_icons)*sizeof(uint32_t);
		extended_size_icons += (GWR_NUM_ORIGINAL_ICONS_BIG+num_new_icons)*0x20;		
		
		extended_size += extended_size_icons;
	}

	size_t num_new_names = grc->GetNumNames();
	
	if (num_new_names > 0)
	{
		unsigned int extended_size_names;
		
		extended_size_names = (NUM_ORIGINAL_GWR_NAMES+num_new_names)*sizeof(uint32_t);
		extended_size_names += (NUM_ORIGINAL_GWR_NAMES+num_new_names)*0x20;
			
		n_start = size + extended_size;
		extended_size += extended_size_names;
	}	
	
	out = new uint8_t[size+extended_size];
	if (!out)
	{
		DPRINTF("%s: Memory allocation error.\n", __PRETTY_FUNCTION__);
		return false;
	}
	
	memcpy(out, buf, size);
	delete[] buf;
	buf = nullptr;	
	
	if (num_new_icons > 0)
		build_gwr_ems_icons(out, size, num_new_icons, icon_big_first_new_entry);
	
	if (num_new_names > 0)
		build_gwr_ems_names(out, n_start, num_new_names, name_first_new_entry);	
	
	bool ret = Utils::WriteFileBool(GWR_EMS_OUT, out, size+extended_size, true, true);
	delete[] out;
	
	return ret;	
}

bool build_gwr_files(GrcFile *grc)
{
	uint32_t icon_big_first_new_entry, name_first_new_entry;
	
	if (!build_gwr_emb(grc, &icon_big_first_new_entry, &name_first_new_entry))
		return false;
	
	if (!build_gwr_ems(grc, icon_big_first_new_entry, name_first_new_entry))
		return false;	
	
	return true;
}

void process_gwr(SlotsFile *slf)
{
	GrcFile grc;
	
	if (!grc.LoadFromFile(GWR_CACHE, false))
	{
		build_gwr_cache(slf, &grc);
		
		if (grc.IsEmpty())
			return;
		
		if (!build_gwr_files(&grc))
			return;
		
		grc.SaveToFile(GWR_CACHE, true, true);
	}
	
	for (size_t i = 0; i < grc.GetNumIcons(); i++)
	{
		uint32_t cms_entry, cms_model_spec_idx;
		
		if (!grc.GetIconEntry(i, &cms_entry, &cms_model_spec_idx))
		{
			DPRINTF("%s: buuuuuuuuuuuuuuu\n", __PRETTY_FUNCTION__);
			return;
		}
		
		slf->SetGwrIconId(cms_entry, cms_model_spec_idx, (float)(GWR_NUM_ORIGINAL_ICONS_BIG+i));
	}
	
	for (size_t i = 0; i < grc.GetNumNames(); i++)
	{
		uint32_t cms_entry, cms_model_spec_idx;
		
		if (!grc.GetNameEntry(i, &cms_entry, &cms_model_spec_idx))
		{
			DPRINTF("%s: buuuuuuuuuuuuuuu\n", __PRETTY_FUNCTION__);
			return;
		}
		
		slf->SetGwrNameId(cms_entry, cms_model_spec_idx, (float)(NUM_ORIGINAL_GWR_NAMES+i));
	}
}

void build_gwt_cache(SlotsFile *slf, GtcFile *gtc)
{
	for (uint32_t i = 0; i < slf->GetNumSlots(); i++)
	{
		for (uint32_t j = 0; j < MAX_SLOTS; j++)
		{
			if (slf->SlotHasCI4(i, j))
			{
				uint32_t cms_entry;
				uint32_t cms_model_spec_idx;
				
				if (slf->GetCmsEntry(i, j, &cms_entry, &cms_model_spec_idx))
				{
					std::string name = Utils::SSSSModelFileName(cms_entry, cms_model_spec_idx, ".dds");
					
					if (Utils::FileExists(std::string(GWT_BANNERS_DIRECTORY) + name))
					{
						gtc->AppendBannerEntry(cms_entry, cms_model_spec_idx);
					}

					if (Utils::FileExists(std::string(GWT_BANNER_NAMES_DIRECTORY) + languages[1] + "/" + name))
					{
						gtc->AppendBannerNameEntry(cms_entry, cms_model_spec_idx);
					}
				}
			}
		}
	}	
}

bool build_gwt_emb(GtcFile *gtc, uint32_t *banner_first_new_entry, uint32_t *bn_first_new_entry)
{
	*banner_first_new_entry = 0;
	*bn_first_new_entry = 0;
		
	for (size_t i = 0; i < gwt_base.size(); i++)
	{
		EmbFile emb;
		
		if (!emb.LoadFromFile(gwt_base[i]))
			return false;
		
		// Banners
		for (size_t j = 0; j < gtc->GetNumBanners(); j++)
		{
			uint32_t cms_entry, cms_model_spec_idx;
			
			if (!gtc->GetBannerEntry(j, &cms_entry, &cms_model_spec_idx))
				return false;
		
			uint32_t entry;
			uint8_t *buf;
			size_t size;	
			std::string name;
			
			name = Utils::SSSSModelFileName(cms_entry, cms_model_spec_idx, ".dds");
			
			buf = Utils::ReadFile(std::string(GWT_BANNERS_DIRECTORY) + name, &size, false);
			if (!buf)
				return false;
			
			entry = emb.AppendFile(buf, size, name);
			delete[] buf;
			
			if (j == 0)
			{
				if (i == 0)
				{
					*banner_first_new_entry = entry;
				}
				else if (entry != *banner_first_new_entry)
				{
					DPRINTF("There is a mismatch between the gw_tour bases. You shouldn't have added entries to these ones!!!!\n");
					return false;
				}
			}			
		}

		// Banner Names
		for (size_t j = 0; j < gtc->GetNumBannerNames(); j++)
		{
			uint32_t cms_entry, cms_model_spec_idx;
			
			if (!gtc->GetBannerNameEntry(j, &cms_entry, &cms_model_spec_idx))
				return false;
			
			uint32_t entry;
			uint8_t *buf;
			size_t size;	
			std::string name;
			
			name = Utils::SSSSModelFileName(cms_entry, cms_model_spec_idx, ".dds");
			
			buf = Utils::ReadFile(std::string(GWT_BANNER_NAMES_DIRECTORY) + languages[i] + "/" + name, &size, false);
			if (!buf)
			{
				// If this is english version, return false, this one must exist.
				if (i == 1)
					return false;
				
				// Try to fallback to english version
				buf = Utils::ReadFile(std::string(GWT_BANNER_NAMES_DIRECTORY) + languages[1] + "/" + name, &size, false);
				if (!buf)
					return false;
			}
			
			entry = emb.AppendFile(buf, size, name);
			
			if (j == 0)
			{
				if (i == 0)
				{
					*bn_first_new_entry = entry;
				}
				else if (entry != *bn_first_new_entry)
				{
					DPRINTF("There is a mismatch between the gw_tour bases. You shouldn't have added entries to these ones!!!!\n");
					return false;
				}
			}
		}
		
		if (!emb.SaveToFile(gwt_out[i], true, true))
			return false;
	}	
	
	return true;
}

void build_gwt_ems_banners(uint8_t *buf, size_t new_section_start, size_t num_new_banners, uint32_t banner_first_new_entry)
{
	uint32_t banners_new_start;
	uint32_t total_banners = NUM_ORIGINAL_BANNERS+num_new_banners;
		
	banners_new_start = new_section_start;
	uint32_t *id_table = (uint32_t *)(buf + banners_new_start);
	uint8_t *mapping = ((uint8_t *)id_table) + total_banners*sizeof(uint32_t);	
	
	*(uint32_t *)(buf+GWT_BANNER_NUM_OFFSET) = total_banners;
	*(uint16_t *)(buf+GWT_BANNER_NUM_OFFSET2) = total_banners;
	*(uint16_t *)(buf+GWT_BANNER_NUM_OFFSET3) = total_banners;
	
	memcpy(id_table, buf+GWT_BANNER_ID_TABLE, NUM_ORIGINAL_BANNERS*sizeof(uint32_t));
	memcpy(mapping, buf+GWT_BANNER_MAPPING, NUM_ORIGINAL_BANNERS*0x20);
	
	*(uint32_t *)(buf+GWT_BANNER_ID_TABLE_FPTR) = banners_new_start;
	*(uint32_t *)(buf+GWT_BANNER_MAPPING_FPTR) = banners_new_start + total_banners*sizeof(uint32_t);
	
	for (unsigned int i = NUM_ORIGINAL_BANNERS; i < total_banners; i++)
	{
		uint32_t *map = (uint32_t *)(mapping + i*0x20);
		
		id_table[i] = i;
		map[0] = banner_first_new_entry + (i-NUM_ORIGINAL_BANNERS);
		map[1] = 3;
		map[2] = 0x3F800000; // 1.0
		map[3] = 8;
		map[4] = map[5] = 0x3F800000; // 1.0
		map[6] = 0x42800000; // 64.0  2/3 of width
		map[7] = 0x438D0000; // 282.0 2/3 of height
	}
}

void build_gwt_ems_banner_names(uint8_t *buf, size_t new_section_start, size_t num_new_banner_names, uint32_t bn_first_new_entry)
{
	uint32_t total_banner_names = NUM_ORIGINAL_BANNER_NAMES+num_new_banner_names;
		
	uint32_t *id_table = (uint32_t *)(buf + new_section_start);
	uint8_t *mapping = ((uint8_t *)id_table) + total_banner_names*sizeof(uint32_t);	
	
	*(uint32_t *)(buf+GWT_BANNER_NAME_NUM_OFFSET) = total_banner_names;
	*(uint16_t *)(buf+GWT_BANNER_NAME_NUM_OFFSET2) = total_banner_names;
	*(uint16_t *)(buf+GWT_BANNER_NAME_NUM_OFFSET3) = total_banner_names;
	
	memcpy(id_table, buf+GWT_BANNER_NAME_ID_TABLE, NUM_ORIGINAL_BANNER_NAMES*sizeof(uint32_t));
	memcpy(mapping, buf+GWT_BANNER_NAME_MAPPING, NUM_ORIGINAL_BANNER_NAMES*0x20);
	
	*(uint32_t *)(buf+GWT_BANNER_NAME_ID_TABLE_FPTR) = new_section_start;
	*(uint32_t *)(buf+GWT_BANNER_NAME_MAPPING_FPTR) = new_section_start + total_banner_names*sizeof(uint32_t);
	
	for (unsigned int i = NUM_ORIGINAL_BANNER_NAMES; i < total_banner_names; i++)
	{
		uint32_t *map = (uint32_t *)(mapping + i*0x20);
		
		id_table[i] = i;
		map[0] = bn_first_new_entry + (i-NUM_ORIGINAL_BANNER_NAMES);
		map[1] = 3;
		map[2] = 0x3F800000; // 1.0
		map[3] = 0x8;
		map[4] = map[5] = 0;
		map[6] = 0x42000000; // 32.0	2/3 of width
		map[7] = 0x43800000; // 256.0	2/3 of height
	}		
}

bool build_gwt_ems(GtcFile *gtc, uint32_t banner_first_new_entry, uint32_t bn_first_new_entry)
{
	uint8_t *buf, *out;
	size_t size;
	unsigned int extended_size = 0;
	unsigned int bn_start = 0;
	
	buf = Utils::ReadFile(GWT_EMS_BASE, &size);
	if (!buf)
		return false;
	
	if (size != GWT_EMS_SIZE)
	{
		DPRINTF("Don't modify gw_tour_JP.ems, retard!\n");
		delete[] buf;
		return false;
	}
	
	size_t num_new_banners = gtc->GetNumBanners();
	
	if (num_new_banners > 0)
	{
		unsigned int extended_size_banners;
		
		extended_size_banners = (NUM_ORIGINAL_BANNERS+num_new_banners)*sizeof(uint32_t);
		extended_size_banners += (NUM_ORIGINAL_BANNERS+num_new_banners)*0x20;		
		
		extended_size += extended_size_banners;
	}

	size_t num_new_banner_names = gtc->GetNumBannerNames();
	
	if (num_new_banner_names > 0)
	{
		unsigned int extended_size_bn;
		
		extended_size_bn = (NUM_ORIGINAL_BANNER_NAMES+num_new_banner_names)*sizeof(uint32_t);
		extended_size_bn += (NUM_ORIGINAL_BANNER_NAMES+num_new_banner_names)*0x20;
			
		bn_start = size + extended_size;
		extended_size += extended_size_bn;
	}	
	
	out = new uint8_t[size+extended_size];
	if (!out)
	{
		DPRINTF("%s: Memory allocation error.\n", __PRETTY_FUNCTION__);
		return false;
	}
	
	memcpy(out, buf, size);
	delete[] buf;
	buf = nullptr;	
	
	if (num_new_banners > 0)
		build_gwt_ems_banners(out, size, num_new_banners, banner_first_new_entry);
	
	if (num_new_banner_names > 0)
		build_gwt_ems_banner_names(out, bn_start, num_new_banner_names, bn_first_new_entry);	
	
	bool ret = Utils::WriteFileBool(GWT_EMS_OUT, out, size+extended_size, true, true);
	delete[] out;
	
	return ret;	
}

bool build_gwt_files(GtcFile *gtc)
{
	uint32_t banner_first_new_entry, bn_first_new_entry;
	
	if (!build_gwt_emb(gtc, &banner_first_new_entry, &bn_first_new_entry))
		return false;
	
	if (!build_gwt_ems(gtc, banner_first_new_entry, bn_first_new_entry))
		return false;	
	
	return true;
}

void process_gwt(SlotsFile *slf)
{
	GtcFile gtc;
	
	if (!gtc.LoadFromFile(GWT_CACHE, false))
	{
		build_gwt_cache(slf, &gtc);
		
		if (gtc.IsEmpty())
			return;
		
		if (!build_gwt_files(&gtc))
			return;
		
		gtc.SaveToFile(GWT_CACHE, true, true);
	}
	
	for (size_t i = 0; i < gtc.GetNumBanners(); i++)
	{
		uint32_t cms_entry, cms_model_spec_idx;
		
		if (!gtc.GetBannerEntry(i, &cms_entry, &cms_model_spec_idx))
		{
			DPRINTF("%s: buuuuuuuuuuuuuuu\n", __PRETTY_FUNCTION__);
			return;
		}
		
		slf->SetGwtBannerId(cms_entry, cms_model_spec_idx, (float)(NUM_ORIGINAL_BANNERS+i));
	}
	
	for (size_t i = 0; i < gtc.GetNumBannerNames(); i++)
	{
		uint32_t cms_entry, cms_model_spec_idx;
		
		if (!gtc.GetBannerNameEntry(i, &cms_entry, &cms_model_spec_idx))
		{
			DPRINTF("%s: buuuuuuuuuuuuuuu\n", __PRETTY_FUNCTION__);
			return;
		}
		
		slf->SetGwtBannerNameId(cms_entry, cms_model_spec_idx, (float)(NUM_ORIGINAL_BANNER_NAMES+i));
	}
}

void build_vs_cache(SlotsFile *slf, VscFile *vsc)
{
	for (uint32_t i = 0; i < slf->GetNumSlots(); i++)
	{
		for (uint32_t j = 0; j < MAX_SLOTS; j++)
		{
			if (!slf->IsCurrentRandomSlot(i) && !slf->IsFreeSlot(i, j))
			{
				uint32_t cms_entry;
				uint32_t cms_model_spec_idx;
				
				if (slf->GetCmsEntry(i, j, &cms_entry, &cms_model_spec_idx))
				{
					std::string name = Utils::SSSSModelFileName(cms_entry, cms_model_spec_idx, ".dds");
					
					if (Utils::FileExists(std::string(SN_DIRECTORY) + languages[1] + "/" + name))
					{
						vsc->AppendSelectNameEntry(cms_entry, cms_model_spec_idx);
					}					
				}
			}
		}
	}	
}

bool build_vs_emb(VscFile *vsc, uint32_t *vsn_first_new_entry)
{
	*vsn_first_new_entry = 0;
		
	for (size_t i = 0; i < vs_base.size(); i++)
	{
		EmbFile emb;
		
		if (!emb.LoadFromFile(vs_base[i]))
			return false;
		
		// Select names
		for (size_t j = 0; j < vsc->GetNumSelectNames(); j++)
		{
			uint32_t cms_entry, cms_model_spec_idx;
			
			if (!vsc->GetSelectNameEntry(j, &cms_entry, &cms_model_spec_idx))
				return false;
			
			uint32_t entry;
			uint8_t *buf;
			size_t size;	
			std::string name;
			
			name = Utils::SSSSModelFileName(cms_entry, cms_model_spec_idx, ".dds");
			
			buf = Utils::ReadFile(std::string(SN_DIRECTORY) + languages[i] + "/" + name, &size, false);
			if (!buf)
			{
				// If this is english version, return false, this one must exist.
				if (i == 1)
					return false;
				
				// Try to fallback to english version
				buf = Utils::ReadFile(std::string(SN_DIRECTORY) + languages[1] + "/" + name, &size, false);
				if (!buf)
					return false;
			}
			
			entry = emb.AppendFile(buf, size, name);
			
			if (j == 0)
			{
				if (i == 0)
				{
					*vsn_first_new_entry = entry;
				}
				else if (entry != *vsn_first_new_entry)
				{
					DPRINTF("There is a mismatch between the vs bases. You shouldn't have added entries to these ones!!!!\n");
					return false;
				}
			}
		}		
		
		if (!emb.SaveToFile(vs_out[i], true, true))
			return false;
	}
	
	return true;	
}

void build_vs_ems_vsn(uint8_t *buf, size_t new_section_start, size_t num_new_select_names, uint32_t vsn_first_new_entry)
{
	uint32_t total_select_names = NUM_ORIGINAL_SELECT_NAMES+num_new_select_names;
		
	uint32_t *id_table = (uint32_t *)(buf + new_section_start);
	uint8_t *mapping = ((uint8_t *)id_table) + total_select_names*sizeof(uint32_t);	
	
	*(uint32_t *)(buf+VS_NAME_NUM_OFFSET) = total_select_names;
	*(uint16_t *)(buf+VS_NAME_NUM_OFFSET2) = total_select_names;
	*(uint16_t *)(buf+VS_NAME_NUM_OFFSET3) = total_select_names;
	
	memcpy(id_table, buf+VS_NAME_ID_TABLE, NUM_ORIGINAL_VS_NAMES*sizeof(uint32_t));
	memcpy(mapping, buf+VS_NAME_MAPPING, NUM_ORIGINAL_VS_NAMES*0x20);
	
	*(uint32_t *)(buf+VS_NAME_ID_TABLE_FPTR) = new_section_start;
	*(uint32_t *)(buf+VS_NAME_MAPPING_FPTR) = new_section_start + total_select_names*sizeof(uint32_t);
	
	for (unsigned int i = NUM_ORIGINAL_VS_NAMES; i < total_select_names; i++)
	{
		uint32_t *map = (uint32_t *)(mapping + i*0x20);
		
		id_table[i] = i;
		
		if (i == NUM_ORIGINAL_VS_NAMES)
		{
			map[0] = 2;
		}
		else
		{
			map[0] = vsn_first_new_entry + (i-1-NUM_ORIGINAL_VS_NAMES);			
		}
		
		map[1] = 3;
		map[2] = 0x3F800000; // 1.0
		map[3] = 8;
		map[4] = map[5] = 0;
		map[6] = 0x43AA0000; // 340.0        2/3 of width
		map[7] = 0x42600000; // 56.0		 2/3 of height
	}	
}

bool build_vs_ems(VscFile *vsc, uint32_t vsn_first_new_entry)
{
	uint8_t *buf, *out;
	size_t size;
	unsigned int extended_size = 0;
		
	buf = Utils::ReadFile(VS_EMS_BASE, &size);
	if (!buf)
		return false;
	
	if (size != VS_EMS_SIZE)
	{
		DPRINTF("Don't modify vs_JP.ems, retard!\n");
		delete[] buf;
		return false;
	}	

	size_t num_new_select_names = vsc->GetNumSelectNames();
	
	if (num_new_select_names > 0)
	{
		unsigned int extended_size_sn;
		
		extended_size_sn = (NUM_ORIGINAL_SELECT_NAMES+num_new_select_names)*sizeof(uint32_t);
		extended_size_sn += (NUM_ORIGINAL_SELECT_NAMES+num_new_select_names)*0x20;
			
		extended_size += extended_size_sn;
	}	
	
	out = new uint8_t[size+extended_size];
	if (!out)
	{
		DPRINTF("%s: Memory allocation error.\n", __PRETTY_FUNCTION__);
		return false;
	}
	
	memcpy(out, buf, size);
	delete[] buf;
	buf = nullptr;	
	
	if (num_new_select_names > 0)
		build_vs_ems_vsn(out, size, num_new_select_names, vsn_first_new_entry);
	
	bool ret = Utils::WriteFileBool(VS_EMS_OUT, out, size+extended_size, true, true);
	delete[] out;
	
	return ret;	
}

bool build_vs_files(VscFile *vsc)
{
	uint32_t vsn_first_new_entry;
	
	if (!build_vs_emb(vsc, &vsn_first_new_entry))
		return false;
	
	if (!build_vs_ems(vsc, vsn_first_new_entry))
		return false;	
	
	return true;
}

void process_vs(SlotsFile *slf)
{
	VscFile vsc;
	
	if (!vsc.LoadFromFile(VS_CACHE, false))
	{
		build_vs_cache(slf, &vsc);
		
		if (vsc.IsEmpty())
			return;
		
		if (!build_vs_files(&vsc))
			return;
		
		vsc.SaveToFile(VS_CACHE, true, true);
	}
	
	// Nothing to do here, select_names id are already done by process_cha_sel
}


void process_ui(SlotsFile *slf)
{
	process_battle_steam(slf);
	process_chr_name(slf);
	process_cha_sel(slf);
	process_gwr(slf);
	process_gwt(slf);
	process_vs(slf);
}

static void instructions_sanity_check()
{
	uint8_t b =  read_mem_rel8(CPTR_GALAXIAN_WARS_LIST_SYMBOL-1);
	uint8_t b2 = read_mem_rel8(CPTR_GALAXIAN_WARS_LIST_P4_SYMBOL-1);
	uint8_t b3 = read_mem_rel8(CPTR_GALAXIAN_WARS_LIST_P8_SYMBOL-2);
	//uint8_t b4 = read_mem_rel8(CPTR_CHARACTERS_INFO4_SYMBOL4-3);
	
	if (b != 0xBF || b2 != 0xB8 || b3 != 0x8B /*|| b4 != 0x8B*/)
	{
		DPRINTF("Error in instructions of first group.\n");
		ExitProcess(-1);
	}
	
	b =  read_mem_rel8(CPTR_GALAXIAN_WARS_LIST_AFTER_SYMBOL-2);
	b2 = read_mem_rel8(CPTR_GALAXIAN_WARS_LIST_AFTER_P4_SYMBOL-1);
		
	if (b != 0x81 || b2 != 0x3D)
	{
		DPRINTF("Error in instructions of second group.\n");
		ExitProcess(-1);
	}
	
	/*b =  read_mem_rel8(CPTR_CHARACTERS_INFO4_P8_SYMBOL-3);
	b2 = read_mem_rel8(CPTR_CHARACTERS_INFO4_PC_SYMBOL-2);
		
	if (b != 0xD9 || b2 != 0xD9)
	{
		DPRINTF("Error in instructions of third group.\n");
		ExitProcess(-1);
	}
	
	b =  read_mem_rel8(CPTR_CHARACTERS_INFO4_P10_SYMBOL-2);
	b2 = read_mem_rel8(CPTR_CHARACTERS_INFO4_P14_SYMBOL-3);
		
	if (b != 0x8B || b2 != 0x8B)
	{
		DPRINTF("Error in instructions of fourth group.\n");
		ExitProcess(-1);
	}
	
	b =  read_mem_rel8(CPTR_CHARACTERS_INFO4_SIZE_SYMBOL-2);
		
	if (b != 0x81)
	{
		DPRINTF("Error in instructions of fifth group.\n");
		ExitProcess(-1);
	}*/
}

void character_patches()
{
	CmsFile cms;
	CdcFile cdc;
	CspFile csp;
	RpdFile rpd;
	SlotsFile slf;
	GpdFile gpd;
	GwdFile gwd;
	BgrFile bgr;
	
	TiXmlDocument doc_cms, doc_cdc, doc_csp, doc_rpd, doc_slf, doc_gpd, doc_bgr;
	bool gpd_available = false;
	bool bgr_available = false;
	
	mod_debug_level(2);
	assert(sizeof(size_t) == sizeof(uint32_t));
	
	if (!doc_cms.LoadFile(CMS_XML_FILE))
	{
		if (doc_cms.ErrorId() == TiXmlBase::TIXML_ERROR_OPENING_FILE)
		{
			DPRINTF("Cannot load file %s. Character patches will be disabled.\n", CMS_XML_FILE);
			mod_debug_level(-2);
			return;
		}
		
		DPRINTF("Error parsing file \"%s\". This is what tinyxml has to say: %s. Row=%d, col=%d.\n", CMS_XML_FILE, doc_cms.ErrorDesc(), doc_cms.ErrorRow(), doc_cms.ErrorCol());
		ExitProcess(-1);		
	}
	
	if (!doc_cdc.LoadFile(CDC_XML_FILE))
	{
		if (doc_cdc.ErrorId() == TiXmlBase::TIXML_ERROR_OPENING_FILE)
		{
			DPRINTF("Cannot load file %s. Character patches will be disabled.\n", CDC_XML_FILE);
			mod_debug_level(-2);
			return;
		}
		
		DPRINTF("Error parsing file \"%s\". This is what tinyxml has to say: %s. Row=%d, col=%d.\n", CDC_XML_FILE, doc_cdc.ErrorDesc(), doc_cdc.ErrorRow(), doc_cdc.ErrorCol());
		ExitProcess(-1);		
	}
	
	if (!doc_csp.LoadFile(CSP_XML_FILE))
	{
		if (doc_csp.ErrorId() == TiXmlBase::TIXML_ERROR_OPENING_FILE)
		{
			DPRINTF("Cannot load file %s. Character patches will be disabled.\n", CSP_XML_FILE);
			mod_debug_level(-2);
			return;
		}
		
		DPRINTF("Error parsing file \"%s\". This is what tinyxml has to say: %s. Row=%d, col=%d.\n", CSP_XML_FILE, doc_csp.ErrorDesc(), doc_csp.ErrorRow(), doc_csp.ErrorCol());
		ExitProcess(-1);		
	}
	
	if (!doc_rpd.LoadFile(RPD_XML_FILE))
	{
		if (doc_rpd.ErrorId() == TiXmlBase::TIXML_ERROR_OPENING_FILE)
		{
			DPRINTF("Cannot load file %s. Character patches will be disabled.\n", RPD_XML_FILE);
			mod_debug_level(-2);
			return;
		}
		
		DPRINTF("Error parsing file \"%s\". This is what tinyxml has to say: %s. Row=%d, col=%d.\n", RPD_XML_FILE, doc_rpd.ErrorDesc(), doc_rpd.ErrorRow(), doc_rpd.ErrorCol());
		ExitProcess(-1);		
	}
	
	if (!doc_slf.LoadFile(SLF_XML_FILE))
	{
		if (doc_slf.ErrorId() == TiXmlBase::TIXML_ERROR_OPENING_FILE)
		{
			DPRINTF("Cannot load file %s. Character patches will be disabled.\n", SLF_XML_FILE);
			mod_debug_level(-2);
			return;
		}
		
		DPRINTF("Error parsing file \"%s\". This is what tinyxml has to say: %s. Row=%d, col=%d.\n", SLF_XML_FILE, doc_slf.ErrorDesc(), doc_slf.ErrorRow(), doc_slf.ErrorCol());
		ExitProcess(-1);		
	}
	
	if (!doc_gpd.LoadFile(GPD_XML_FILE))
	{
		if (doc_gpd.ErrorId() != TiXmlBase::TIXML_ERROR_OPENING_FILE)
		{
			DPRINTF("Error parsing file \"%s\". This is what tinyxml has to say: %s. Row=%d, col=%d.\n", GPD_XML_FILE, doc_slf.ErrorDesc(), doc_slf.ErrorRow(), doc_slf.ErrorCol());
			ExitProcess(-1);		
		}
	}
	else
	{
		gpd_available = true;
	}
	
	if (!doc_bgr.LoadFile(BGR_XML_FILE))
	{
		if (doc_gpd.ErrorId() != TiXmlBase::TIXML_ERROR_OPENING_FILE)
		{
			DPRINTF("Error parsing file \"%s\". This is what tinyxml has to say: %s. Row=%d, col=%d.\n", BGR_XML_FILE, doc_slf.ErrorDesc(), doc_slf.ErrorRow(), doc_slf.ErrorCol());
			ExitProcess(-1);		
		}
	}
	else
	{
		bgr_available = true;
	}
	
	if (!cms.Compile(&doc_cms))
	{
		DPRINTF("Error compiling %s.\n", CMS_XML_FILE);
		ExitProcess(-1);
	}
	
	if (!cdc.Compile(&doc_cdc))
	{
		DPRINTF("Error compiling %s.\n", CDC_XML_FILE);
		ExitProcess(-1);
	}
	
	if (!csp.Compile(&doc_csp))
	{
		DPRINTF("Error compiling %s. \n", CSP_XML_FILE);
		ExitProcess(-1);
	}
	
	if (!rpd.Compile(&doc_rpd))
	{
		DPRINTF("Error compiling %s.\n", RPD_XML_FILE);
		ExitProcess(-1);
	}
	
	if (!slf.Compile(&doc_slf))
	{
		DPRINTF("Error compiling %s.\n", SLF_XML_FILE);
		ExitProcess(-1);
	}
	
	if (!slf.CheckModels(&cms))
	{
		DPRINTF("CheckModels returned error.\n");
		ExitProcess(-1);		
	}
	
	if (!slf.HasCI3() || !slf.HasCI4())
	{
		DPRINTF("slots_data.slf.xml is not compatible with this version of sssspatcher!\n"
				"Run SS Mods Installer at least one to upgrade the xml format.\n");
				
		ExitProcess(-1);
	}
	
	if (gpd_available)
	{
		if (!gpd.Compile(&doc_gpd))
		{
			DPRINTF("Error compiling %s.\n", GPD_XML_FILE);
			ExitProcess(-1);
		}
		
		if (!gwd.Load(gw_data, sizeof(gw_data)))
		{
			DPRINTF("Weird, load of internal gw_data failed.\n");
			ExitProcess(-1);
		}
		
		if (!gpd.SetGwdTournaments(gwd))
		{
			DPRINTF("SetGwdTournaments unexpectedly failed.\n");
			ExitProcess(-1);
		}
	}
	
	if (bgr_available)
	{
		if (!bgr.Compile(&doc_bgr))
		{
			DPRINTF("Error compiling %s.\n", BGR_XML_FILE);
			ExitProcess(-1);
		}
	}

	process_ui(&slf);
	
	SlotsSaveData save_data;	
	//
	// slots_buf -> can be deleted
	// ci_buf -> Permanently on RAM, don't delete[]
	// ci2_buf -> Permanently on RAM don't delete[]
	// ci3_buf -> Permanently on RAM, don't delete[]
	// ci4_buf -> Permanently on RAM, don't delete[]
	
	if (!slf.CreateFile(&save_data))
	{
		DPRINTF("%s: slf.CreateFile failed.\n", __PRETTY_FUNCTION__);
		ExitProcess(-1);
	}	
	
	if (save_data.slots_size != SLOTS_DATA_SIZE)
	{
		DPRINTF("Error: size of slot data doesn't match. You probably added or removed CharacterSlot!\n");
		ExitProcess(-1);
	}
	
	uint8_t *ptr_lists[NUM_CHARACTERS_LISTS_NOT_EMPTY];
	unsigned int lists_size;
	
	if (!slf.CreateLists(&lists_size, ptr_lists))
	{
		DPRINTF("CreateLists failed.\n");
		ExitProcess(-1);
	}
	
	uint8_t *global_characters_list; // Permanently on RAM, don't delete[]
	unsigned int gcl_size;
	
	global_characters_list = rpd.CreateGlobalCharactersList(&gcl_size, &cms);
	if (!global_characters_list)
	{
		DPRINTF("CreateGlobalCharactersList failed. Either some model in the rpd doesn't exist in the CMS or some cms_entry was removed in some character.\n");
		ExitProcess(-1);
	}
	
	uint8_t *galaxian_wars_list = nullptr; // Permanently on RAM, don't delete[]
	unsigned int gwl_size = 0;
	
	if (gpd_available)
	{
		galaxian_wars_list = gpd.CreateGalaxianWarsList(&gwl_size, &cms);
	}
	
	if (!cms.SaveToFile(CMS_OUT_FILE, true, true))
	{
		ExitProcess(-1);
		return;
	}
	
	if (!cdc.SaveToFile(CDC_OUT_FILE, true, true))
	{
		ExitProcess(-1);
		return;
	}
	
	if (!csp.SaveToFile(CSP_OUT_FILE, true, true))
	{
		ExitProcess(-1);
		return;
	}
	
	if (!rpd.SaveToFile(RPD_OUT_FILE, true, true))
	{
		ExitProcess(-1);
		return;
	}
	
	if (gpd_available)
	{
		if (!gpd.SaveToFile(GPD_OUT_FILE, true, true))
		{
			ExitProcess(-1);
			return;
		}
		
		if (!gwd.SaveToFile(GWD_OUT_FILE, true, true))
		{
			ExitProcess(-1);
			return;
		}
	}
	else
	{
		Utils::RemoveFile(GPD_OUT_FILE);
		Utils::RemoveFile(GWD_OUT_FILE);
	}
	
	if (bgr_available)
	{
		if (!bgr.SaveToFile(BGR_OUT_FILE, true, true))
		{
			ExitProcess(-1);
			return;
		}
	}
	else
	{
		Utils::RemoveFile(BGR_OUT_FILE);
	}
	
	// Patch slots
	memcpy_dst(SLOTS_DATA_SYMBOL, save_data.slots_buf, SLOTS_DATA_COPY_SIZE);
	delete[] save_data.slots_buf;
	
	instructions_sanity_check();
	
	// Patch character info
	write_mem_rel32(CPTR_CHARACTERS_INFO_SYMBOL, (uint32_t)save_data.ci_buf);
	write_mem_rel32(CPTR_CHARACTERS_INFO_SYMBOL2, (uint32_t)save_data.ci_buf);
	write_mem_rel32(CPTR_CHARACTERS_INFO_SYMBOL3, (uint32_t)save_data.ci_buf);
	write_mem_rel32(CPTR_CHARACTERS_INFO_SYMBOL4, (uint32_t)save_data.ci_buf);
	write_mem_rel32(CPTR_CHARACTERS_INFO_SYMBOL5, (uint32_t)save_data.ci_buf);
	write_mem_rel32(CPTR_CHARACTERS_INFO_SYMBOL6, (uint32_t)save_data.ci_buf);
	write_mem_rel32(CPTR_CHARACTERS_INFO_SYMBOL7, (uint32_t)save_data.ci_buf);	
	write_mem_rel32(CPTR_CHARACTERS_INFO_P4_SYMBOL, (uint32_t)save_data.ci_buf+4);
	write_mem_rel32(CPTR_CHARACTERS_INFO_P4_SYMBOL2, (uint32_t)save_data.ci_buf+4);
	write_mem_rel32(CPTR_CHARACTERS_INFO_P8_SYMBOL, (uint32_t)save_data.ci_buf+8);
	write_mem_rel32(CPTR_CHARACTERS_INFO_PC_SYMBOL, (uint32_t)save_data.ci_buf+0xC);
	write_mem_rel32(CPTR_CHARACTERS_INFO_P10_SYMBOL, (uint32_t)save_data.ci_buf+0x10);
	
	// Patch lists
	uint32_t current_list_ptr = PTR_CHARACTERS_LISTS_SYMBOL;
	for (unsigned int i = 0; i < NUM_CHARACTERS_LISTS_NOT_EMPTY; i++)
	{
		write_mem_rel32(current_list_ptr, (uint32_t)ptr_lists[i]);
		current_list_ptr += 8;
	}
	
	// Patch global characters list
	write_mem_rel32(CPTR_GLOBAL_CHARACTERS_LIST_SYMBOL, (uint32_t)global_characters_list);
	write_mem_rel32(CPTR_GLOBAL_CHARACTERS_LIST_SYMBOL2, (uint32_t)global_characters_list);
	write_mem_rel32(CPTR_GLOBAL_CHARACTERS_LIST_P4_SYMBOL, (uint32_t)global_characters_list+4);
	
	// Patch character info 2
	write_mem_rel32(CPTR_CHARACTERS_INFO2_SYMBOL, (uint32_t)save_data.ci2_buf);
	write_mem_rel32(CPTR_CHARACTERS_INFO2_SYMBOL2, (uint32_t)save_data.ci2_buf);
	write_mem_rel32(CPTR_CHARACTERS_INFO2_SYMBOL3, (uint32_t)save_data.ci2_buf);
	write_mem_rel32(CPTR_CHARACTERS_INFO2_SYMBOL4, (uint32_t)save_data.ci2_buf);
	write_mem_rel32(CPTR_CHARACTERS_INFO2_P4_SYMBOL, (uint32_t)save_data.ci2_buf+4);
	write_mem_rel32(CPTR_CHARACTERS_INFO2_P4_SYMBOL2, (uint32_t)save_data.ci2_buf+4);
	write_mem_rel32(CPTR_CHARACTERS_INFO2_P8_SYMBOL, (uint32_t)save_data.ci2_buf+8);
	write_mem_rel32(CPTR_CHARACTERS_INFO2_P8_SYMBOL2, (uint32_t)save_data.ci2_buf+8);
	write_mem_rel32(CPTR_CHARACTERS_INFO2_PC_SYMBOL, (uint32_t)save_data.ci2_buf+0xC);
	write_mem_rel32(CPTR_CHARACTERS_INFO2_PC_SYMBOL2, (uint32_t)save_data.ci2_buf+0xC);
	write_mem_rel32(CPTR_CHARACTERS_INFO2_P10_SYMBOL, (uint32_t)save_data.ci2_buf+0x10);
	write_mem_rel32(CPTR_CHARACTERS_INFO2_P10_SYMBOL2, (uint32_t)save_data.ci2_buf+0x10);
	
	// Patch character info 3
	write_mem_rel32(CPTR_CHARACTERS_INFO3_SYMBOL, (uint32_t)save_data.ci3_buf);
	write_mem_rel32(CPTR_CHARACTERS_INFO3_SYMBOL2, (uint32_t)save_data.ci3_buf);
	write_mem_rel32(CPTR_CHARACTERS_INFO3_SYMBOL3, (uint32_t)save_data.ci3_buf);
	write_mem_rel32(CPTR_CHARACTERS_INFO3_P4_SYMBOL, (uint32_t)save_data.ci3_buf+4);
	write_mem_rel32(CPTR_CHARACTERS_INFO3_P4_SYMBOL2, (uint32_t)save_data.ci3_buf+4);
	write_mem_rel32(CPTR_CHARACTERS_INFO3_P4_SYMBOL3, (uint32_t)save_data.ci3_buf+4);
	write_mem_rel32(CPTR_CHARACTERS_INFO3_P8_SYMBOL, (uint32_t)save_data.ci3_buf+8);
	write_mem_rel32(CPTR_CHARACTERS_INFO3_PC_SYMBOL, (uint32_t)save_data.ci3_buf+0xC);
	write_mem_rel32(CPTR_CHARACTERS_INFO3_P10_SYMBOL, (uint32_t)save_data.ci3_buf+0x10);
	write_mem_rel32(CPTR_CHARACTERS_INFO3_P14_SYMBOL, (uint32_t)save_data.ci3_buf+0x14);
	
	write_mem_rel32(CPTR_CHARACTERS_INFO3_SIZE_SYMBOL, save_data.ci3_size);
	write_mem_rel32(CPTR_CHARACTERS_INFO3_SIZE_SYMBOL2, save_data.ci3_size);
	write_mem_rel32(CPTR_CHARACTERS_INFO3_SIZE_SYMBOL3, save_data.ci3_size);	
	
	// Patch character info 4
	write_mem_rel32(CPTR_CHARACTERS_INFO4_SYMBOL, (uint32_t)save_data.ci4_buf);
	write_mem_rel32(CPTR_CHARACTERS_INFO4_SYMBOL2, (uint32_t)save_data.ci4_buf);
	write_mem_rel32(CPTR_CHARACTERS_INFO4_SYMBOL3, (uint32_t)save_data.ci4_buf);
	write_mem_rel32(CPTR_CHARACTERS_INFO4_SYMBOL4, (uint32_t)save_data.ci4_buf);	
	write_mem_rel32(CPTR_CHARACTERS_INFO4_P4_SYMBOL, (uint32_t)save_data.ci4_buf+4);
	write_mem_rel32(CPTR_CHARACTERS_INFO4_P4_SYMBOL2, (uint32_t)save_data.ci4_buf+4);
	write_mem_rel32(CPTR_CHARACTERS_INFO4_P8_SYMBOL, (uint32_t)save_data.ci4_buf+8);
	write_mem_rel32(CPTR_CHARACTERS_INFO4_PC_SYMBOL, (uint32_t)save_data.ci4_buf+0xC);
	write_mem_rel32(CPTR_CHARACTERS_INFO4_P10_SYMBOL, (uint32_t)save_data.ci4_buf+0x10);
	write_mem_rel32(CPTR_CHARACTERS_INFO4_P14_SYMBOL, (uint32_t)save_data.ci4_buf+0x14);
	
	write_mem_rel32(CPTR_CHARACTERS_INFO4_SIZE_SYMBOL, save_data.ci4_size);
	
	// Patch galaxian wars list
	if (gpd_available)
	{
		write_mem_rel32(CPTR_GALAXIAN_WARS_LIST_SYMBOL, (uint32_t)galaxian_wars_list);
		write_mem_rel32(CPTR_GALAXIAN_WARS_LIST_P4_SYMBOL, (uint32_t)galaxian_wars_list+4);
		write_mem_rel32(CPTR_GALAXIAN_WARS_LIST_P8_SYMBOL, (uint32_t)galaxian_wars_list+8);
		write_mem_rel32(CPTR_GALAXIAN_WARS_LIST_AFTER_SYMBOL, (uint32_t)galaxian_wars_list+gwl_size);
		write_mem_rel32(CPTR_GALAXIAN_WARS_LIST_AFTER_P4_SYMBOL, (uint32_t)galaxian_wars_list+gwl_size+4);
		//DPRINTF("GPD Patch done. Size = %x\n", gwl_size);
	}
	
	// Time for the big patch, extend models data.
	models_data = new uint8_t[MAX_NUMBER_OF_MODELS*sizeof(ModelStruct)]; // Permanently on RAM
	
	mod_debug_level(-2);
	
	num_models = cdc.GetNumEntries()-1;
	
	DPRINTF("num_models: 0x%x\n", num_models);
	
	write_mem_rel32(CPTR_MODELS_DATA_SYMBOL, (uint32_t)models_data);
	write_mem_rel32(CPTR_MODELS_DATA_SYMBOL2, (uint32_t)models_data);
	write_mem_rel32(CPTR_MODELS_DATA_SYMBOL3, (uint32_t)models_data);
	write_mem_rel32(CPTR_MODELS_DATA_SYMBOL4, (uint32_t)models_data);
	write_mem_rel32(CPTR_MODELS_DATA_SYMBOL5, (uint32_t)models_data);
	write_mem_rel32(CPTR_MODELS_DATA_SYMBOL6, (uint32_t)models_data);
	write_mem_rel32(CPTR_MODELS_DATA_PC_SYMBOL, (uint32_t)models_data + 0xC);
	write_mem_rel32(CPTR_MODELS_DATA_AFTER_SYMBOL, (uint32_t)models_data + (num_models)*sizeof(ModelStruct));
	write_mem_rel32(CPTR_NUM_MODELS_SYMBOL, num_models);
	write_mem_rel32(CPTR_MODELS_DATA_SIZE_SYMBOL, (num_models)*sizeof(ModelStruct));
	
	hook_function(IS_CHARACTER_UNLOCKED2, (void **)&is_character_unlocked, (void *)is_character_unlocked_patched);
	
	// Patch random characters
	
	if (!slf.IsCurrentRandomSlot(RANDOM_SLOT_UPPER_RIGHT))
	{
		write_mem_rel8(CPTR_RANDOM_SLOT_UPPER_RIGHT, 0xFF);
	}
	
	if (!slf.IsCurrentRandomSlot(RANDOM_SLOT_LOWER_LEFT))
	{
		write_mem_rel8(CPTR_RANDOM_SLOT_LOWER_LEFT, 0xFF);
	}
	
	if (!slf.IsCurrentRandomSlot(RANDOM_SLOT_LOWER_RIGHT))
	{
		write_mem_rel8(CPTR_RANDOM_SLOT_LOWER_RIGHT, 0xFF);
	}
	
	DPRINTF("Characters patch done succesfully.\n");	
	
	// *****  PATCHES PLAY GROUND ***** //
	
	/*IniFile ini;
	uint32_t address, value;
	
	ini.LoadFromFile(CONFIGURATION_FILE);
	ini.GetIntegerValue("TEST", "patch_address", (int *)&address);
	ini.GetIntegerValue("TEST", "patch_value", (int *)&value);
	
	UPRINTF("Address = %x, value = %x\n", address, value);
	
	write_mem_rel32(address-MY_DUMP_BASE, value); */
	
	/*std::vector<uint32_t> addresses =
	{
		//0x213BD08,   // BINGO
		//0x213BE70,
		//0x2159F90,*/
		/*0x2A4FD1C,
		0x2A628E8,
		0x2A62CF0,*/
		/*0x2A63640,
		0x2A65858,
		0x2A65DA8,
		0x2A66230,
		0x2A676A0,*/
		/*0x2A676AC,
		0x2A6C3A4,		
		0x2A71C88,
		0x2A72E50,
		0x2A77500,
		0x2A78910,
		0x2A7A61C,
		0x2A7FDB8,
		0x2A8F228,
		0x2A8FC90,
		0x2F6ADF0,
		0x2F6AE64,
	};*/
	
	/*for (uint32_t ad : addresses)
	{
		write_mem_rel32(ad-MY_DUMP_BASE, 0x49);
	}*/
}

/*void *(* load_file)(char *, int unk);

void *my_load_file(char *s, int unk)
{
	DPRINTF("load_file %s\n", s);
	
	if (s && strcmp(s, "resource\\ui\\SP\\battle_STEAM.emb") == 0)
	{
		DumpStackTrace5();		
	}

	return load_file(s, unk);
}*/

void patches()
{
	readfile_import = (void **)get_module_import("KERNEL32.dll", "ReadFile");
	if (!readfile_import)
	{
		DPRINTF("Cannot find ReadFile import!.\n");
		return;
	}
	
	original_readfile = *readfile_import;	
	DPRINTF("Patch at %p\n", readfile_import);
	write_mem32((void *)readfile_import, (uint32_t)ReadFile_patched);		

#if defined(SSSS)
	hook_function(CPK_FILE_EXISTS_SYMBOL, (void **)&cpk_file_exists, (void *)cpk_file_exists_patched);		
#endif

#ifdef DEBUG
	hook_function(CPK_OPEN_FILE_SYMBOL, (void **)&open_cpk_file, (void *)my_open_cpk_file);
#endif

	//hook_function(0x176C290-MY_DUMP_BASE, (void **)&load_file, (void *)my_load_file);
	
#if defined(SSSS)
	
	if (do_controller_patch)
	{
		void **pXInputGetState = (void **) ((uint32_t) GetModuleHandle(NULL) + XINPUT_GETSTATE_SYMBOL); //        
		XInputGetState = (DWORD (WINAPI *)(DWORD, void *)) *pXInputGetState;	
	
		DPRINTF("Address of XinputGetState = %p\n", XInputGetState);
		write_mem32((void *)pXInputGetState, (uint32_t)XInputGetState_Patched);			
	}
	
#endif
	
}

void read_config()
{
	IniFile ini;
	int idx;
	
	if (!ini.LoadFromFile(CONFIGURATION_FILE, false))
		return;
	
	ini.GetBooleanValue("GENERAL", "unlock_characters", &do_unlock_patch, false);
	ini.GetBooleanValue("CONTROLLER", "controller_patch", &do_controller_patch, false);	
	
	ini.GetIntegerValue("CONTROLLER", "controller_index", &idx, 1);
		
	DPRINTF("CONFIG: unlock_characters = %d, controller_patch = %d, controller_index = %d\n", do_unlock_patch, do_controller_patch, controller_index);
	
}

extern "C" BOOL EXPORT DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	switch (fdwReason)
	{
		case DLL_PROCESS_ATTACH:
		
#ifdef DEBUG
			set_debug_level(1);
#endif

			DPRINTF("Hello world. Exe base = %p\n", GetModuleHandle(NULL));
			
			if (read_mem_rel32(CPK_FILE_EXISTS_SYMBOL) != 0x83EC8B55)
			{
				mod_debug_level(2);
				DPRINTF("sssspatcher is not compatible with this game version.\n");
				ExitProcess(-1);
			}
		
			if (InGameProcess())
			{
				if (!LoadDllAndResolveExports())
					return FALSE;
				
				read_config();
				
				CpkFile *cpks[NUM_CPK];				
				
				if (get_cpk_tocs(cpks))
				{
					character_patches();
					
					for (int i = 0; i < NUM_CPK; i++)
					{
						patch_toc(cpks[i]);						
						cpks[i]->RevertEncryption(false);
						delete cpks[i];
					}					
					
					patches();
#ifdef DEBUG
					debug_patches();
#endif					
				}				
			}		
			
		break;
		
		case DLL_PROCESS_DETACH:		
			
			if (!lpvReserved)
				UnloadDll();
			
		break;
	}
	
	return TRUE;
}
