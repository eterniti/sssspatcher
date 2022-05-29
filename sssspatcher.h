#ifndef __SSPATCHER_H__
#define __SSPATCHER_H__

#include "SSSS/CmsFile.h"
#include "SSSS/CdcFile.h"

typedef struct
{
	uint8_t *hdr_buf;
	uint8_t *toc_buf;
	uint64_t toc_offset;
	uint64_t toc_size;
	bool patched;
	const char *name;
} CpkDef;

#define CONFIGURATION_FILE	"sssspatcher.ini"

#ifdef SSSS

#define ORIGINAL_NUMBER_MODELS		0x9A
#define MAX_NUMBER_OF_MODELS		720	

#define PROCESS_NAME 	"ssss.exe"

#define CMS_XML_FILE		"cmn_model_spec.cms.xml"
#define CMS_OUT_FILE		"resource/system/cmn_model_spec.cms"

#define CDC_XML_FILE		"chara_data.cdc.xml"
#define CDC_OUT_FILE		"resource/system/chara_data.cdc"

#define CSP_XML_FILE		"CharacterSelectPosition.csp.xml"
#define CSP_OUT_FILE		"resource/character_select/CharacterSelectPosition.csp"

#define RPD_XML_FILE		"ResultPositionData.rpd.xml"
#define RPD_OUT_FILE		"resource/result/ResultPositionData.rpd"

#define SLF_XML_FILE		"slots_data.slf.xml"

#define GPD_XML_FILE		"resource/sssspatcher/GW_POSITION_DATA.gpd.xml"
#define GPD_OUT_FILE		"resource/duel/GW_POSITION_DATA.gpd"

#define GWD_OUT_FILE		"resource/duel/GW_DATA.gwd"

#define BGR_XML_FILE		"resource/sssspatcher/BattleOfGoldReverse.bgr.xml"
#define BGR_OUT_FILE		"resource/system/BattleOfGoldReverse.bgr"

static std::vector<std::string> languages =
{
	"JP",
	"EN",
	"FR",
	"IT",
	"SP",
	"BR",
	"CH",
	"NS"
};

/////////////////////////

#define BATTLE_STEAM_EMS_BASE		"resource/sssspatcher/base/battle_STEAM.ems"
#define BATTLE_STEAM_EMS_OUT		"resource/ui/battle_STEAM.ems"

#define BATTLE_STEAM_EMB_JP_BASE	"resource/sssspatcher/base/battle_STEAM_JP.emb"
#define BATTLE_STEAM_EMB_JP_OUT		"resource/ui/JP/battle_STEAM.emb"

#define BATTLE_STEAM_EMB_EN_BASE	"resource/sssspatcher/base/battle_STEAM_EN.emb"
#define BATTLE_STEAM_EMB_EN_OUT		"resource/ui/EN/battle_STEAM.emb"

#define BATTLE_STEAM_EMB_FR_BASE	"resource/sssspatcher/base/battle_STEAM_FR.emb"
#define BATTLE_STEAM_EMB_FR_OUT		"resource/ui/FR/battle_STEAM.emb"

#define BATTLE_STEAM_EMB_IT_BASE	"resource/sssspatcher/base/battle_STEAM_IT.emb"
#define BATTLE_STEAM_EMB_IT_OUT		"resource/ui/IT/battle_STEAM.emb"

#define BATTLE_STEAM_EMB_SP_BASE	"resource/sssspatcher/base/battle_STEAM_SP.emb"
#define BATTLE_STEAM_EMB_SP_OUT		"resource/ui/SP/battle_STEAM.emb"

#define BATTLE_STEAM_EMB_BR_BASE	"resource/sssspatcher/base/battle_STEAM_BR.emb"
#define BATTLE_STEAM_EMB_BR_OUT		"resource/ui/BR/battle_STEAM.emb"

#define BATTLE_STEAM_EMB_NS_BASE	"resource/sssspatcher/base/battle_STEAM_NS.emb"
#define BATTLE_STEAM_EMB_NS_OUT		"resource/ui/NS/battle_STEAM.emb"

