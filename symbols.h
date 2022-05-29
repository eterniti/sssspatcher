#ifndef __SYMBOLS_H___
#define __SYMBOLS_H___

#if defined(SSSS)

#define MY_DUMP_BASE	(0x1390000)

// *** Functions/methods

#define DPRINTF1_SYMBOL				(0x18AEBD3-MY_DUMP_BASE)
#define DPRINTF2_SYMBOL				(0x18AEBBB-MY_DUMP_BASE)

#define CPK_OPEN_FILE_SYMBOL		(0x192A6B8-MY_DUMP_BASE)
#define CPK_FILE_EXISTS_SYMBOL		(0x176B4E0-MY_DUMP_BASE)

#define XINPUT_GETSTATE_SYMBOL		(0x20BB770-MY_DUMP_BASE)

#define IS_CHARACTER_UNLOCKED2		(0x16B3000-MY_DUMP_BASE)

// Variables

#define SLOTS_DATA_SYMBOL						(0x2A6E920-MY_DUMP_BASE)
#define SLOTS_DATA_SIZE							(0x4B*0xBC)
#define SLOTS_DATA_COPY_SIZE					((0x4A*0xBC) + 0xA0)

#define CHARACTERS_INFO_SYMBOL					(0x2A72118-MY_DUMP_BASE)
#define CHARACTERS_INFO_ORIGINAL_SIZE			(0x93*0x18)	

#define CHARACTERS_LISTS_SYMBOL					(0x2A62380-MY_DUMP_BASE)
#define CHARACTERS_LISTS_ORIGINAL_SIZE			(0xC60)
#define CHARACTERS_LISTS_ORIGINAL_SIZE_NO_EMPTY	(0xC30)

#define CHARACTERS_LISTS2_SYMBOL				(0x2159B78-MY_DUMP_BASE)
#define CHARACTERS_LISTS2_SIZE					(0x480)

#define GLOBAL_CHARACTERS_LIST_SYMBOL			(0x2A632D8-MY_DUMP_BASE)
#define GLOBAL_CHARACTERS_LIST_ORIGINAL_SIZE	(0x4B8)

#define CHARACTERS_INFO2_SYMBOL					(0x2A66F48-MY_DUMP_BASE)
#define CHARACTERS_INFO2_ORIGINAL_SIZE			(0xCA8)

// aka "important struct"
#define MODELS_DATA_SYMBOL						(0x2E6AAA0-MY_DUMP_BASE)
#define MODELS_DATA_ORIGINAL_SIZE				(0x9A0)

#define CHARACTERS_INFO3_SYMBOL					(0x2A76D80-MY_DUMP_BASE)
#define CHARACTERS_INFO3_ORIGINAL_SIZE			(0x93*0x18)

#define CHARACTERS_INFO4_SYMBOL					(0x2A77B78-MY_DUMP_BASE)
#define CHARACTERS_INFO4_ORIGINAL_SIZE			(0x94*0x18)

#define GALAXIAN_WARS_LIST_SYMBOL				(0x213B8A8-MY_DUMP_BASE)
#define GALAXIAN_WARS_LIST_AFTER_SYMBOL			(0x213C0F8-MY_DUMP_BASE)
#define GALAXIAN_WARS_LIST_ORIGINAL_SIZE		(0x85*0x10)

// *** Pointers to variables in data

#define PTR_CHARACTERS_LISTS_SYMBOL		(0x2A62FE0-MY_DUMP_BASE)

// *** Pointers to variables in code

