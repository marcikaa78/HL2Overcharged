#include "shaderlib/cshader.h"
class ssao_gauss_blur_3x3_ps30_Static_Index
{
public:
	ssao_gauss_blur_3x3_ps30_Static_Index( )
	{
	}
	int GetIndex()
	{
		// Asserts to make sure that we aren't using any skipped combinations.
		// Asserts to make sure that we are setting all of the combination vars.
#ifdef _DEBUG
#endif // _DEBUG
		return 0;
	}
};
#define shaderStaticTest_ssao_gauss_blur_3x3_ps30 0
class ssao_gauss_blur_3x3_ps30_Dynamic_Index
{
public:
	ssao_gauss_blur_3x3_ps30_Dynamic_Index()
	{
	}
	int GetIndex()
	{
		// Asserts to make sure that we aren't using any skipped combinations.
		// Asserts to make sure that we are setting all of the combination vars.
#ifdef _DEBUG
#endif // _DEBUG
		return 0;
	}
};
#define shaderDynamicTest_ssao_gauss_blur_3x3_ps30 0