#define BATTLE_STEAM_EMB_CH_BASE	"resource/sssspatcher/base/battle_STEAM_CH.emb"
#define BATTLE_STEAM_EMB_CH_OUT		"resource/ui/CH/battle_STEAM.emb"

#define AVATARS_DIRECTORY				"resource/sssspatcher/avatars/"
#define BATTLE_NAMES_DIRECTORY_LEFT		"resource/sssspatcher/battle_names_left/"
#define BATTLE_NAMES_DIRECTORY_RIGHT	"resource/sssspatcher/battle_names_right/"

#define BATTLE_STEAM_CACHE				"resource/sssspatcher/avatars/cache.bin"

#define BATTLE_STEAM_EMS_SIZE		(0x22590)
#define NUM_ORIGINAL_AVATARS		(0x95)
//#define FIRST_AVATAR_NEW_EMB_ENTRY	(0xB3)

#define BS_AVATAR_LEFT_NUM_OFFSET		(0x5100)
#define BS_AVATAR_LEFT_NUM_OFFSET2		(0x5194)
#define BS_AVATAR_LEFT_NUM_OFFSET3		(0x5196)
#define BS_AVATAR_LEFT_ID_TABLE			(0x51B0)
#define BS_AVATAR_LEFT_MAPPING			(0x5410)
#define BS_AVATAR_LEFT_ID_TABLE_FPTR	(0x51A0)
#define BS_AVATAR_LEFT_MAPPING_FPTR		(0x51A4)	

#define BS_AVATAR_RIGHT_NUM_OFFSET		(0x66D0)
#define BS_AVATAR_RIGHT_NUM_OFFSET2		(0x6764)
#define BS_AVATAR_RIGHT_NUM_OFFSET3		(0x6766)
#define BS_AVATAR_RIGHT_ID_TABLE		(0x6780)
#define BS_AVATAR_RIGHT_MAPPING			(0x69E0)
#define BS_AVATAR_RIGHT_ID_TABLE_FPTR	(0x6770)
#define BS_AVATAR_RIGHT_MAPPING_FPTR	(0x6774)	

#define NUM_ORIGINAL_BATTLE_NAMES	(0x32)

#define BS_BN_LEFT_NUM_OFFSET		(0xB650)
#define BS_BN_LEFT_NUM_OFFSET2		(0xB6E4)
#define BS_BN_LEFT_NUM_OFFSET3		(0xB6E6)
#define BS_BN_LEFT_ID_TABLE			(0xB700)
#define BS_BN_LEFT_MAPPING			(0xB7D0)
#define BS_BN_LEFT_ID_TABLE_FPTR	(0xB6F0)
#define BS_BN_LEFT_MAPPING_FPTR		(0xB6F4)

#define BS_BN_RIGHT_NUM_OFFSET		(0xBE30)
#define BS_BN_RIGHT_NUM_OFFSET2		(0xBEC4)
#define BS_BN_RIGHT_NUM_OFFSET3		(0xBEC6)
#define BS_BN_RIGHT_ID_TABLE		(0xBEE0)
#define BS_BN_RIGHT_MAPPING			(0xBFB0)
#define BS_BN_RIGHT_ID_TABLE_FPTR	(0xBED0)
#define BS_BN_RIGHT_MAPPING_FPTR	(0xBED4)

static std::vector<std::string> battle_steam_base =
{
	BATTLE_STEAM_EMB_JP_BASE,
	BATTLE_STEAM_EMB_EN_BASE,
	BATTLE_STEAM_EMB_FR_BASE,
	BATTLE_STEAM_EMB_IT_BASE,
	BATTLE_STEAM_EMB_SP_BASE,
	BATTLE_STEAM_EMB_BR_BASE,
	BATTLE_STEAM_EMB_CH_BASE,
	BATTLE_STEAM_EMB_NS_BASE
};

static std::vector<std::string> battle_steam_out =
{
	BATTLE_STEAM_EMB_JP_OUT,
	BATTLE_STEAM_EMB_EN_OUT,
	BATTLE_STEAM_EMB_FR_OUT,
	BATTLE_STEAM_EMB_IT_OUT,
	BATTLE_STEAM_EMB_SP_OUT,
	BATTLE_STEAM_EMB_BR_OUT,
	BATTLE_STEAM_EMB_CH_OUT,
	BATTLE_STEAM_EMB_NS_OUT,
};


