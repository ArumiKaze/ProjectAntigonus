#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "VRCharacterPawn.generated.h"

UCLASS()
class PROJECTANTIGONUS_API AVRCharacterPawn : public APawn
{
	GENERATED_BODY()

private:

	//---Desired hand---//
	EControllerHand m_desiredhand;

	//---Player Settings---//
	float m_defplayerheight;
	bool m_b_psvrcontrollerrollrotation;

	//---Player State---//
	bool m_b_isteleporting;

	//---Load blueprint object---//
	TSubclassOf<class AVRMotionController> m_handcontroller;

	//---Hand controllers---//
	UPROPERTY()
	class AVRMotionController* m_lefthand;
	UPROPERTY()
	class AVRMotionController* m_righthand;

	//---Controller Input Axis Values---//
	float m_leftthumb_y;
	float m_leftthumb_x;
	float m_rightthumb_y;
	float m_rightthumb_x;

	//---Teleport Camera Settings---//
	float m_camerafadeoutduration;
	float m_camerafadeinduration;
	FLinearColor m_cameratelefadecolor;

	//---Timers---//
	FTimerHandle timerhandle_camerafade;





	//---Set up player height and both controllers---//
	void SetupVROptions();

	//---Controller Input Axis Functions---//
	void LeftYThumb(float value);
	void LeftXThumb(float value);
	void RightYThumb(float value);
	void RightXThumb(float value);

	//---Controller Input Actions---//
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

	//---Calculate teleport rotation---//
	FRotator RotationFromInput(float upaxis, float rightaxis, AVRMotionController *&vrcontroller);

	//---Handle Teleportation---//
	void HandleTeleportation(AVRMotionController*& controller);
	UFUNCTION()
	void DelayTeleportation(AVRMotionController*& delaycontroller);

protected:

	//---Begin Play---//
	virtual void BeginPlay() override;

	//---Components---//
	UPROPERTY(VisibleAnywhere, Category = "Components")
	class UCameraComponent* camera;
	UPROPERTY(VisibleAnywhere, Category = "Components")
	USceneComponent* vrorigin;

public:

	//---Contructor---//
	AVRCharacterPawn();

	//---Tick---//
	virtual void Tick(float DeltaTime) override;

	//---Player Input Component---//
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	//---Getter---//
	EControllerHand GetPawnHand();	//Get desired hand, left or right, for hand creation
};
