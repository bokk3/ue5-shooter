// =========================================================================================
// NOBLE FERMI // CLOUDFLARE D1 OPERATOR AUTHENTICATION & PROFILE SYNC SUBSYSTEM
// Pure C++20 / Slate Compatible GameInstance Subsystem
// Resilient Cloudflare Edge D1 link with seamless offline cryptographic fallback
// =========================================================================================

#include "Auth/FPSAuthSubsystem.h"
#include "Economy/FPSEconomySubsystem.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "Misc/CommandLine.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/SecureHash.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Dom/JsonObject.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "HAL/FileManager.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"

void UFPSAuthSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// 1. Configurable API Endpoint via command line or config file
	FString CmdUrl;
	if (FParse::Value(FCommandLine::Get(), TEXT("AuthUrl="), CmdUrl) && !CmdUrl.IsEmpty())
	{
		ApiBaseUrl = CmdUrl;
	}
	else
	{
		FString ConfigUrl;
		if (GConfig && GConfig->GetString(TEXT("NobleFermi.Auth"), TEXT("ApiBaseUrl"), ConfigUrl, GGameIni) && !ConfigUrl.IsEmpty())
		{
			ApiBaseUrl = ConfigUrl;
		}
	}

	if (ApiBaseUrl.EndsWith(TEXT("/")))
	{
		ApiBaseUrl = ApiBaseUrl.LeftChop(1);
	}

	UE_LOG(LogTemp, Log, TEXT("[FPSAuthSubsystem] Initialized. Target Edge API: %s"), *ApiBaseUrl);

	// 2. Perform zero-latency optimistic auto-login on startup
	TryAutoLoginOnStartup();
}

void UFPSAuthSubsystem::Deinitialize()
{
	Super::Deinitialize();
}

FString UFPSAuthSubsystem::GetAuthSessionFilePath() const
{
	return FPaths::ProjectSavedDir() / TEXT("NobleFermiAuth.json");
}

FString UFPSAuthSubsystem::GetLocalAccountsFilePath() const
{
	return FPaths::ProjectSavedDir() / TEXT("NobleFermiLocalAccounts.json");
}

void UFPSAuthSubsystem::SaveSession(const FString& Token, const FString& UserId, const FString& Callsign, const FString& Email, bool bRememberMe, const FString& CachedHash)
{
	CurrentToken = Token;
	CurrentUserId = UserId;
	CurrentCallsign = Callsign;
	CurrentEmail = Email;
	bCurrentRememberMe = bRememberMe;

	if (!bRememberMe)
	{
		ClearSavedSession();
		return;
	}

	TSharedPtr<FJsonObject> JsonObj = MakeShared<FJsonObject>();
	JsonObj->SetStringField(TEXT("Token"), Token);
	JsonObj->SetStringField(TEXT("UserId"), UserId);
	JsonObj->SetStringField(TEXT("Callsign"), Callsign);
	JsonObj->SetStringField(TEXT("Email"), Email);
	JsonObj->SetBoolField(TEXT("bRememberMe"), bRememberMe);
	if (!CachedHash.IsEmpty())
	{
		JsonObj->SetStringField(TEXT("CachedPasswordHashForOffline"), CachedHash);
	}

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	if (FJsonSerializer::Serialize(JsonObj.ToSharedRef(), Writer))
	{
		FFileHelper::SaveStringToFile(OutputString, *GetAuthSessionFilePath(), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
	}
}

bool UFPSAuthSubsystem::LoadSavedSession(FFPSSavedSession& OutSession)
{
	const FString FilePath = GetAuthSessionFilePath();
	FString JsonIn;
	if (!FFileHelper::LoadFileToString(JsonIn, *FilePath) || JsonIn.IsEmpty())
	{
		return false;
	}

	TSharedPtr<FJsonObject> JsonObj;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonIn);
	if (!FJsonSerializer::Deserialize(Reader, JsonObj) || !JsonObj.IsValid())
	{
		return false;
	}

	OutSession.Token = JsonObj->GetStringField(TEXT("Token"));
	OutSession.UserId = JsonObj->GetStringField(TEXT("UserId"));
	OutSession.Callsign = JsonObj->GetStringField(TEXT("Callsign"));
	OutSession.Email = JsonObj->GetStringField(TEXT("Email"));
	OutSession.bRememberMe = JsonObj->GetBoolField(TEXT("bRememberMe"));
	OutSession.CachedPasswordHashForOffline = JsonObj->GetStringField(TEXT("CachedPasswordHashForOffline"));

	return !OutSession.Token.IsEmpty() && !OutSession.Callsign.IsEmpty();
}

