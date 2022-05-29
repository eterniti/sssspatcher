CC = gcc
CXX = g++

OUTPUT := steam_api.dll
OBJS=main.o debug.o patch.o asm.o 
OBJS += ../eternity_common/crypto/sha1.o ../eternity_common/crypto/md5.o
OBJS += ../eternity_common/Utils.o ../eternity_common/SSSS/SsssData.o ../eternity_common/BaseFile.o ../eternity_common/IniFile.o CpkFile.o 
OBJS += ../eternity_common/SSSS/CmsFile.o ../eternity_common/SSSS/CdcFile.o ../eternity_common/SSSS/CscFile.o ../eternity_common/SSSS/CspFile.o ../eternity_common/SSSS/RpdFile.o ../eternity_common/SSSS/SlotsFile.o 
OBJS += ../eternity_common/SSSS/GpdFile.o ../eternity_common/SSSS/GwdFile.o ../eternity_common/SSSS/BgrFile.o
OBJS += ../eternity_common/Dimps/EmbFile.o ../eternity_common/SSSS/TdbFile.o ../eternity_common/SSSS/SstFile.o
OBJS += ../eternity_common/SSSS/BscFile.o ../eternity_common/SSSS/CncFile.o ../eternity_common/SSSS/ChcFile.o ../eternity_common/SSSS/GrcFile.o ../eternity_common/SSSS/GtcFile.o ../eternity_common/SSSS/VscFile.o
OBJS += ../eternity_common/tinyxml/tinyxml.o ../eternity_common/tinyxml/tinystr.o ../eternity_common/tinyxml/tinyxmlerror.o ../eternity_common/tinyxml/tinyxmlparser.o
CFLAGS +=-Wall -I. -I../eternity_common -std=gnu99 -mno-ms-bitfields -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 -O2 -masm=intel -shared -s -Wl,--subsystem,windows,--kill-at,--enable-stdcall-fixup
CFLAGS += -static -static-libgcc -static-libstdc++
CFLAGS += -DSSSS
CPPFLAGS +=-Wall -I../eternity_common -O2 -mno-ms-bitfields -std=c++11 -DTIXML_USE_STL -DNO_CRYPTO -DNO_ZLIB
CPPFLAGS += -static-libgcc -static-libstdc++ -static -Wl,--subsystem,windows,--kill-at
CPPFLAGS += -DSSSS 
LDFLAGS=-L. -static
LIBS = -lstdc++  -ldetours -lversion

all: $(OUTPUT)	

clean:
	rm -f $(OUTPUT) *.o
	rm -f ../eternity_common/*.o
	rm -f ../eternity_common/SSSS/*.o
	rm -f ../eternity_common/Criware/*.o
	rm -f ../eternity_common/Dimps/*.o
	rm -f ../eternity_common/tinyxml/*.o	
	rm -f ../eternity_common/crypto/*.o

$(OUTPUT): $(OBJS)
	$(LINK.c) $(LDFLAGS) -o $@ $^ $(LIBS)
	cp $(OUTPUT) ../ssssmins/bin/steam_api.dll
