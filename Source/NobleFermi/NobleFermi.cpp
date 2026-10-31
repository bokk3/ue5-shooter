#include "NobleFermi.h"
#include "Modules/ModuleManager.h"
#include "Lyra/AbilitySystem/FPSGameplayTags.h"

class FNobleFermiModule : public FDefaultGameModuleImpl
{
public:
	virtual void StartupModule() override
	{
		FDefaultGameModuleImpl::StartupModule();
		FFPSGameplayTags::InitializeNativeGameplayTags();
	}

	virtual void ShutdownModule() override
	{
		FDefaultGameModuleImpl::ShutdownModule();
	}
};

IMPLEMENT_PRIMARY_GAME_MODULE(FNobleFermiModule, NobleFermi, "NobleFermi");