/////////////////////

#define NUM_ORIGINAL_CN		(0x1C0)

#define TDB_CHR_NAME_BASE	"resource/sssspatcher/base/TdbChrName.tdb"
#define TDB_CHR_NAME_OUT	"resource/text/TdbChrName.tdb"

#define CHR_NAME_DIRECTORY	"resource/sssspatcher/chr_name/"
#define CHR_NAME_CACHE		"resource/sssspatcher/chr_name/cache.bin"

////////////////////

#define CHA_SEL_EMS_BASE		"resource/sssspatcher/base/cha_sel_JP.ems"
#define CHA_SEL_EMS_OUT			"resource/ui/cha_sel_JP.ems"

#define CHA_SEL_EMB_JP_BASE		"resource/sssspatcher/base/cha_sel_JP_JP.emb"
#define CHA_SEL_EMB_JP_OUT		"resource/ui/JP/cha_sel_JP.emb"

#define CHA_SEL_EMB_EN_BASE		"resource/sssspatcher/base/cha_sel_JP_EN.emb"
#define CHA_SEL_EMB_EN_OUT		"resource/ui/EN/cha_sel_JP.emb"

#define CHA_SEL_EMB_FR_BASE		"resource/sssspatcher/base/cha_sel_JP_FR.emb"
#define CHA_SEL_EMB_FR_OUT		"resource/ui/FR/cha_sel_JP.emb"

#define CHA_SEL_EMB_IT_BASE		"resource/sssspatcher/base/cha_sel_JP_IT.emb"
#define CHA_SEL_EMB_IT_OUT		"resource/ui/IT/cha_sel_JP.emb"

#define CHA_SEL_EMB_SP_BASE		"resource/sssspatcher/base/cha_sel_JP_SP.emb"
#define CHA_SEL_EMB_SP_OUT		"resource/ui/SP/cha_sel_JP.emb"

#define CHA_SEL_EMB_BR_BASE		"resource/sssspatcher/base/cha_sel_JP_BR.emb"
#define CHA_SEL_EMB_BR_OUT		"resource/ui/BR/cha_sel_JP.emb"

#define CHA_SEL_EMB_NS_BASE		"resource/sssspatcher/base/cha_sel_JP_NS.emb"
#define CHA_SEL_EMB_NS_OUT		"resource/ui/NS/cha_sel_JP.emb"

#define CHA_SEL_EMB_CH_BASE		"resource/sssspatcher/base/cha_sel_JP_CH.emb"
#define CHA_SEL_EMB_CH_OUT		"resource/ui/CH/cha_sel_JP.emb"

#define ICONS_DIRECTORY_BIG		"resource/sssspatcher/icons_big/"
#define ICONS_DIRECTORY_SMALL	"resource/sssspatcher/icons_small/"
#define SN_DIRECTORY			"resource/sssspatcher/cha_sel/select_names/"
#define SN2_DIRECTORY			"resource/sssspatcher/cha_sel/select2_names/"

#define CHA_SEL_CACHE			"resource/sssspatcher/cha_sel/cache.bin"

#define CHA_SEL_EMS_SIZE				(0x1B360)
#define CS_NUM_ORIGINAL_ICONS_BIG		(0x96)
#define CS_NUM_ORIGINAL_ICONS_SMALL		(0x95)

#define CS_ICON_BIG_NUM_OFFSET		(0x65A0)
#define CS_ICON_BIG_NUM_OFFSET2		(0x6634)
#define CS_ICON_BIG_NUM_OFFSET3		(0x6636)
#define CS_ICON_BIG_ID_TABLE		(0x6650)
#define CS_ICON_BIG_MAPPING			(0x68B0)
#define CS_ICON_BIG_ID_TABLE_FPTR	(0x6640)
#define CS_ICON_BIG_MAPPING_FPTR	(0x6644)	