/* Pointer in a cmp instruction */
#define CPTR_CHARACTERS_INFO_SYMBOL		(0x1501B49-MY_DUMP_BASE)
/* Pointer in a mov instruction */
#define CPTR_CHARACTERS_INFO_SYMBOL2	(0x1501B55-MY_DUMP_BASE)
/* Pointer in a lea instruction */
#define CPTR_CHARACTERS_INFO_SYMBOL3	(0x1501B76-MY_DUMP_BASE)
/* Pointer in a mov instruction */
#define CPTR_CHARACTERS_INFO_SYMBOL4	(0x1504655-MY_DUMP_BASE)
/* Pointer in a mov instruction */
#define CPTR_CHARACTERS_INFO_SYMBOL5	(0x1504687-MY_DUMP_BASE)
/* Pointer in a mov instruction */
#define CPTR_CHARACTERS_INFO_SYMBOL6	(0x1506046-MY_DUMP_BASE)
/* Pointer in a mov instruction */
#define CPTR_CHARACTERS_INFO_SYMBOL7	(0x1506069-MY_DUMP_BASE)
/* Pointer in a cmp instruction */
#define CPTR_CHARACTERS_INFO_P4_SYMBOL	(0x1504676-MY_DUMP_BASE)
/* Pointer in a cmp instruction */
#define CPTR_CHARACTERS_INFO_P4_SYMBOL2	(0x1506058-MY_DUMP_BASE)
/* Pointer in a mov instruction */
#define CPTR_CHARACTERS_INFO_P8_SYMBOL	(0x1506080-MY_DUMP_BASE)
/* Pointer in a mov instruction */
#define CPTR_CHARACTERS_INFO_PC_SYMBOL	(0x1506088-MY_DUMP_BASE)
/* Pointer in a mov instruction */
#define CPTR_CHARACTERS_INFO_P10_SYMBOL	(0x150469F-MY_DUMP_BASE)

/* Pointers in mov instructions */
#define CPTR_BRONZE_SAINT_LIST_SYMBOL		(0x176A98D-MY_DUMP_BASE)
#define CPTR_BRONZE_SAINT_LIST_SYMBOL2		(0x176AB83-MY_DUMP_BASE)
#define CPTR_BRONZE_SAINT_LIST_SYMBOL3		(0x176AFEA-MY_DUMP_BASE)
#define CPTR_SILVER_SAINT_LIST_SYMBOL		(0x176A999-MY_DUMP_BASE)
#define CPTR_SILVER_SAINT_LIST_SYMBOL2		(0x176AB97-MY_DUMP_BASE)
#define CPTR_SILVER_SAINT_LIST_SYMBOL3		(0x176AFF3-MY_DUMP_BASE)
#define CPTR_GOLD_SAINT_LIST_SYMBOL			(0x176A9A5-MY_DUMP_BASE)
#define CPTR_GOLD_SAINT_LIST_SYMBOL2		(0x176ABAB-MY_DUMP_BASE)
#define CPTR_GOLD_SAINT_LIST_SYMBOL3		(0x176AFFC-MY_DUMP_BASE)
#define CPTR_GOLD_GOD_SAINT_LIST_SYMBOL		(0x176A9B1-MY_DUMP_BASE)
#define CPTR_GOLD_GOD_SAINT_LIST_SYMBOL2	(0x176ABBF-MY_DUMP_BASE)
#define CPTR_GOLD_GOD_SAINT_LIST_SYMBOL3	(0x176B005-MY_DUMP_BASE)
#define CPTR_ASGARD_WARRIOR_LIST_SYMBOL		(0x176A9BD-MY_DUMP_BASE)
#define CPTR_ASGARD_WARRIOR_LIST_SYMBOL2	(0x176ABD3-MY_DUMP_BASE)
#define CPTR_ASGARD_WARRIOR_LIST_SYMBOL3	(0x176B00E -MY_DUMP_BASE)
#define CPTR_POSEIDON_GENERAL_LIST_SYMBOL	(0x176A9C9-MY_DUMP_BASE)
#define CPTR_POSEIDON_GENERAL_LIST_SYMBOL2	(0x176ABE7-MY_DUMP_BASE)
#define CPTR_POSEIDON_GENERAL_LIST_SYMBOL3	(0x176B017-MY_DUMP_BASE)
#define CPTR_HADES_SPECTRE_LIST_SYMBOL		(0x176A9D5-MY_DUMP_BASE)
#define CPTR_HADES_SPECTRE_LIST_SYMBOL2		(0x176ABFB-MY_DUMP_BASE)
#define CPTR_HADES_SPECTRE_LIST_SYMBOL3		(0x176B020-MY_DUMP_BASE)
#define CPTR_GOD_LIST_SYMBOL				(0x176A9DC-MY_DUMP_BASE)
#define CPTR_GOD_LIST_SYMBOL2				(0x176AC0F-MY_DUMP_BASE)
#define CPTR_GOD_LIST_SYMBOL3				(0x176B027-MY_DUMP_BASE)
#define CPTR_BRONZE_GOD_ARMOR_LIST_SYMBOL	(0x176A9E3-MY_DUMP_BASE)
#define CPTR_BRONZE_GOD_ARMOR_LIST_SYMBOL2	(0x176AC23-MY_DUMP_BASE)
#define CPTR_BRONZE_GOD_ARMOR_LIST_SYMBOL3	(0x176B02E -MY_DUMP_BASE)