void UFPSAuthSubsystem::ClearSavedSession()
{
	IFileManager::Get().Delete(*GetAuthSessionFilePath(), false, true, true);
}

FString UFPSAuthSubsystem::ComputeSaltedHash(const FString& Plaintext, const FString& Salt) const
{
	const FString Combined = Salt + TEXT("::NF_SALT::") + Plaintext;
	return FSHA1::HashBuffer(TCHAR_TO_ANSI(*Combined), Combined.Len()).ToString();
}

void UFPSAuthSubsystem::SaveLocalAccount(const FString& Callsign, const FString& Email, const FString& Password)
{
	const FString FilePath = GetLocalAccountsFilePath();
	FString JsonIn;
	TSharedPtr<FJsonObject> RootObj = MakeShared<FJsonObject>();

	if (FFileHelper::LoadFileToString(JsonIn, *FilePath) && !JsonIn.IsEmpty())
	{
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonIn);
		FJsonSerializer::Deserialize(Reader, RootObj);
	}

	if (!RootObj.IsValid())
	{
		RootObj = MakeShared<FJsonObject>();
	}

	const FString Salt = FGuid::NewGuid().ToString();
	const FString Hash = ComputeSaltedHash(Password, Salt);

	TSharedPtr<FJsonObject> AccountObj = MakeShared<FJsonObject>();
	AccountObj->SetStringField(TEXT("Callsign"), Callsign);
	AccountObj->SetStringField(TEXT("Email"), Email);
	AccountObj->SetStringField(TEXT("Salt"), Salt);
	AccountObj->SetStringField(TEXT("PasswordHash"), Hash);

	RootObj->SetObjectField(Callsign.ToLower(), AccountObj);
	if (!Email.IsEmpty())
	{
		RootObj->SetObjectField(Email.ToLower(), AccountObj);
	}

	FString JsonOut;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonOut);
	if (FJsonSerializer::Serialize(RootObj.ToSharedRef(), Writer))
	{
		FFileHelper::SaveStringToFile(JsonOut, *FilePath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
	}
}

bool UFPSAuthSubsystem::TryLocalOfflineLogin(const FString& Identifier, const FString& Password)
{
	const FString FilePath = GetLocalAccountsFilePath();
	FString JsonIn;
	if (!FFileHelper::LoadFileToString(JsonIn, *FilePath) || JsonIn.IsEmpty())
	{
		return false;
	}

	TSharedPtr<FJsonObject> RootObj;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonIn);
	if (!FJsonSerializer::Deserialize(Reader, RootObj) || !RootObj.IsValid())
	{
		return false;
	}

	const FString Key = Identifier.ToLower().TrimStartAndEnd();
	if (!RootObj->HasField(Key))
	{
		return false;
	}

	TSharedPtr<FJsonObject> AccountObj = RootObj->GetObjectField(Key);
	if (!AccountObj.IsValid())
	{
		return false;
	}

	const FString Salt = AccountObj->GetStringField(TEXT("Salt"));
	const FString ExpectedHash = AccountObj->GetStringField(TEXT("PasswordHash"));
	const FString Computed = ComputeSaltedHash(Password, Salt);

	if (Computed.Equals(ExpectedHash, ESearchCase::CaseSensitive))
	{
		CurrentCallsign = AccountObj->GetStringField(TEXT("Callsign"));
		CurrentEmail = AccountObj->GetStringField(TEXT("Email"));
		CurrentUserId = TEXT("local_offline_") + CurrentCallsign;
		CurrentToken = TEXT("offline_token_") + FGuid::NewGuid().ToString();
		bIsAuthenticated = true;
		bIsOfflineMode = true;

		RefreshLocalPlayerName();
		OnAuthStateChanged.Broadcast(true, CurrentCallsign, true);
		return true;
	}

	return false;
}

