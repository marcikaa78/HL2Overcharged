#include "shaderlib/cshader.h"

class deferred_lightning_vs30_Static_Index
{
public:
	deferred_lightning_vs30_Static_Index( )
	{
	}

	int GetIndex() const
	{
		// Asserts to make sure that we aren't using any skipped combinations.
		// Asserts to make sure that we are setting all of the combination vars.
#ifdef _DEBUG
#endif // _DEBUG
		return 0;
	}
};

#define shaderStaticTest_deferred_lightning_vs30 0


class deferred_lightning_vs30_Dynamic_Index
{
public:
	deferred_lightning_vs30_Dynamic_Index( )
	{
	}

	int GetIndex() const
	{
		// Asserts to make sure that we aren't using any skipped combinations.
		// Asserts to make sure that we are setting all of the combination vars.
#ifdef _DEBUG
#endif // _DEBUG
		return 0;
	}
};

#define shaderDynamicTest_deferred_lightning_vs30 0