#define CS_ICON_SMALL_NUM_OFFSET	(0x184A0)
#define CS_ICON_SMALL_NUM_OFFSET2	(0x184E4)
#define CS_ICON_SMALL_NUM_OFFSET3	(0x184E6)
#define CS_ICON_SMALL_ID_TABLE		(0x18500)
#define CS_ICON_SMALL_MAPPING		(0x18760)
#define CS_ICON_SMALL_ID_TABLE_FPTR	(0x184F0)
#define CS_ICON_SMALL_MAPPING_FPTR	(0x184F4)

#define NUM_ORIGINAL_SELECT_NAMES	(0x32)

#define CS_SN_NUM_OFFSET			(0xF160)
#define CS_SN_NUM_OFFSET2			(0xF1F4)
#define CS_SN_NUM_OFFSET3			(0xF1F6)
#define CS_SN_ID_TABLE				(0xF210)
#define CS_SN_MAPPING				(0xF2E0)
#define CS_SN_ID_TABLE_FPTR			(0xF200)
#define CS_SN_MAPPING_FPTR			(0xF204)

#define NUM_ORIGINAL_SELECT2_NAMES	(0x31)

#define CS_SN2_NUM_OFFSET			(0x19A20)
#define CS_SN2_NUM_OFFSET2			(0x19AB4)
#define CS_SN2_NUM_OFFSET3			(0x19AB6)
#define CS_SN2_ID_TABLE				(0x19AD0)
#define CS_SN2_MAPPING				(0x19BA0)
#define CS_SN2_ID_TABLE_FPTR		(0x19AC0)
#define CS_SN2_MAPPING_FPTR			(0x19AC4)	

static std::vector<std::string> cha_sel_base =
{
	CHA_SEL_EMB_JP_BASE,
	CHA_SEL_EMB_EN_BASE,
	CHA_SEL_EMB_FR_BASE,
	CHA_SEL_EMB_IT_BASE,
	CHA_SEL_EMB_SP_BASE,
	CHA_SEL_EMB_BR_BASE,
	CHA_SEL_EMB_CH_BASE,
	CHA_SEL_EMB_NS_BASE
};

static std::vector<std::string> cha_sel_out =
{
	CHA_SEL_EMB_JP_OUT,
	CHA_SEL_EMB_EN_OUT,
	CHA_SEL_EMB_FR_OUT,
	CHA_SEL_EMB_IT_OUT,
	CHA_SEL_EMB_SP_OUT,
	CHA_SEL_EMB_BR_OUT,
	CHA_SEL_EMB_CH_OUT,
	CHA_SEL_EMB_NS_OUT,
};

///////////////////

#define GWR_EMS_BASE		"resource/sssspatcher/base/gw_result_JP.ems"
#define GWR_EMS_OUT			"resource/ui/gw_result_JP.ems"

#define GWR_EMB_JP_BASE		"resource/sssspatcher/base/gw_result_JP_JP.emb"
#define GWR_EMB_JP_OUT		"resource/ui/JP/gw_result_JP.emb"

#define GWR_EMB_EN_BASE		"resource/sssspatcher/base/gw_result_JP_EN.emb"
#define GWR_EMB_EN_OUT		"resource/ui/EN/gw_result_JP.emb"

#define GWR_EMB_FR_BASE		"resource/sssspatcher/base/gw_result_JP_FR.emb"
#define GWR_EMB_FR_OUT		"resource/ui/FR/gw_result_JP.emb"

#define GWR_EMB_IT_BASE		"resource/sssspatcher/base/gw_result_JP_IT.emb"
#define GWR_EMB_IT_OUT		"resource/ui/IT/gw_result_JP.emb"

#define GWR_EMB_SP_BASE		"resource/sssspatcher/base/gw_result_JP_SP.emb"
#define GWR_EMB_SP_OUT		"resource/ui/SP/gw_result_JP.emb"

#define GWR_EMB_BR_BASE		"resource/sssspatcher/base/gw_result_JP_BR.emb"
#define GWR_EMB_BR_OUT		"resource/ui/BR/gw_result_JP.emb"

#define GWR_EMB_NS_BASE		"resource/sssspatcher/base/gw_result_JP_NS.emb"
#define GWR_EMB_NS_OUT		"resource/ui/NS/gw_result_JP.emb"