void UFPSAuthSubsystem::TryAutoLoginOnStartup()
{
	FFPSSavedSession SavedSession;
	if (LoadSavedSession(SavedSession))
	{
		// 0ms Optimistic UI activation: Player is immediately authenticated locally!
		bIsAuthenticated = true;
		bIsOfflineMode = false;
		CurrentToken = SavedSession.Token;
		CurrentUserId = SavedSession.UserId;
		CurrentCallsign = SavedSession.Callsign;
		CurrentEmail = SavedSession.Email;
		bCurrentRememberMe = SavedSession.bRememberMe;

		RefreshLocalPlayerName();
		OnAuthStateChanged.Broadcast(true, CurrentCallsign, false);

		UE_LOG(LogTemp, Log, TEXT("[FPSAuthSubsystem] Optimistic session loaded for '%s'. Hydrating from D1 in background..."), *CurrentCallsign);

		// Hydrate latest profile state in background from Cloudflare D1
		TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
		Request->SetURL(ApiBaseUrl + TEXT("/auth/me"));
		Request->SetVerb(TEXT("GET"));
		Request->SetHeader(TEXT("User-Agent"), TEXT("Mozilla/5.0 (Windows NT 10.0; Win64; x64) NobleFermiClient/5.8"));
		Request->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("Bearer %s"), *CurrentToken));
		Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
		Request->OnProcessRequestComplete().BindUObject(this, &UFPSAuthSubsystem::OnMeResponseReceived);
		Request->ProcessRequest();
	}
	else
	{
		bIsAuthenticated = false;
		bIsOfflineMode = false;
		CurrentToken.Empty();
		CurrentCallsign.Empty();
	}
}

void UFPSAuthSubsystem::Login(const FString& Identifier, const FString& Password, bool bRememberMe)
{
	const FString TrimmedIdentifier = Identifier.TrimStartAndEnd();

	// Special path: 6-Digit Website Verification Code submission ("VERIFY_CODE::<Identifier>")
	if (TrimmedIdentifier.StartsWith(TEXT("VERIFY_CODE::")))
	{
		const FString TargetAccount = TrimmedIdentifier.Mid(13).TrimStartAndEnd();
		const FString VerificationCode = Password.TrimStartAndEnd();
		if (VerificationCode.IsEmpty())
		{
			OnAuthError.Broadcast(TEXT("Please enter the 6-digit verification code from the website."));
			return;
		}

		bIsPendingRequest = true;

		TSharedPtr<FJsonObject> VerifyJson = MakeShared<FJsonObject>();
		VerifyJson->SetStringField(TEXT("identifier"), TargetAccount.IsEmpty() ? CurrentCallsign : TargetAccount);
		VerifyJson->SetStringField(TEXT("code"), VerificationCode);

		FString VerifyBody;
		TSharedRef<TJsonWriter<>> VerifyWriter = TJsonWriterFactory<>::Create(&VerifyBody);
		FJsonSerializer::Serialize(VerifyJson.ToSharedRef(), VerifyWriter);

		TSharedRef<IHttpRequest, ESPMode::ThreadSafe> VerifyReq = FHttpModule::Get().CreateRequest();
		VerifyReq->SetURL(ApiBaseUrl + TEXT("/auth/verify"));
		VerifyReq->SetVerb(TEXT("POST"));
		VerifyReq->SetHeader(TEXT("User-Agent"), TEXT("Mozilla/5.0 (Windows NT 10.0; Win64; x64) NobleFermiClient/5.8"));
		if (!CurrentToken.IsEmpty())
		{
			VerifyReq->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("Bearer %s"), *CurrentToken));
		}
		VerifyReq->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
		VerifyReq->SetContentAsString(VerifyBody);
		VerifyReq->OnProcessRequestComplete().BindUObject(this, &UFPSAuthSubsystem::OnLoginResponseReceived, TargetAccount, VerificationCode, bRememberMe);
		VerifyReq->ProcessRequest();
		return;
	}

	if (TrimmedIdentifier.IsEmpty() || Password.IsEmpty())
	{
		OnAuthError.Broadcast(TEXT("Callsign/Email and passkey are required."));
		return;
	}

	bIsPendingRequest = true;

	TSharedPtr<FJsonObject> JsonObj = MakeShared<FJsonObject>();
	JsonObj->SetStringField(TEXT("identifier"), TrimmedIdentifier);
	JsonObj->SetStringField(TEXT("password"), Password);

	FString RequestBody;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&RequestBody);
	FJsonSerializer::Serialize(JsonObj.ToSharedRef(), Writer);

	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
	Request->SetURL(ApiBaseUrl + TEXT("/auth/login"));
	Request->SetVerb(TEXT("POST"));
	Request->SetHeader(TEXT("User-Agent"), TEXT("Mozilla/5.0 (Windows NT 10.0; Win64; x64) NobleFermiClient/5.8"));
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	Request->SetContentAsString(RequestBody);

	Request->OnProcessRequestComplete().BindUObject(this, &UFPSAuthSubsystem::OnLoginResponseReceived, TrimmedIdentifier, Password, bRememberMe);
	Request->ProcessRequest();
}

