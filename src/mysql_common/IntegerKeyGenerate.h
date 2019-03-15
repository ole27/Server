
#ifndef INTEFER_KEY_GENERATE_H__
#define INTEFER_KEY_GENERATE_H__

#include <string>

class IntegerKeyGenerate
{
public:
	size_t operator()(const char *key) const
	{
		size_t res=0;

		while( *key ) res=(res<<1)^*key++ ; //use int value of characters

		return res;
	}
} ;


#endif