#define GWR_EMB_CH_BASE		"resource/sssspatcher/base/gw_result_JP_CH.emb"
#define GWR_EMB_CH_OUT		"resource/ui/CH/gw_result_JP.emb"

#define GWR_NAMES_DIRECTORY	"resource/sssspatcher/gwr/names/"

#define GWR_CACHE			"resource/sssspatcher/gwr/cache.bin"

#define GWR_EMS_SIZE					(0xDB60)
#define GWR_NUM_ORIGINAL_ICONS_BIG		(0x93)

#define GWR_ICON_BIG_NUM_OFFSET		(0x3DC0)
#define GWR_ICON_BIG_NUM_OFFSET2	(0x3E84)
#define GWR_ICON_BIG_NUM_OFFSET3	(0x3E86)
#define GWR_ICON_BIG_ID_TABLE		(0x3EA0)
#define GWR_ICON_BIG_MAPPING		(0x40F0)
#define GWR_ICON_BIG_ID_TABLE_FPTR	(0x3E90)
#define GWR_ICON_BIG_MAPPING_FPTR	(0x3E94)	

#define NUM_ORIGINAL_GWR_NAMES		(0x6D)

#define GWR_NAME_NUM_OFFSET		(0x2B10)
#define GWR_NAME_NUM_OFFSET2	(0x2BD4)
#define GWR_NAME_NUM_OFFSET3	(0x2BD6)
#define GWR_NAME_ID_TABLE		(0x2BF0)
#define GWR_NAME_MAPPING		(0x2DB0)
#define GWR_NAME_ID_TABLE_FPTR	(0x2BE0)
#define GWR_NAME_MAPPING_FPTR	(0x2BE4)	

static std::vector<std::string> gwr_base =
{
	GWR_EMB_JP_BASE,
	GWR_EMB_EN_BASE,
	GWR_EMB_FR_BASE,
	GWR_EMB_IT_BASE,
	GWR_EMB_SP_BASE,
	GWR_EMB_BR_BASE,
	GWR_EMB_CH_BASE,
	GWR_EMB_NS_BASE
};

static std::vector<std::string> gwr_out =
{
	GWR_EMB_JP_OUT,
	GWR_EMB_EN_OUT,
	GWR_EMB_FR_OUT,
	GWR_EMB_IT_OUT,
	GWR_EMB_SP_OUT,
	GWR_EMB_BR_OUT,
	GWR_EMB_CH_OUT,
	GWR_EMB_NS_OUT,
};

///////////////////

#define GWT_EMS_BASE		"resource/sssspatcher/base/gw_tour_JP.ems"
#define GWT_EMS_OUT			"resource/ui/gw_tour_JP.ems"

#define GWT_EMB_JP_BASE		"resource/sssspatcher/base/gw_tour_JP_JP.emb"
#define GWT_EMB_JP_OUT		"resource/ui/JP/gw_tour_JP.emb"

#define GWT_EMB_EN_BASE		"resource/sssspatcher/base/gw_tour_JP_EN.emb"
#define GWT_EMB_EN_OUT		"resource/ui/EN/gw_tour_JP.emb"

#define GWT_EMB_FR_BASE		"resource/sssspatcher/base/gw_tour_JP_FR.emb"
#define GWT_EMB_FR_OUT		"resource/ui/FR/gw_tour_JP.emb"

#define GWT_EMB_IT_BASE		"resource/sssspatcher/base/gw_tour_JP_IT.emb"
#define GWT_EMB_IT_OUT		"resource/ui/IT/gw_tour_JP.emb"

#define GWT_EMB_SP_BASE		"resource/sssspatcher/base/gw_tour_JP_SP.emb"
#define GWT_EMB_SP_OUT		"resource/ui/SP/gw_tour_JP.emb"

#define GWT_EMB_BR_BASE		"resource/sssspatcher/base/gw_tour_JP_BR.emb"
#define GWT_EMB_BR_OUT		"resource/ui/BR/gw_tour_JP.emb"

#define GWT_EMB_NS_BASE		"resource/sssspatcher/base/gw_tour_JP_NS.emb"
#define GWT_EMB_NS_OUT		"resource/ui/NS/gw_tour_JP.emb"