void UFPSAuthSubsystem::OnLoginResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully, FString Identifier, FString Password, bool bRememberMe)
{
	bIsPendingRequest = false;

	if (!bConnectedSuccessfully || !Response.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("[FPSAuthSubsystem] Cloudflare D1 login request failed to connect. Falling back to local auth..."));
		if (TryLocalOfflineLogin(Identifier, Password))
		{
			return;
		}
		OnAuthError.Broadcast(TEXT("Cloud edge unreachable and no matching local account found."));
		return;
	}

	const int32 ResponseCode = Response->GetResponseCode();
	const FString ResponseBody = Response->GetContentAsString();

	TSharedPtr<FJsonObject> JsonObj;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseBody);
	if (!FJsonSerializer::Deserialize(Reader, JsonObj) || !JsonObj.IsValid())
	{
		if (TryLocalOfflineLogin(Identifier, Password))
		{
			return;
		}
		OnAuthError.Broadcast(FString::Printf(TEXT("Invalid server response (HTTP %d)."), ResponseCode));
		return;
	}

	if (ResponseCode == 200 && JsonObj->GetBoolField(TEXT("ok")))
	{
		if (JsonObj->HasField(TEXT("token")))
		{
			CurrentToken = JsonObj->GetStringField(TEXT("token"));
		}
		TSharedPtr<FJsonObject> UserObj = JsonObj->GetObjectField(TEXT("user"));
		bool bIsVerified = true;
		if (UserObj.IsValid())
		{
			CurrentUserId = UserObj->GetStringField(TEXT("id"));
			CurrentCallsign = UserObj->GetStringField(TEXT("callsign"));
			CurrentEmail = UserObj->GetStringField(TEXT("email"));
			if (UserObj->HasField(TEXT("is_verified")))
			{
				bIsVerified = UserObj->GetBoolField(TEXT("is_verified"));
			}
		}
		if (JsonObj->HasField(TEXT("requires_verification")) && JsonObj->GetBoolField(TEXT("requires_verification")))
		{
			bIsVerified = false;
		}

		TSharedPtr<FJsonObject> ProfileObj = JsonObj->GetObjectField(TEXT("profile"));
		if (ProfileObj.IsValid())
		{
			ApplyCloudProfileToEconomy(ProfileObj);
		}

		// If the account has not yet been verified via the 6-digit website code, prompt for verification first!
		if (!bIsVerified)
		{
			bIsAuthenticated = false;
			bIsOfflineMode = false;
			ClearSavedSession();
			UE_LOG(LogTemp, Log, TEXT("[FPSAuthSubsystem] Account '%s' requires 6-digit website verification code."), *CurrentCallsign);
			OnAuthError.Broadcast(FString::Printf(TEXT("VERIFY_REQUIRED::%s::%s"), *CurrentCallsign, *CurrentToken));
			return;
		}

		bIsAuthenticated = true;
		bIsOfflineMode = false;
		bCurrentRememberMe = bRememberMe;

		SaveSession(CurrentToken, CurrentUserId, CurrentCallsign, CurrentEmail, bRememberMe);
		SaveLocalAccount(CurrentCallsign, CurrentEmail, Password);

		RefreshLocalPlayerName();
		OnAuthStateChanged.Broadcast(true, CurrentCallsign, false);
		UE_LOG(LogTemp, Log, TEXT("[FPSAuthSubsystem] Operator '%s' verified and authenticated via Cloudflare D1."), *CurrentCallsign);
	}
	else
	{
		const FString ErrorMsg = JsonObj->HasField(TEXT("error")) ? JsonObj->GetStringField(TEXT("error")) : TEXT("Authentication failed.");
		OnAuthError.Broadcast(ErrorMsg);
	}
}