/* Pointer in mov instructions 32 bits */
#define CPTR_BRONZE_SAINT_LIST_SIZE_SYMBOL				(0x176A988-MY_DUMP_BASE)
#define CPTR_SILVER_SAINT_LIST_SIZE_SYMBOL				(0x176A994-MY_DUMP_BASE)
#define CPTR_GOLD_SAINT_LIST_SIZE_SYMBOL				(0x176A9A0-MY_DUMP_BASE)
#define CPTR_GOLD_GOD_SAINT_LIST_SIZE_SYMBOL			(0x176A9AC-MY_DUMP_BASE)
#define CPTR_ASGARD_WARRIOR_LIST_SIZE_SYMBOL			(0x176A9B8-MY_DUMP_BASE)
#define CPTR_POSEIDON_GENERAL_LIST_SIZE_SYMBOL			(0x176A9C4-MY_DUMP_BASE)
#define CPTR_HADES_SPECTRE_LIST_SIZE_SYMBOL				(0x176A9D0-MY_DUMP_BASE)
#define CPTR_GOD_AND_BRONZE_GOD_ARMOR_LIST_SIZE_SYMBOL	(0x176A9E8-MY_DUMP_BASE)

/* Pointer in mov instructions 32 bits */
#define CPTR_BRONZE_SAINT_LIST_SIZE_SYMBOL2				(0x176AB7E -MY_DUMP_BASE)
#define CPTR_SILVER_SAINT_LIST_SIZE_SYMBOL2				(0x176AB92-MY_DUMP_BASE)
#define CPTR_GOLD_SAINT_LIST_SIZE_SYMBOL2				(0x176ABA6-MY_DUMP_BASE)
#define CPTR_GOLD_GOD_SAINT_LIST_SIZE_SYMBOL2			(0x176ABBA-MY_DUMP_BASE)
#define CPTR_ASGARD_WARRIOR_LIST_SIZE_SYMBOL2			(0x176ABCE -MY_DUMP_BASE)
#define CPTR_POSEIDON_GENERAL_LIST_SIZE_SYMBOL2			(0x176ABE2-MY_DUMP_BASE)
#define CPTR_HADES_SPECTRE_LIST_SIZE_SYMBOL2			(0x176ABF6-MY_DUMP_BASE)
#define CPTR_GOD_LIST_SIZE_SYMBOL						(0x176AC0A-MY_DUMP_BASE)
#define CPTR_BRONZE_GOD_ARMOR_LIST_SIZE_SYMBOL			(0x176AC1E -MY_DUMP_BASE)

/* Pointer in push instructions 8 bits */
#define CPTR_BRONZE_SAINT_LIST_SIZE_SYMBOL3				(0x176AFE8-MY_DUMP_BASE)
#define CPTR_SILVER_SAINT_LIST_SIZE_SYMBOL3				(0x176AFF1-MY_DUMP_BASE)
#define CPTR_GOLD_SAINT_LIST_SIZE_SYMBOL3				(0x176AFFA-MY_DUMP_BASE)
#define CPTR_GOLD_GOD_SAINT_LIST_SIZE_SYMBOL3			(0x176B003-MY_DUMP_BASE)
#define CPTR_ASGARD_WARRIOR_LIST_SIZE_SYMBOL3			(0x176B00C-MY_DUMP_BASE)
#define CPTR_POSEIDON_GENERAL_LIST_SIZE_SYMBOL3			(0x176B015-MY_DUMP_BASE)
#define CPTR_HADES_SPECTRE_LIST_SIZE_SYMBOL3			(0x176B01E -MY_DUMP_BASE)
#define CPTR_GOD_AND_BRONZE_GOD_ARMOR_LIST_SIZE_SYMBOL2	(0x176B033-MY_DUMP_BASE)