#define GWT_EMB_CH_BASE		"resource/sssspatcher/base/gw_tour_JP_CH.emb"
#define GWT_EMB_CH_OUT		"resource/ui/CH/gw_tour_JP.emb"

#define GWT_BANNERS_DIRECTORY			"resource/sssspatcher/gwt/banners/"		
#define GWT_BANNER_NAMES_DIRECTORY		"resource/sssspatcher/gwt/banner_names/"

#define GWT_CACHE					"resource/sssspatcher/gwt/cache.bin"

#define GWT_EMS_SIZE				(0x12990)
#define NUM_ORIGINAL_BANNERS		(0x93)

#define GWT_BANNER_NUM_OFFSET		(0x6E10)
#define GWT_BANNER_NUM_OFFSET2		(0x6EA4)
#define GWT_BANNER_NUM_OFFSET3		(0x6EA6)
#define GWT_BANNER_ID_TABLE			(0x6EC0)
#define GWT_BANNER_MAPPING			(0x7110)
#define GWT_BANNER_ID_TABLE_FPTR	(0x6EB0)
#define GWT_BANNER_MAPPING_FPTR		(0x6EB4)	

#define NUM_ORIGINAL_BANNER_NAMES	(0x31)

#define GWT_BANNER_NAME_NUM_OFFSET		(0x9050)
#define GWT_BANNER_NAME_NUM_OFFSET2		(0x90E4)
#define GWT_BANNER_NAME_NUM_OFFSET3		(0x90E6)
#define GWT_BANNER_NAME_ID_TABLE		(0x9100)
#define GWT_BANNER_NAME_MAPPING			(0x91D0)
#define GWT_BANNER_NAME_ID_TABLE_FPTR	(0x90F0)
#define GWT_BANNER_NAME_MAPPING_FPTR	(0x90F4)	

static std::vector<std::string> gwt_base =
{
	GWT_EMB_JP_BASE,
	GWT_EMB_EN_BASE,
	GWT_EMB_FR_BASE,
	GWT_EMB_IT_BASE,
	GWT_EMB_SP_BASE,
	GWT_EMB_BR_BASE,
	GWT_EMB_CH_BASE,
	GWT_EMB_NS_BASE
};

static std::vector<std::string> gwt_out =
{
	GWT_EMB_JP_OUT,
	GWT_EMB_EN_OUT,
	GWT_EMB_FR_OUT,
	GWT_EMB_IT_OUT,
	GWT_EMB_SP_OUT,
	GWT_EMB_BR_OUT,
	GWT_EMB_CH_OUT,
	GWT_EMB_NS_OUT,
};


///////////////////

#define VS_EMS_BASE		"resource/sssspatcher/base/vs_JP.ems"
#define VS_EMS_OUT			"resource/ui/vs_JP.ems"

#define VS_EMB_JP_BASE		"resource/sssspatcher/base/vs_JP_JP.emb"
#define VS_EMB_JP_OUT		"resource/ui/JP/vs_JP.emb"

#define VS_EMB_EN_BASE		"resource/sssspatcher/base/vs_JP_EN.emb"
#define VS_EMB_EN_OUT		"resource/ui/EN/vs_JP.emb"

#define VS_EMB_FR_BASE		"resource/sssspatcher/base/vs_JP_FR.emb"
#define VS_EMB_FR_OUT		"resource/ui/FR/vs_JP.emb"

#define VS_EMB_IT_BASE		"resource/sssspatcher/base/vs_JP_IT.emb"
#define VS_EMB_IT_OUT		"resource/ui/IT/vs_JP.emb"

#define VS_EMB_SP_BASE		"resource/sssspatcher/base/vs_JP_SP.emb"
#define VS_EMB_SP_OUT		"resource/ui/SP/vs_JP.emb"

#define VS_EMB_BR_BASE		"resource/sssspatcher/base/vs_JP_BR.emb"
#define VS_EMB_BR_OUT		"resource/ui/BR/vs_JP.emb"

#define VS_EMB_NS_BASE		"resource/sssspatcher/base/vs_JP_NS.emb"
#define VS_EMB_NS_OUT		"resource/ui/NS/vs_JP.emb"

