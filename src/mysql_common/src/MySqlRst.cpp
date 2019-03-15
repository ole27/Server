#include "MySqlRst.h"


#include <string.h>
static const char* INDEX_ERROR = "Index Error!";


CMySqlRst::CMySqlRst(MYSQL_RES* res, uint32 FieldCount, uint32 RowCount) : m_nFieldCount(FieldCount), m_nRowCount(RowCount), m_pResult( res )
{
	if( res == NULL || FieldCount == 0 )
	{
		m_pCurrentRow	= NULL;
		m_pFields		= NULL;
		m_nRowCount		= 0;
		//m_fieldName.clear();
	}else{
		m_pCurrentRow	= new Field[FieldCount];
		m_pFields		= mysql_fetch_fields(res);
		/*for ( uint32 iField = 0; iField < m_nFieldCount; iField ++ )
		{
			m_fieldName[ m_pFields[iField].name] = iField;
		}*/
	}
}

CMySqlRst::~CMySqlRst()
{
	//m_fieldName.clear();
	if(m_pResult) mysql_free_result(m_pResult);
	delete [] m_pCurrentRow;
}

bool CMySqlRst::NextRow()
{
	if( m_nRowCount <= 0 )
		return false;

	MYSQL_ROW row = mysql_fetch_row(m_pResult);
	if(row == NULL)
		return false;

	for(uint32 i = 0; i < m_nFieldCount; ++i)
		m_pCurrentRow[i].SetValue(row[i]);

	return true;
}

bool CMySqlRst::GetIsNumber( int type )
{
	return IS_NUM( type );
}

bool CMySqlRst::GetIsString( int type )
{
	return IS_LONGDATA( type );

}


short CMySqlRst::GetFieldType( uint32 iCol )
{
	int32 index = static_cast<int32>( iCol );
	if( index < 0 || static_cast<uint32>( index ) >= m_nFieldCount )
	{
		return -1;
	}
	return m_pFields[ index ].type;
}

const char* CMySqlRst::GetFieldName(uint32 iCol)
{
	int32 index = static_cast<int32>(iCol);
	if ( index < 0 || index >= m_nFieldCount)
	{
		return INDEX_ERROR;
	}
	return m_pFields[ index ].name;
}

/*
int32 CMySqlRst::GetFieldIndex(const std::string strFieldName)
{
	ServerMap< std::string, uint32>::iterator iter = m_fieldName.find(strFieldName);
	if ( iter != m_fieldName.end() )
		return iter->second;
	return -1;
}

const Field& CMySqlRst::operator[](const std::string strFieldName) const
{
	assert(m_pCurrentRow);	
	ServerMap< std::string, uint32>::const_iterator iter = m_fieldName.find( strFieldName);
	assert( iter != m_fieldName.end() );
	return m_pCurrentRow[iter->second];
}*/

const Field& CMySqlRst::operator[](uint32 iCol) const
{
	assert(m_pCurrentRow&& iCol<= m_nFieldCount);
	return m_pCurrentRow[iCol];
}