/* Pointers in a mov instruction */
#define CPTR_GLOBAL_CHARACTERS_LIST_SYMBOL				 (0x14B2335-MY_DUMP_BASE)
#define CPTR_GLOBAL_CHARACTERS_LIST_SYMBOL2				 (0x14B2366-MY_DUMP_BASE)
/* Pointer in a cmp instruction */
#define CPTR_GLOBAL_CHARACTERS_LIST_P4_SYMBOL			 (0x14B2356-MY_DUMP_BASE)

/* Pointers in mov instructions */
#define CPTR_CHARACTERS_INFO2_SYMBOL		(0x14E626D-MY_DUMP_BASE)
#define CPTR_CHARACTERS_INFO2_SYMBOL2		(0x14E629F-MY_DUMP_BASE)
#define CPTR_CHARACTERS_INFO2_SYMBOL3		(0x14E62AC-MY_DUMP_BASE)
#define CPTR_CHARACTERS_INFO2_SYMBOL4		(0x14E62E2-MY_DUMP_BASE)

/* Pointers in cmp instructions */
#define CPTR_CHARACTERS_INFO2_P4_SYMBOL		(0x14E6280-MY_DUMP_BASE)
#define CPTR_CHARACTERS_INFO2_P4_SYMBOL2	(0x14E62CA-MY_DUMP_BASE)

/* Pointers in mov instructions */
#define CPTR_CHARACTERS_INFO2_P8_SYMBOL		(0x14E62F7-MY_DUMP_BASE)
#define CPTR_CHARACTERS_INFO2_P8_SYMBOL2	(0x14E630E -MY_DUMP_BASE)
#define CPTR_CHARACTERS_INFO2_PC_SYMBOL		(0x14E62FE -MY_DUMP_BASE)
#define CPTR_CHARACTERS_INFO2_PC_SYMBOL2	(0x14E6315-MY_DUMP_BASE)

/* Pointers in cmp instructions */
#define CPTR_CHARACTERS_INFO2_P10_SYMBOL	(0x14E6290-MY_DUMP_BASE)
#define CPTR_CHARACTERS_INFO2_P10_SYMBOL2	(0x14E62D2-MY_DUMP_BASE)

/* Pointer in add instruction */
#define CPTR_MODELS_DATA_SYMBOL				(0x14944A7-MY_DUMP_BASE)
#define CPTR_MODELS_DATA_SYMBOL6			(0x1494846-MY_DUMP_BASE)
/* Pointers in mov intructions */
#define CPTR_MODELS_DATA_SYMBOL2			(0x14944C1-MY_DUMP_BASE)
#define CPTR_MODELS_DATA_SYMBOL3			(0x1494566-MY_DUMP_BASE)
#define CPTR_MODELS_DATA_SYMBOL4			(0x1494758-MY_DUMP_BASE)
#define CPTR_MODELS_DATA_SYMBOL5			(0x149487A-MY_DUMP_BASE)
/* Pointer in movsx instuction */
#define CPTR_MODELS_DATA_PC_SYMBOL			(0x1494537-MY_DUMP_BASE)

/* Pointer in cmp instructions */
#define CPTR_MODELS_DATA_AFTER_SYMBOL		(0x1494776-MY_DUMP_BASE)
#define CPTR_NUM_MODELS_SYMBOL				(0x1494455-MY_DUMP_BASE)
#define CPTR_MODELS_DATA_SIZE_SYMBOL		(0x149489F-MY_DUMP_BASE)

/* Pointers in mov instructions */
#define CPTR_CHARACTERS_INFO3_SYMBOL		(0x1554CF1-MY_DUMP_BASE)
#define CPTR_CHARACTERS_INFO3_SYMBOL2		(0x1555A4F-MY_DUMP_BASE)
#define CPTR_CHARACTERS_INFO3_SYMBOL3		(0x15568DF-MY_DUMP_BASE)

#define CPTR_CHARACTERS_INFO3_P4_SYMBOL		(0x1554D0B-MY_DUMP_BASE)
#define CPTR_CHARACTERS_INFO3_P4_SYMBOL2	(0x1555A67-MY_DUMP_BASE)
#define CPTR_CHARACTERS_INFO3_P4_SYMBOL3	(0x15568F7-MY_DUMP_BASE)