void UFPSAuthSubsystem::Register(const FString& Callsign, const FString& Email, const FString& Password, bool bRememberMe)
{
	const FString TrimmedCallsign = Callsign.TrimStartAndEnd();
	const FString TrimmedEmail = Email.TrimStartAndEnd().ToLower();

	if (TrimmedCallsign.Len() < 3 || TrimmedCallsign.Len() > 20)
	{
		OnAuthError.Broadcast(TEXT("Callsign must be 3-20 characters."));
		return;
	}

	if (Password.Len() < 6)
	{
		OnAuthError.Broadcast(TEXT("Passkey must be at least 6 characters."));
		return;
	}

	bIsPendingRequest = true;

	TSharedPtr<FJsonObject> JsonObj = MakeShared<FJsonObject>();
	JsonObj->SetStringField(TEXT("callsign"), TrimmedCallsign);
	JsonObj->SetStringField(TEXT("email"), TrimmedEmail);
	JsonObj->SetStringField(TEXT("password"), Password);

	FString RequestBody;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&RequestBody);
	FJsonSerializer::Serialize(JsonObj.ToSharedRef(), Writer);

	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
	Request->SetURL(ApiBaseUrl + TEXT("/auth/register"));
	Request->SetVerb(TEXT("POST"));
	Request->SetHeader(TEXT("User-Agent"), TEXT("Mozilla/5.0 (Windows NT 10.0; Win64; x64) NobleFermiClient/5.8"));
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	Request->SetContentAsString(RequestBody);

	Request->OnProcessRequestComplete().BindUObject(this, &UFPSAuthSubsystem::OnRegisterResponseReceived, TrimmedCallsign, TrimmedEmail, Password, bRememberMe);
	Request->ProcessRequest();
}