#define VS_EMB_CH_BASE		"resource/sssspatcher/base/vs_JP_CH.emb"
#define VS_EMB_CH_OUT		"resource/ui/CH/vs_JP.emb"

#define VS_CACHE			"resource/sssspatcher/vs/cache.bin"

#define VS_EMS_SIZE				(0xB9D0)
// It is one less than select_name, we have to add a dummy entry!
#define NUM_ORIGINAL_VS_NAMES	(0x31) 

#define VS_NAME_NUM_OFFSET		(0x2C30)
#define VS_NAME_NUM_OFFSET2		(0x2CC4)
#define VS_NAME_NUM_OFFSET3		(0x2CC6)
#define VS_NAME_ID_TABLE		(0x2CE0)
#define VS_NAME_MAPPING			(0x2DB0)
#define VS_NAME_ID_TABLE_FPTR	(0x2CD0)
#define VS_NAME_MAPPING_FPTR	(0x2CD4)	


static std::vector<std::string> vs_base =
{
	VS_EMB_JP_BASE,
	VS_EMB_EN_BASE,
	VS_EMB_FR_BASE,
	VS_EMB_IT_BASE,
	VS_EMB_SP_BASE,
	VS_EMB_BR_BASE,
	VS_EMB_CH_BASE,
	VS_EMB_NS_BASE
};

static std::vector<std::string> vs_out =
{
	VS_EMB_JP_OUT,
	VS_EMB_EN_OUT,
	VS_EMB_FR_OUT,
	VS_EMB_IT_OUT,
	VS_EMB_SP_OUT,
	VS_EMB_BR_OUT,
	VS_EMB_CH_OUT,
	VS_EMB_NS_OUT,
};

///////////////////

#define NUM_CPK			2

typedef struct 
{
	CDCEntry *cdc_entry; // 0
	CMSEntry *cms_entry; // 4
	CMSModelSpec *model_spec; // 8
	uint16_t id; // 0xC
	uint16_t pad; // 0x10
} __attribute__((packed)) ModelStruct;

static CpkDef cpk_defs[NUM_CPK] = 
{
	{ .hdr_buf = NULL, .toc_buf = NULL, .toc_offset = 0, .toc_size = 0, .patched = false, .name = "localize.cpk" },
	{ .hdr_buf = NULL, .toc_buf = NULL, .toc_offset = 0, .toc_size = 0, .patched = false, .name = "resource.cpk" }
};

#elif defined(XENOVERSE)
	
#define PROCESS_NAME 	"dbxv.exe"

#define NUM_CPK			5

static CpkDef cpk_defs[NUM_CPK] = 
{
	{ .hdr_buf = NULL, .toc_buf = NULL, .toc_offset = 0, .toc_size = 0, .patched = false, .name = "data.cpk" },
	{ .hdr_buf = NULL, .toc_buf = NULL, .toc_offset = 0, .toc_size = 0, .patched = false, .name = "data2.cpk" },
	{ .hdr_buf = NULL, .toc_buf = NULL, .toc_offset = 0, .toc_size = 0, .patched = false, .name = "datap1.cpk" },
	{ .hdr_buf = NULL, .toc_buf = NULL, .toc_offset = 0, .toc_size = 0, .patched = false, .name = "datap2.cpk" },
	{ .hdr_buf = NULL, .toc_buf = NULL, .toc_offset = 0, .toc_size = 0, .patched = false, .name = "datap3.cpk" },
	/*{ .hdr_buf = NULL, .toc_buf = NULL, .toc_offset = 0, .toc_size = 0, .patched = false, .name = "movie.cpk" },
	{ .hdr_buf = NULL, .toc_buf = NULL, .toc_offset = 0, .toc_size = 0, .patched = false, .name = "movie2.cpk" },
	{ .hdr_buf = NULL, .toc_buf = NULL, .toc_offset = 0, .toc_size = 0, .patched = false, .name = "movie_eu.cpk" },
	{ .hdr_buf = NULL, .toc_buf = NULL, .toc_offset = 0, .toc_size = 0, .patched = false, .name = "movie_us.cpk" }*/
};


#endif /* GAME */

#endif

