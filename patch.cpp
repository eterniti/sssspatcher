#include <windows.h>
#include <stdint.h>
#include <stdlib.h>

#include <detours/detours.h>

#include "patch.h"
#include "debug.h"

void *get_ptr(uint32_t rel_address, const char *mod)
{
	uint8_t *mod_top = (uint8_t *)GetModuleHandle(mod);	
	if (!mod_top)
		return NULL;
	
	return (void *)(mod_top+rel_address);
}

uint8_t read_mem_rel8(uint32_t rel_address, const char *mod)
{
	uint8_t *mod_top = (uint8_t *)GetModuleHandle(mod);	
	if (!mod_top)
		return 0;
	
	return *(mod_top+rel_address);
}

uint32_t read_mem_rel32(uint32_t rel_address, const char *mod)
{
	uint8_t *mod_top = (uint8_t *)GetModuleHandle(mod);	
	if (!mod_top)
		return 0;
	
	return *(uint32_t *)(mod_top+rel_address);
}

void write_mem32(void *address, uint32_t data)
{
	DWORD lOldProtect;
	
	VirtualProtect(address, 4, PAGE_EXECUTE_READWRITE, &lOldProtect);
	*(uint32_t *) address = data;
	VirtualProtect(address, 4, lOldProtect, &lOldProtect);
}

void write_mem_rel8(uint32_t rel_address, uint8_t data, const char *mod)
{
	DWORD lOldProtect;
	
	uint8_t *mod_top = (uint8_t *)GetModuleHandle(mod);	
	if (!mod_top)
		return;
	
	uint8_t *address = mod_top + rel_address;
	
	VirtualProtect(address, 4, PAGE_EXECUTE_READWRITE, &lOldProtect);
	*address = data;
	VirtualProtect(address, 4, lOldProtect, &lOldProtect);
}

void write_mem_rel32(uint32_t rel_address, uint32_t data, const char *mod)
{
	DWORD lOldProtect;
	
	uint8_t *mod_top = (uint8_t *)GetModuleHandle(mod);	
	if (!mod_top)
		return;
	
	uint32_t *address = (uint32_t *)(mod_top + rel_address);
	
	VirtualProtect(address, 4, PAGE_EXECUTE_READWRITE, &lOldProtect);
	*address = data;
	VirtualProtect(address, 4, lOldProtect, &lOldProtect);
}

void memcpy_dst(uint32_t dst, void *src, unsigned int size, const char *mod)
{
	DWORD lOldProtect;
	
	uint8_t *mod_top = (uint8_t *)GetModuleHandle(mod);	
	if (!mod_top)
		return;
	
	uint8_t *address = mod_top + dst;
	
	VirtualProtect(address, size, PAGE_EXECUTE_READWRITE, &lOldProtect);
	memcpy(address, src, size);
	VirtualProtect(address, size, lOldProtect, &lOldProtect);
}

void memcpy_src(void *dst, uint32_t src, unsigned int size, const char *mod)
{
	uint8_t *mod_top = (uint8_t *)GetModuleHandle(mod);	
	if (!mod_top)
		return;
	
	uint8_t *address = mod_top + src;
	
	memcpy(dst, address, size);	
}

void memset_dst(uint32_t dst, uint8_t value, unsigned int size, const char *mod)
{
	DWORD lOldProtect;
	
	uint8_t *mod_top = (uint8_t *)GetModuleHandle(mod);	
	if (!mod_top)
		return;
	
	uint8_t *address = mod_top + dst;
	
	VirtualProtect(address, size, PAGE_EXECUTE_READWRITE, &lOldProtect);
	memset(address, value, size);
	VirtualProtect(address, size, lOldProtect, &lOldProtect);
}