void UFPSAuthSubsystem::OnRegisterResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully, FString Callsign, FString Email, FString Password, bool bRememberMe)
{
	bIsPendingRequest = false;

	if (!bConnectedSuccessfully || !Response.IsValid())
	{
		OnAuthError.Broadcast(TEXT("Could not reach https://noble-fermi.pages.dev to register online account. Check internet connection."));
		return;
	}

	const int32 ResponseCode = Response->GetResponseCode();
	const FString ResponseBody = Response->GetContentAsString();

	TSharedPtr<FJsonObject> JsonObj;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseBody);
	if (!FJsonSerializer::Deserialize(Reader, JsonObj) || !JsonObj.IsValid())
	{
		OnAuthError.Broadcast(FString::Printf(TEXT("Malformed registration response (HTTP %d)."), ResponseCode));
		return;
	}

	if ((ResponseCode == 200 || ResponseCode == 201) && JsonObj->GetBoolField(TEXT("ok")))
	{
		CurrentToken = JsonObj->GetStringField(TEXT("token"));
		TSharedPtr<FJsonObject> UserObj = JsonObj->GetObjectField(TEXT("user"));
		if (UserObj.IsValid())
		{
			CurrentUserId = UserObj->GetStringField(TEXT("id"));
			CurrentCallsign = UserObj->GetStringField(TEXT("callsign"));
			CurrentEmail = UserObj->GetStringField(TEXT("email"));
		}

		SaveLocalAccount(CurrentCallsign, CurrentEmail, Password);

		TSharedPtr<FJsonObject> ProfileObj = JsonObj->GetObjectField(TEXT("profile"));
		if (ProfileObj.IsValid())
		{
			ApplyCloudProfileToEconomy(ProfileObj);
		}

		// Require website 6-digit verification code before completing login!
		bIsAuthenticated = false;
		bIsOfflineMode = false;
		ClearSavedSession();

		UE_LOG(LogTemp, Log, TEXT("[FPSAuthSubsystem] Operator '%s' created on Cloudflare D1. Awaiting 6-digit website verification code."), *CurrentCallsign);
		OnAuthError.Broadcast(FString::Printf(TEXT("VERIFY_REQUIRED::%s::%s"), *CurrentCallsign, *CurrentToken));
	}
	else
	{
		const FString ErrorMsg = JsonObj->HasField(TEXT("error")) ? JsonObj->GetStringField(TEXT("error")) : TEXT("Enlistment failed.");
		OnAuthError.Broadcast(ErrorMsg);
	}
}

void UFPSAuthSubsystem::LoginAsGuest()
{
	const int32 GuestNum = FMath::RandRange(100, 999);
	CurrentCallsign = FString::Printf(TEXT("OPERATOR_%d"), GuestNum);
	CurrentEmail = FString::Printf(TEXT("guest_%d@noblefermi.local"), GuestNum);
	CurrentUserId = TEXT("guest_") + FGuid::NewGuid().ToString();
	CurrentToken = TEXT("guest_token_") + FGuid::NewGuid().ToString();

	bIsAuthenticated = true;
	bIsOfflineMode = true;
	bCurrentRememberMe = false;

	RefreshLocalPlayerName();
	OnAuthStateChanged.Broadcast(true, CurrentCallsign, true);
}

void UFPSAuthSubsystem::Logout()
{
	bIsAuthenticated = false;
	bIsOfflineMode = false;
	bIsPendingRequest = false;

	CurrentToken.Empty();
	CurrentUserId.Empty();
	CurrentCallsign.Empty();
	CurrentEmail.Empty();

	ClearSavedSession();
	OnAuthStateChanged.Broadcast(false, TEXT(""), false);
}

