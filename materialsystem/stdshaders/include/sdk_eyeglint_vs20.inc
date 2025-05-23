// ALL SKIP STATEMENTS THAT AFFECT THIS SHADER!!!
// defined $LIGHTING_PREVIEW && defined $FASTPATH && $LIGHTING_PREVIEW && $FASTPATH

#ifndef SDK_EYEGLINT_VS20_H
#define SDK_EYEGLINT_VS20_H

#include "shaderapi/ishaderapi.h"
#include "shaderapi/ishadershadow.h"
#include "materialsystem/imaterialvar.h"

class sdk_eyeglint_vs20_Static_Index
{
public:
	sdk_eyeglint_vs20_Static_Index( IShaderShadow* pShaderShadow, IMaterialVar** params )
	{
	}

	int GetIndex() const
	{
		return 0;
	}
};

#define shaderStaticTest_sdk_eyeglint_vs20 1


class sdk_eyeglint_vs20_Dynamic_Index
{
public:
	sdk_eyeglint_vs20_Dynamic_Index( IShaderDynamicAPI* pShaderAPI )
	{
	}

	int GetIndex() const
	{
		return 0;
	}
};

#define shaderDynamicTest_sdk_eyeglint_vs20 1


#endif	// SDK_EYEGLINT_VS20_H