#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "VRCharacter.generated.h"

UCLASS()
class PROJECTANTIGONUS_API AVRCharacter : public ACharacter
{
	GENERATED_BODY()

private:

	//---Motion Controller Input---//
	//Left grab
	void GrabLeft_Pressed();
	void GrabLeft_Released();
	//Right grab
	void GrabRight_Pressed();
	void GrabRight_Released();
	//Left teleport
	void TeleportLeft_Pressed();
	void TeleportLeft_Released();
	//Right teleport
	void TeleportRight_Pressed();
	void TeleportRight_Released();

protected:

	virtual void BeginPlay() override;

	//---Camera---//
	UPROPERTY(VisibleAnywhere, Category = "Components")
	class UCameraComponent* Camera;

	//---Component to specify origin for the HMD---//
	UPROPERTY(VisibleAnywhere, Category = "Components")
	USceneComponent* VROrigin;

	UPROPERTY(EditDefaultsOnly, Category = "VR")
	bool bPositionalHeadTracking;

	//---Motion Controllers---//
	UPROPERTY(EditDefaultsOnly, Category = "Components")
	class UMotionControllerComponent* LeftHand;
	UPROPERTY(EditDefaultsOnly, Category = "Components")
	class UMotionControllerComponent* RightHand;

public:	

	AVRCharacter();

	virtual void Tick(float DeltaTime) override;

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	void SetupVROptions();

	void ResetHMDOrigin();

	//---Toggle between seated and standing play---//
	void ToggleTrackingSpace();
};