PVOID get_module_import(LPCSTR lpModName, LPCSTR lpProcName, const char *mod)
{
	PUINT8 pModTop;
	PIMAGE_DOS_HEADER pDosHdr;
	PIMAGE_NT_HEADERS pNtHdr;
	PIMAGE_IMPORT_DESCRIPTOR pImportDir;
	
	HMODULE hModule = GetModuleHandle(mod);
		
	pModTop = (PUINT8) hModule;
	pDosHdr = (PIMAGE_DOS_HEADER) pModTop;
	pNtHdr = (PIMAGE_NT_HEADERS) (pModTop + pDosHdr->e_lfanew);
	pImportDir = (PIMAGE_IMPORT_DESCRIPTOR) (pModTop + pNtHdr->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);
		
	while (pImportDir->OriginalFirstThunk != 0)
	{
		PCHAR pName = (PCHAR) (pModTop + pImportDir->Name);
		
		if (strcasecmp(pName, lpModName) == 0)
		{
			PIMAGE_THUNK_DATA pThunkData;
						
			pThunkData = (PIMAGE_THUNK_DATA) (pModTop + pImportDir->OriginalFirstThunk);
			
			while (pThunkData->u1.AddressOfData != 0)
			{
				PIMAGE_IMPORT_BY_NAME pImportByName = (PIMAGE_IMPORT_BY_NAME) (pModTop + pThunkData->u1.AddressOfData);
				
				if (strcmp((char *)pImportByName->Name, lpProcName) == 0)
				{
					return (PVOID) ((DWORD) (pThunkData) + pImportDir->FirstThunk - pImportDir->OriginalFirstThunk);
				}
				
				pThunkData++;
			}
			
			break;
		}
		
		pImportDir++;		
	}		
	
	DPRINTF("Function not found.\n");
	return NULL;	
}

PVOID get_module_import_by_ordinal(LPCSTR lpModName, DWORD ordinal, const char *mod)
{
	PUINT8 pModTop;
	PIMAGE_DOS_HEADER pDosHdr;
	PIMAGE_NT_HEADERS pNtHdr;
	PIMAGE_IMPORT_DESCRIPTOR pImportDir;
	
	HMODULE hModule = GetModuleHandle(mod);
		
	pModTop = (PUINT8) hModule;
	pDosHdr = (PIMAGE_DOS_HEADER) pModTop;
	pNtHdr = (PIMAGE_NT_HEADERS) (pModTop + pDosHdr->e_lfanew);
	pImportDir = (PIMAGE_IMPORT_DESCRIPTOR) (pModTop + pNtHdr->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);
		
	while (pImportDir->OriginalFirstThunk != 0)
	{
		PCHAR pName = (PCHAR) (pModTop + pImportDir->Name);
		
		if (strcasecmp(pName, lpModName) == 0)
		{
			PIMAGE_THUNK_DATA pThunkData;
						
			pThunkData = (PIMAGE_THUNK_DATA) (pModTop + pImportDir->OriginalFirstThunk);
			//DPRINTF("pThunkData = %p\n", pThunkData);
						
			while (pThunkData->u1.Ordinal != 0)
			{
				//DPRINTF("Ordinal: %d\n", pThunkData->u1.Ordinal);
				
				if (pThunkData->u1.Ordinal == ordinal)
				{
					return (PVOID) ((DWORD) (pThunkData) + pImportDir->FirstThunk - pImportDir->OriginalFirstThunk);
				}
				
				pThunkData++;
			}
			
			break;
		}
		
		pImportDir++;		
	}		
	
	DPRINTF("Function not found.\n");
	return NULL;	
}


long hook_function(uint32_t rel_addr, void **orig, void *newfunc, const char *mod)
{
	HMODULE hMod = GetModuleHandle(mod);
	if (!hMod)
		return -1;
	
	if (orig)
	{	
		*orig = (void *)( ((uint32_t)hMod) + rel_addr );
	
		DetourTransactionBegin();	
		DetourAttach(orig, newfunc);
	}
	else
	{
		void *my_orig;

		my_orig = (void *)( ((uint32_t)hMod) + rel_addr );
		DetourTransactionBegin();	
		DetourAttach(&my_orig, newfunc);
	}
	
	return DetourTransactionCommit();
}