/* Pointers in fld instructions */
#define CPTR_CHARACTERS_INFO3_P8_SYMBOL		(0x1554D43-MY_DUMP_BASE)
#define CPTR_CHARACTERS_INFO3_PC_SYMBOL		(0x1554D22-MY_DUMP_BASE)

/* Pointers in mov instructions */
#define CPTR_CHARACTERS_INFO3_P10_SYMBOL	(0x155692C-MY_DUMP_BASE)
#define CPTR_CHARACTERS_INFO3_P14_SYMBOL	(0x1555A72-MY_DUMP_BASE)

/* Pointers in cmp instructions */
#define CPTR_CHARACTERS_INFO3_SIZE_SYMBOL	(0x1554D5B-MY_DUMP_BASE)
#define CPTR_CHARACTERS_INFO3_SIZE_SYMBOL2	(0x1555A8F-MY_DUMP_BASE)
#define CPTR_CHARACTERS_INFO3_SIZE_SYMBOL3  (0x1556908-MY_DUMP_BASE)

/* Pointers in cmp instructions */
#define CPTR_RANDOM_SLOT_UPPER_RIGHT	(0x1501B32-MY_DUMP_BASE)
#define CPTR_RANDOM_SLOT_LOWER_LEFT		(0x1501B28-MY_DUMP_BASE)
#define CPTR_RANDOM_SLOT_LOWER_RIGHT	(0x1501B2D-MY_DUMP_BASE)

/* Pointers in cmp instructions */
#define CPTR_CHARACTERS_INFO4_SYMBOL		(0x1559A91-MY_DUMP_BASE)
#define CPTR_CHARACTERS_INFO4_SYMBOL2		(0x155A154-MY_DUMP_BASE)

/* Pointers in mov instructions */
#define CPTR_CHARACTERS_INFO4_SYMBOL3		(0x155A16A-MY_DUMP_BASE)
#define CPTR_CHARACTERS_INFO4_SYMBOL4		(0x155A191-MY_DUMP_BASE)

/* Pointers in cmp instructions */
#define CPTR_CHARACTERS_INFO4_P4_SYMBOL		(0x1559AA8-MY_DUMP_BASE)
#define CPTR_CHARACTERS_INFO4_P4_SYMBOL2	(0x155A180-MY_DUMP_BASE)

/* Pointers in fld instructions */
#define CPTR_CHARACTERS_INFO4_P8_SYMBOL		(0x155A1A8-MY_DUMP_BASE)
#define CPTR_CHARACTERS_INFO4_PC_SYMBOL		(0x155A1B3-MY_DUMP_BASE)

/* Pointers in mov instructions */
#define CPTR_CHARACTERS_INFO4_P10_SYMBOL	(0x1559AD3-MY_DUMP_BASE)
#define CPTR_CHARACTERS_INFO4_P14_SYMBOL	(0x1559ACB-MY_DUMP_BASE)

/* Pointer in cmp instruction */
#define CPTR_CHARACTERS_INFO4_SIZE_SYMBOL 	(0x1559AB6-MY_DUMP_BASE)

/* Pointer in mov instruction */
#define CPTR_GALAXIAN_WARS_LIST_SYMBOL		(0x166A6D5-MY_DUMP_BASE)
#define CPTR_GALAXIAN_WARS_LIST_P4_SYMBOL	(0x1668559-MY_DUMP_BASE)
#define CPTR_GALAXIAN_WARS_LIST_P8_SYMBOL	(0x1668581-MY_DUMP_BASE)

/* Pointer in cmp instruction */
#define CPTR_GALAXIAN_WARS_LIST_AFTER_SYMBOL	(0x166A747-MY_DUMP_BASE)
#define CPTR_GALAXIAN_WARS_LIST_AFTER_P4_SYMBOL	(0x1668574-MY_DUMP_BASE)


#elif defined(XENOVERSE)
	
#define MY_DUMP_BASE	(0xB60000)

#define DPRINTF1_SYMBOL				(0xFD81E8-MY_DUMP_BASE)
#define DPRINTF2_SYMBOL				(0xFD81D0-MY_DUMP_BASE)

#define CPK_OPEN_FILE_SYMBOL		(0xFE4E36-MY_DUMP_BASE)

#endif

#endif

