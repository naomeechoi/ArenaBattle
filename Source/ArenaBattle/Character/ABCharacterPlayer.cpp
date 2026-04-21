// Fill out your copyright notice in the Description page of Project Settings.

#include "Character/ABCharacterPlayer.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "InputMappingContext.h"
#include "InputAction.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "ABCharacterControlData.h"

AABCharacterPlayer::AABCharacterPlayer()
{
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 720.0f, 0.0f);
	GetCharacterMovement()->JumpZVelocity = 800.0f;

	//GetMesh()->SetRelativeLocationAndRotation(FVector(0.0f, 0.0f, -88.0f), FRotator(0.0f, -90.0f, 0.0f));
	//static ConstructorHelpers::FObjectFinder<USkeletalMesh> CharacMesh(
	//	TEXT("/Game/InfinityBladeWarriors/Character/CompleteCharacters/SK_CharM_Cardboard.SK_CharM_Cardboard")
	//);
	//
	//if (CharacMesh.Succeeded())
	//	GetMesh()->SetSkeletalMesh(CharacMesh.Object);
	//
	//static ConstructorHelpers::FClassFinder<UAnimInstance> CharacterAnim(
	//	TEXT("/Game/ArenaBattle/Animation/ABP_ABCharacter.ABP_ABCharacter_C")
	//);
	//
	//if (CharacterAnim.Succeeded())
	//	GetMesh()->SetAnimInstanceClass(CharacterAnim.Class);

	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));

	if (!SpringArm || !Camera)
		return;

	SpringArm->SetupAttachment(RootComponent);
	SpringArm->TargetArmLength = 600.0f;
	SpringArm->bUsePawnControlRotation = true;

	Camera->SetupAttachment(SpringArm);

	//////////////////////////////////////////////////////////////////////
	// Move Action
	//////////////////////////////////////////////////////////////////////
	static ConstructorHelpers::FObjectFinder<UInputAction> ShoulderMoveActionRef(
		TEXT("/Game/ArenaBattle/Input/Actions/IA_ShoulderMove.IA_ShoulderMove")
	);
	if (ShoulderMoveActionRef.Succeeded())
		ShoulderMoveAction = ShoulderMoveActionRef.Object;

	//////////////////////////////////////////////////////////////////////
	// Look Action
	//////////////////////////////////////////////////////////////////////
	static ConstructorHelpers::FObjectFinder<UInputAction> ShoulderLookActionRef(
		TEXT("/Game/ArenaBattle/Input/Actions/IA_ShoulderLook.IA_ShoulderLook")
	);
	if (ShoulderLookActionRef.Succeeded())
		ShoulderLookAction = ShoulderLookActionRef.Object;


	//////////////////////////////////////////////////////////////////////
	// Jump Action
	//////////////////////////////////////////////////////////////////////
	static ConstructorHelpers::FObjectFinder<UInputAction> JumpActionRef(
		TEXT("/Game/ArenaBattle/Input/Actions/IA_Jump.IA_Jump")
	);
	if (JumpActionRef.Succeeded())
		JumpAction = JumpActionRef.Object;

	//////////////////////////////////////////////////////////////////////
	// Quarter Move Action
	//////////////////////////////////////////////////////////////////////
	static ConstructorHelpers::FObjectFinder<UInputAction> QuarterMoveActionRef(
		TEXT("/Game/ArenaBattle/Input/Actions/IA_QuarterMove.IA_QuarterMove")
	);
	if (QuarterMoveActionRef.Succeeded())
		QuarterMoveAction = QuarterMoveActionRef.Object;

	//////////////////////////////////////////////////////////////////////
	// Change Control Action
	//////////////////////////////////////////////////////////////////////
	static ConstructorHelpers::FObjectFinder<UInputAction> ChangeControlActionRef(
		TEXT("/Game/ArenaBattle/Input/Actions/IA_ChangeControl.IA_ChangeControl")
	);
	if (ChangeControlActionRef.Succeeded())
		ChangeControlAction = ChangeControlActionRef.Object;

	//////////////////////////////////////////////////////////////////////
	// Attack Action
	//////////////////////////////////////////////////////////////////////
	static ConstructorHelpers::FObjectFinder<UInputAction> AttackActionRef(
		TEXT("/Game/ArenaBattle/Input/Actions/IA_Attack.IA_Attack")
	);
	if (AttackActionRef.Succeeded())
		AttackAction = AttackActionRef.Object;

	CurrentCharacterControlType = ECharacterControlType::Shoulder;
}

