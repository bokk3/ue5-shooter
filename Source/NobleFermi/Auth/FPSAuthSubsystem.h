// =========================================================================================
// NOBLE FERMI // CLOUDFLARE D1 OPERATOR AUTHENTICATION & PROFILE SYNC SUBSYSTEM
// Pure C++20 / Slate Compatible GameInstance Subsystem
// Resilient Cloudflare Edge D1 link with seamless offline cryptographic fallback
// =========================================================================================

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/IHttpRequest.h"
#include "FPSAuthSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnAuthStateChangedSignature, bool, bIsAuthenticated, const FString&, Callsign, bool, bIsOfflineMode);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAuthErrorSignature, const FString&, ErrorMessage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnProfileSyncCompletedSignature, bool, bSuccess);

USTRUCT(BlueprintType)
struct FFPSSavedSession
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPS Auth")
	FString Token;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPS Auth")
	FString UserId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPS Auth")
	FString Callsign;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPS Auth")
	FString Email;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPS Auth")
	bool bRememberMe = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPS Auth")
	FString CachedPasswordHashForOffline;
};

/**
 * AAA Full-Stack Authentication & Cloud Profile Subsystem
 * Connects Unreal Engine 5.8 client with Cloudflare D1 SQL via https://noble-fermi.pages.dev/api.
 * Provides 0ms instant optimistic auto-login on startup, background profile hydration,
 * and resilient local cryptographic fallback when offline.
 */
UCLASS()
class NOBLEFERMI_API UFPSAuthSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// === MULTICAST DELEGATES ===
	UPROPERTY(BlueprintAssignable, Category = "FPS Auth")
	FOnAuthStateChangedSignature OnAuthStateChanged;

	UPROPERTY(BlueprintAssignable, Category = "FPS Auth")
	FOnAuthErrorSignature OnAuthError;

	UPROPERTY(BlueprintAssignable, Category = "FPS Auth")
	FOnProfileSyncCompletedSignature OnProfileSyncCompleted;

	// === STATE GETTERS ===
	UFUNCTION(BlueprintPure, Category = "FPS Auth")
	bool IsAuthenticated() const { return bIsAuthenticated; }

	UFUNCTION(BlueprintPure, Category = "FPS Auth")
	bool IsOfflineMode() const { return bIsOfflineMode; }

	UFUNCTION(BlueprintPure, Category = "FPS Auth")
	bool IsPendingRequest() const { return bIsPendingRequest; }

	UFUNCTION(BlueprintPure, Category = "FPS Auth")
	const FString& GetActiveCallsign() const { return CurrentCallsign; }

	UFUNCTION(BlueprintPure, Category = "FPS Auth")
	const FString& GetActiveEmail() const { return CurrentEmail; }

	UFUNCTION(BlueprintPure, Category = "FPS Auth")
	const FString& GetActiveUserId() const { return CurrentUserId; }

	UFUNCTION(BlueprintPure, Category = "FPS Auth")
	const FString& GetAuthToken() const { return CurrentToken; }

	UFUNCTION(BlueprintPure, Category = "FPS Auth")
	const FString& GetApiBaseUrl() const { return ApiBaseUrl; }

	UFUNCTION(BlueprintCallable, Category = "FPS Auth")
	void SetApiBaseUrl(const FString& NewUrl) { ApiBaseUrl = NewUrl; }

	// === CORE AUTHENTICATION OPERATIONS ===
	UFUNCTION(BlueprintCallable, Category = "FPS Auth")
	void TryAutoLoginOnStartup();

	UFUNCTION(BlueprintCallable, Category = "FPS Auth")
	void Login(const FString& Identifier, const FString& Password, bool bRememberMe = true);

	UFUNCTION(BlueprintCallable, Category = "FPS Auth")
	void Register(const FString& Callsign, const FString& Email, const FString& Password, bool bRememberMe = true);

	UFUNCTION(BlueprintCallable, Category = "FPS Auth")
	void LoginAsGuest();

	UFUNCTION(BlueprintCallable, Category = "FPS Auth")
	void Logout();

	// === CLOUD PROFILE & ECONOMY BRIDGING ===
	UFUNCTION(BlueprintCallable, Category = "FPS Auth")
	void SyncProfileToCloud();

	void ApplyCloudProfileToEconomy(const TSharedPtr<FJsonObject>& ProfileJson);

	void RefreshLocalPlayerName();

private:
	bool bIsAuthenticated = false;
	bool bIsOfflineMode = false;
	bool bIsPendingRequest = false;

	FString CurrentToken;
	FString CurrentUserId;
	FString CurrentCallsign;
	FString CurrentEmail;
	bool bCurrentRememberMe = true;

	FString ApiBaseUrl = TEXT("https://noble-fermi.pages.dev/api");

	// Session persistence
	FString GetAuthSessionFilePath() const;
	FString GetLocalAccountsFilePath() const;

	void SaveSession(const FString& Token, const FString& UserId, const FString& Callsign, const FString& Email, bool bRememberMe, const FString& CachedHash = TEXT(""));
	bool LoadSavedSession(FFPSSavedSession& OutSession);
	void ClearSavedSession();

	// Offline local cryptographic fallback
	bool TryLocalOfflineLogin(const FString& Identifier, const FString& Password);
	void SaveLocalAccount(const FString& Callsign, const FString& Email, const FString& Password);
	FString ComputeSaltedHash(const FString& Plaintext, const FString& Salt) const;

	// Internal HTTP callbacks
	void OnLoginResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully, FString Identifier, FString Password, bool bRememberMe);
	void OnRegisterResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully, FString Callsign, FString Email, FString Password, bool bRememberMe);
	void OnMeResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully);
	void OnSyncResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully);
};
