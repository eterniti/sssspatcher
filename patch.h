#include <windows.h>
#include <stdint.h>

void *get_ptr(uint32_t rel_address, const char *mod=NULL);

uint8_t read_mem_rel8(uint32_t rel_address, const char *mod=NULL);
uint32_t read_mem_rel32(uint32_t rel_address, const char *mod=NULL);

void write_mem32(void *address, uint32_t data);
void write_mem_rel8(uint32_t rel_address, uint8_t data, const char *mod=NULL);
void write_mem_rel32(uint32_t rel_address, uint32_t data, const char *mod=NULL);

void memcpy_dst(uint32_t dst, void *src, unsigned int size, const char *mod=NULL);
void memcpy_src(void *dst, uint32_t src, unsigned int size, const char *mod=NULL);

void memset_dst(uint32_t dst, uint8_t value, unsigned int size, const char *mod=NULL);

PVOID get_module_import(LPCSTR lpModName, LPCSTR lpProcName, const char *mod=NULL);
PVOID get_module_import_by_ordinal(LPCSTR lpModName, DWORD ordinal, const char *mod=NULL);

long hook_function(uint32_t rel_addr, void **orig, void *newfunc, const char *mod=NULL);