void AABCharacterPlayer::BeginPlay()
{
	Super::BeginPlay();
	SetCharacterControl(CurrentCharacterControlType);
}

void AABCharacterPlayer::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (EnhancedInputComponent)
	{
		EnhancedInputComponent->BindAction(ShoulderMoveAction, ETriggerEvent::Triggered, this, &AABCharacterPlayer::ShoulderMove);
		EnhancedInputComponent->BindAction(ShoulderLookAction, ETriggerEvent::Triggered, this, &AABCharacterPlayer::ShoulderLook);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		EnhancedInputComponent->BindAction(QuarterMoveAction, ETriggerEvent::Triggered, this, &AABCharacterPlayer::QuarterMove);
		EnhancedInputComponent->BindAction(ChangeControlAction, ETriggerEvent::Started, this, &AABCharacterPlayer::ChangeCharacterControl);

		EnhancedInputComponent->BindAction(AttackAction, ETriggerEvent::Triggered, this, &AABCharacterPlayer::Attack);
	}
}

void AABCharacterPlayer::ShoulderMove(const FInputActionValue& Value)
{
	FVector2D Movement = Value.Get<FVector2D>();

	FRotator Rotation = GetControlRotation();
	FRotator YawRotation(0.0f, Rotation.Yaw, 0.0f);

	FVector ForwardVector = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	FVector RightVector = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

	AddMovementInput(ForwardVector, Movement.Y);
	AddMovementInput(RightVector, Movement.X);
}

void AABCharacterPlayer::ShoulderLook(const FInputActionValue& Value)
{
	FVector2D RotationValue = Value.Get<FVector2D>();
	AddControllerYawInput(RotationValue.X);
	AddControllerPitchInput(RotationValue.Y);
}

void AABCharacterPlayer::QuarterMove(const FInputActionValue& Value)
{
	FVector2D Movement = Value.Get<FVector2D>();
	FVector MoveDirection(Movement.Y, Movement.X, 0.0f);
	MoveDirection.Normalize();

	float MovementVectorSize = FMath::Min(1.0f, Movement.Size());

	Controller->SetControlRotation(
		FRotationMatrix::MakeFromX(MoveDirection).Rotator()
	);

	AddMovementInput(MoveDirection, MovementVectorSize);
}

void AABCharacterPlayer::Attack()
{
	ProcessComboCommand();
}

void AABCharacterPlayer::ChangeCharacterControl()
{
	if (CurrentCharacterControlType == ECharacterControlType::Shoulder)
	{
		SetCharacterControl(ECharacterControlType::Quarter);
	}
	else
	{
		SetCharacterControl(ECharacterControlType::Shoulder);
	}
}

void AABCharacterPlayer::SetCharacterControl(ECharacterControlType NewCharacterControlType)
{
	UABCharacterControlData* NewCharacterControl = CharacterControlManager[NewCharacterControlType];
	check(NewCharacterControl);

	SetCharacterControlData(NewCharacterControl);

	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (PlayerController)
	{
		UEnhancedInputLocalPlayerSubsystem* InputSystem =
			ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer());

		if (InputSystem)
		{
			InputSystem->ClearAllMappings();
			InputSystem->AddMappingContext(NewCharacterControl->InputMappingContext, 0);
		}
	}

	CurrentCharacterControlType = NewCharacterControlType;
}

void AABCharacterPlayer::SetCharacterControlData(const UABCharacterControlData* InCharacterControlData)
{
	Super::SetCharacterControlData(InCharacterControlData);

	SpringArm->TargetArmLength = InCharacterControlData->TargetArmLength;
	SpringArm->SetRelativeRotation(InCharacterControlData->RelativeRotation);
	SpringArm->bDoCollisionTest = InCharacterControlData->bDoCollisionTest;
	SpringArm->bUsePawnControlRotation = InCharacterControlData->bUsePawnControlRotation;
	SpringArm->bInheritPitch = InCharacterControlData->bInheritPitch;
	SpringArm->bInheritYaw = InCharacterControlData->bInheritYaw;
	SpringArm->bInheritRoll = InCharacterControlData->bInheritRoll;
}