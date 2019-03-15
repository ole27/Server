

#ifndef __STRUCT_DEF_H__
#define __STRUCT_DEF_H__


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Data Format
/*

float				'f'
char				'c'
ignore				'x'

int32				'i'
uint32				'u'

uint64/time_t		't'

int64				'I'
uint64				'U'

short				'h'

string              's'
string and escape   'S'
blob                'b'
blob and escape     'B'
*/

#include <stdarg.h>

#include <string>

#include "def/TypeDef.h"

struct ExtraDataStruct
{
public:
	ExtraDataStruct():is_modify(false),is_online(false)
	{
	}

public:
	bool		IsModify() { return is_modify;}
	void		SetModify() { is_modify = true;}
	void		ResetModify() { is_modify = false;}
	void        SetOnline() { is_online = true;}
	void        ResetOnline() { is_online = false;}
	bool        IsOnline()  { return is_online;}
private:
	bool        is_modify;
	bool        is_online;
} ;

struct BaseStruct
{
public:
	typedef uint64 IdType ;

public:
	inline static uint64 CalculateKey( uint16 partA, uint16 partB, uint32 partC )
	{
		uint64 retValue =0 ;

		uint16 *pInt16 =reinterpret_cast< uint16* >( &retValue ) ;

		*pInt16 =partA ;
		++pInt16 ;
		
		*pInt16 =partB ;
		++pInt16 ;

		uint32 *pInt32 = reinterpret_cast< uint32* >( pInt16 ) ;
		*pInt32 =partC ;

		return retValue ;
	}

} ;



#define INIT_MEMBER_DATA \
	uint32 offset =0;\
	char* structpointer = (char*)this;\
	const char* pFormat = GetFormat();\
	for(uint32 i = 0; i< strlen(pFormat); i++ )\
		{\
		switch( pFormat[i])\
			{\
			case 'k':\
			case 'u':\
				*(uint32*)&structpointer[offset] = 0;\
				offset += sizeof(uint32);\
				break;\
			case 'i':\
				*(int32*)&structpointer[offset] = 0;\
				offset += sizeof(int32);\
				break;\
			case 'f':\
				*(float*)&structpointer[offset] = 0.0f;\
				offset += sizeof(float);\
				break;\
			case 'b':\
			case 'B':\
			case 's':\
			case 'S':\
				*(std::string*)&structpointer[offset] = "";\
				offset += sizeof(std::string);\
				break;\
			case 'c':\
				*(uint8*)&structpointer[offset] = 0;\
				offset += sizeof(uint8);\
				break;\
			case 'h':\
				*(uint16*)&structpointer[offset] = 0;\
				offset += sizeof(uint16);\
				break;\
			case 'I':\
				*(int64*)&structpointer[offset] = 0;\
				offset += sizeof(int64);\
				break;\
			case 'U':\
				*(uint64*)&structpointer[offset] = 0;\
				offset += sizeof(uint64);\
				break;\
			case 't':\
				*(int64*)&structpointer[offset] = 0;\
				offset += sizeof(int64);\
				break;\
			}\
		}



#define DECLARE_EXTRA_SIZE_BY_MEMBER(class_name,member_name) \
	static uint32 ExtraSize()\
	{\
	class_name* p = NULL;\
	uint32 offset = (uint32)((uint64)(&p->member_name));\
	return sizeof(class_name) -offset;\
	}



#define DECLARE_EXTRA_SIZE_ZERO \
	static uint32 ExtraSize()\
	{\
	return 0;\
	}



#define INIT_GAMEDATA(classname) \
	classname()\
	{\
	INIT_MEMBER_DATA; \
	};\
	DECLARE_EXTRA_SIZE_ZERO


#define INIT_GAMEDATA_AND_EXTRA( classname, extra_member ) \
	classname() \
	{ \
	INIT_MEMBER_DATA; \
	} ; \
	DECLARE_EXTRA_SIZE_BY_MEMBER( classname, extra_member )



#define TABLE_TYPEDEF( StructName ) \
	typedef CMysqlTableCache<StructName> StructName##Cache ;\
	typedef StructName##Cache::MapItr StructName##CacheMItr ;\
	typedef StructName##Cache::VecItr StructName##CacheVItr ;



#endif


