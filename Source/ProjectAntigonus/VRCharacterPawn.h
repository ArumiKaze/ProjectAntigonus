#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "VRCharacterPawn.generated.h"

UCLASS()
class PROJECTANTIGONUS_API AVRCharacterPawn : public APawn
{
	GENERATED_BODY()

private:

	//---Axis Values---//
	float mc_y_thumbleft;
	float mc_x_thumbleft;
	float mc_y_thumbright;
	float mc_x_thumbright;

	//---Player Settings---//
	float def_playerheight;
	bool b_psvrcontrollerrollrotation;

	//---Player State---//
	bool b_isteleporting;

	//---Teleport Camera Settings---//
	float camerafadeoutduration;
	float camerafadeinduration;
	FLinearColor cameratelefadecolor;

	//---Timers---//
	FTimerDelegate camerafadedelegate;
	FTimerHandle camerafadetimer;

	//---Blueprint Controllers---//
	TSubclassOf<class AVRMotionController> handcontroller;
	UPROPERTY()
	class AVRMotionController* lefthand;
	UPROPERTY()
	class AVRMotionController* righthand;

	//---Setup---//
	void SetupVROptions();

	//---Handle Teleportation---//
	void HandleTeleportation(AVRMotionController *&controller);
	UFUNCTION()
	void DelayTeleportation(AVRMotionController *&delaycontroller);

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

	FRotator GetRotationFromInput(float upaxis, float rightaxis, AVRMotionController *&vrcontroller);

	void GetLeftY(float value);
	void GetLeftX(float value);
	void GetRightY(float value);
	void GetRightX(float value);

protected:

	virtual void BeginPlay() override;

	//---Components---//
	UPROPERTY(VisibleAnywhere, Category = "Components")
	class UCameraComponent* camera;
	UPROPERTY(VisibleAnywhere, Category = "Components")
	USceneComponent* vrorigin;

public:

	AVRCharacterPawn();

	virtual void Tick(float DeltaTime) override;

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
};