void UFPSAuthSubsystem::OnMeResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully)
{
	if (!bConnectedSuccessfully || !Response.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("[FPSAuthSubsystem] Background profile hydration failed to reach Cloudflare Edge. Preserving offline active session."));
		bIsOfflineMode = true;
		OnAuthStateChanged.Broadcast(true, CurrentCallsign, true);
		return;
	}

	const int32 ResponseCode = Response->GetResponseCode();
	if (ResponseCode == 401)
	{
		UE_LOG(LogTemp, Warning, TEXT("[FPSAuthSubsystem] Saved session token expired or invalid (HTTP 401). Clearing session."));
		Logout();
		return;
	}

	if (ResponseCode == 200)
	{
		TSharedPtr<FJsonObject> JsonObj;
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());
		if (FJsonSerializer::Deserialize(Reader, JsonObj) && JsonObj.IsValid() && JsonObj->GetBoolField(TEXT("ok")))
		{
			bool bIsVerified = true;
			TSharedPtr<FJsonObject> UserObj = JsonObj->GetObjectField(TEXT("user"));
			if (UserObj.IsValid())
			{
				CurrentCallsign = UserObj->GetStringField(TEXT("callsign"));
				CurrentEmail = UserObj->GetStringField(TEXT("email"));
				if (UserObj->HasField(TEXT("is_verified")))
				{
					bIsVerified = UserObj->GetBoolField(TEXT("is_verified"));
				}
			}
			if (JsonObj->HasField(TEXT("requires_verification")) && JsonObj->GetBoolField(TEXT("requires_verification")))
			{
				bIsVerified = false;
			}

			TSharedPtr<FJsonObject> ProfileObj = JsonObj->GetObjectField(TEXT("profile"));
			if (ProfileObj.IsValid())
			{
				ApplyCloudProfileToEconomy(ProfileObj);
			}

			if (!bIsVerified)
			{
				bIsAuthenticated = false;
				bIsOfflineMode = false;
				ClearSavedSession();
				OnAuthStateChanged.Broadcast(false, TEXT(""), false);
				OnAuthError.Broadcast(FString::Printf(TEXT("VERIFY_REQUIRED::%s::%s"), *CurrentCallsign, *CurrentToken));
				return;
			}

			bIsOfflineMode = false;
			RefreshLocalPlayerName();
			OnAuthStateChanged.Broadcast(true, CurrentCallsign, false);
		}
	}
}

void UFPSAuthSubsystem::SyncProfileToCloud()
{
	if (!bIsAuthenticated || bIsOfflineMode || CurrentToken.IsEmpty())
	{
		return;
	}

	UGameInstance* GI = GetGameInstance();
	if (!GI) return;

	UFPSEconomySubsystem* Economy = GI->GetSubsystem<UFPSEconomySubsystem>();
	if (!Economy) return;

	int32 CamoIdx = 0, HeadgearIdx = 0, ArmorIdx = 0, WeaponFinishIdx = 0;
	Economy->GetEquippedLoadout(CamoIdx, HeadgearIdx, ArmorIdx, WeaponFinishIdx);

	TSharedPtr<FJsonObject> SyncObj = MakeShared<FJsonObject>();
	SyncObj->SetNumberField(TEXT("gold_balance"), Economy->GetGoldBalance());
	SyncObj->SetNumberField(TEXT("opal_balance"), Economy->GetOpalBalance());
	SyncObj->SetNumberField(TEXT("login_streak"), Economy->GetConsecutiveLoginDays());

	// Unlocked items array
	TArray<TSharedPtr<FJsonValue>> UnlockedArray;
	for (const FString& ItemID : Economy->GetUnlockedItemIDs())
	{
		UnlockedArray.Add(MakeShared<FJsonValueString>(ItemID));
	}
	SyncObj->SetArrayField(TEXT("unlocked_items"), UnlockedArray);

	// Equipped loadout
	TSharedPtr<FJsonObject> LoadoutObj = MakeShared<FJsonObject>();
	LoadoutObj->SetNumberField(TEXT("camo"), CamoIdx);
	LoadoutObj->SetNumberField(TEXT("headgear"), HeadgearIdx);
	LoadoutObj->SetNumberField(TEXT("armor"), ArmorIdx);
	LoadoutObj->SetNumberField(TEXT("weaponFinish"), WeaponFinishIdx);
	SyncObj->SetObjectField(TEXT("equipped_loadout"), LoadoutObj);

	FString RequestBody;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&RequestBody);
	FJsonSerializer::Serialize(SyncObj.ToSharedRef(), Writer);

	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
	Request->SetURL(ApiBaseUrl + TEXT("/profile/sync"));
	Request->SetVerb(TEXT("POST"));
	Request->SetHeader(TEXT("User-Agent"), TEXT("Mozilla/5.0 (Windows NT 10.0; Win64; x64) NobleFermiClient/5.8"));
	Request->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("Bearer %s"), *CurrentToken));
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	Request->SetContentAsString(RequestBody);
	Request->OnProcessRequestComplete().BindUObject(this, &UFPSAuthSubsystem::OnSyncResponseReceived);
	Request->ProcessRequest();
}

void UFPSAuthSubsystem::OnSyncResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully)
{
	const bool bSuccess = bConnectedSuccessfully && Response.IsValid() && Response->GetResponseCode() == 200;
	if (bSuccess)
	{
		TSharedPtr<FJsonObject> JsonObj;
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());
		if (FJsonSerializer::Deserialize(Reader, JsonObj) && JsonObj.IsValid() && JsonObj->GetBoolField(TEXT("ok")))
		{
			TSharedPtr<FJsonObject> ProfileObj = JsonObj->GetObjectField(TEXT("profile"));
			if (ProfileObj.IsValid())
			{
				ApplyCloudProfileToEconomy(ProfileObj);
			}
		}
	}
	OnProfileSyncCompleted.Broadcast(bSuccess);
}

void UFPSAuthSubsystem::ApplyCloudProfileToEconomy(const TSharedPtr<FJsonObject>& ProfileJson)
{
	if (!ProfileJson.IsValid()) return;

	UGameInstance* GI = GetGameInstance();
	if (!GI) return;

	UFPSEconomySubsystem* Economy = GI->GetSubsystem<UFPSEconomySubsystem>();
	if (!Economy) return;

	const int32 CloudGold = ProfileJson->GetIntegerField(TEXT("gold_balance"));
	const int32 CloudOpals = ProfileJson->GetIntegerField(TEXT("opal_balance"));
	Economy->SetBalances(CloudGold, CloudOpals);

	// Unlocked items
	if (ProfileJson->HasField(TEXT("unlocked_items_json")))
	{
		const FString UnlockedStr = ProfileJson->GetStringField(TEXT("unlocked_items_json"));
		TSharedPtr<FJsonValue> ParsedVal;
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(UnlockedStr);
		if (FJsonSerializer::Deserialize(Reader, ParsedVal) && ParsedVal.IsValid())
		{
			const TArray<TSharedPtr<FJsonValue>>* ArrPtr = nullptr;
			if (ParsedVal->TryGetArray(ArrPtr) && ArrPtr)
			{
				for (const auto& Val : *ArrPtr)
				{
					Economy->UnlockItemDirectly(Val->AsString());
				}
			}
		}
	}

	// Equipped loadout
	if (ProfileJson->HasField(TEXT("equipped_loadout_json")))
	{
		const FString LoadoutStr = ProfileJson->GetStringField(TEXT("equipped_loadout_json"));
		TSharedPtr<FJsonObject> LoadoutObj;
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(LoadoutStr);
		if (FJsonSerializer::Deserialize(Reader, LoadoutObj) && LoadoutObj.IsValid())
		{
			const int32 Camo = LoadoutObj->GetIntegerField(TEXT("camo"));
			const int32 Headgear = LoadoutObj->GetIntegerField(TEXT("headgear"));
			const int32 Armor = LoadoutObj->GetIntegerField(TEXT("armor"));
			const int32 Finish = LoadoutObj->GetIntegerField(TEXT("weaponFinish"));
			Economy->SetEquippedLoadout(Camo, Headgear, Armor, Finish);
		}
	}
}

void UFPSAuthSubsystem::RefreshLocalPlayerName()
{
	if (CurrentCallsign.IsEmpty()) return;

	UGameInstance* GI = GetGameInstance();
	if (!GI) return;

	UWorld* World = GI->GetWorld();
	if (!World) return;

	APlayerController* PC = World->GetFirstPlayerController();
	if (PC && PC->PlayerState)
	{
		PC->PlayerState->SetPlayerName(CurrentCallsign);
		UE_LOG(LogTemp, Log, TEXT("[FPSAuthSubsystem] Set local player name to '%s'"), *CurrentCallsign);
	}
}
